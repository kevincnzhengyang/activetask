/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-23 23:27:52
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-24 17:14:40
 * @FilePath    : /active_task/src/msg_blk.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "linux_llist.h"
#include "linux_macros.h"
#include "inner_err.h"
#include "linux_refcount.h"

#include "msg_blk.h"

struct _inner_msgblk {
    bool                        _valid;
    struct llist_node            _node;
    struct kref               refcount;     // reference counter
    msgblk                       _mblk;
};

#define SIZE_INNER_MSGBLK_HEAD    sizeof(struct _inner_msgblk)

#define TO_INNER_MSGBLK(mblk)  container_of(dblk, struct _inner_msgblk, _mblk)

struct _msgblk_pool {
    on_msgblk_init               _init;
    on_msgblk_fini               _fini;
    on_datablk_attach          _attach;
    on_datablk_dettach        _dettach;
    void                         *_arg;
    struct llist_head        msg_stack;
};

static struct _msgblk_pool g_msgblk_pool;

/***
 * @description : init msg block pool
 * @param        {on_msgblk_init} init_func - callback function after msg block malloc
 * @param        {on_msgblk_fini} fini_func - callback function before msg block release
 * @param        {on_datablk_attach} attch_func - callback function after data block attach
 * @param        {on_datablk_dettach} dettach_func - callback function before data block dettach
 * @param        {void} *arg - user defined parameter for callback function
 * @return       {*}
 */
at_error_t msgblk_pool_init(on_msgblk_init init_func, on_msgblk_fini fini_func,
        on_datablk_attach attch_func, on_datablk_dettach dettach_func, void *arg)
{
    memset(&g_msgblk_pool, 0, sizeof(g_msgblk_pool));
    g_msgblk_pool.msg_stack.first = NULL;
    int blk_num = NO_LESS_THAN(MSGBLK_NUM, 2);
    struct _inner_msgblk *blk = NULL;
    for (int j = 0; j < blk_num; j++) {
        blk = (struct _inner_msgblk *)malloc(SIZE_INNER_MSGBLK_HEAD);
        if (NULL == blk) return MEMORY_MALLOC_FAILED;
        blk->_valid = false;
        kref_init(&blk->refcount);
        blk->_mblk.msg_type = -1;
        INIT_LIST_HEAD(&blk->_mblk.list_datablk);
        llist_add(&blk->_node, &g_msgblk_pool.msg_stack);
    }
    g_msgblk_pool._init = init_func;
    g_msgblk_pool._fini = fini_func;
    g_msgblk_pool._attach = attch_func;
    g_msgblk_pool._dettach = dettach_func;
    g_msgblk_pool._arg = arg;
    KRNL_DEBUG("msg block pool init\n");
    return INNER_RES_OK;
}

/***
 * @description : clear msg block pool
 * @return       {*}
 */
void msgblk_pool_fini(void)
{
    struct llist_node *node = NULL;
    while (NULL != (node = llist_del_first(&g_msgblk_pool.msg_stack))) {
        struct _inner_msgblk *blk = container_of(node, struct _inner_msgblk, _node);
        free(blk);
    }
    KRNL_DEBUG("msg block pool fini\n");
}

/***
 * @description : malloc msg block with a data block attached
 * @param        {msgblk} *db - pointer to data block attached
 * @return       {*} - pointer to msg block, got NULL if failed
 */
msgblk * msgblk_malloc(datablk *db)
{
    struct llist_node *node = llist_del_first(&g_msgblk_pool.msg_stack);
    if (NULL == node) return NULL;
    struct _inner_msgblk *blk = container_of(node, struct _inner_msgblk, _node);
    KRNL_DEBUG("malloc node %p, msg %p, blk %p, data %p\n",
            node, &blk->_mblk, blk, db);
    blk->_valid = false;
    INIT_LIST_HEAD(&blk->_mblk.list_datablk);
    blk->_mblk.msg_type = -1;
    kref_init(&blk->refcount);  // reset reference to 1
    if (NULL != g_msgblk_pool._init)
        if (INNER_RES_OK != g_msgblk_pool._init(&blk->_mblk, g_msgblk_pool._arg)) {
            msgblk_free(&blk->_mblk);
            return NULL;
        }

    msgblk_attach_datablk(&blk->_mblk, db);
    return &blk->_mblk;
}

static void __msgblk_release(struct kref *ref)
{
    struct _inner_msgblk *blk = container_of(ref, struct _inner_msgblk, refcount);
    if (NULL != g_msgblk_pool._fini)
        g_msgblk_pool._fini(&blk->_mblk, g_msgblk_pool._arg);

    KRNL_DEBUG("free node %p, msg %p, blk %p\n",
            &blk->_node, &blk->_mblk, blk);
    blk->_valid = false;

    // clear data block
    if (!list_empty(&blk->_mblk.list_datablk)) {
        datablk *dblk, *temp;
        list_for_each_entry_safe(dblk, temp,
                &blk->_mblk.list_datablk, node_msgdata) {
            datablk_free(dblk);
        }
    }
    INIT_LIST_HEAD(&blk->_mblk.list_datablk);
    llist_add(&blk->_node, &g_msgblk_pool.msg_stack);
}

/***
 * @description : clear msg block, decrease reference
 *                  msg block would be release if reference = 0
 * @param        {msgblk} *mb - pointer to msg block
 * @return       {*}
 */
void msgblk_free(msgblk *mb)
{
    if (NULL == mb) return;

    struct _inner_msgblk *blk = container_of(mb, struct _inner_msgblk, _mblk);
    // decrease reference, if 0 recycle with fini
    kref_put(&blk->refcount, __msgblk_release);  // decrease reference
}

/***
 * @description : set valid flag of msg block
 * @param        {msgblk} *mb - pointer to msg block
 * @return       {*}
 */
void msgblk_set_valid(msgblk *mb)
{
    if (NULL == mb) return;
    (container_of(mb, struct _inner_msgblk, _mblk))->_valid = true;
}

/***
 * @description : check valida flag
 * @param        {msgblk} *mb - pointer to msg block
 * @return       {*}
 */
bool msgblk_is_valid(msgblk *mb)
{
    if (NULL == mb) return false;
    return (container_of(mb, struct _inner_msgblk, _mblk))->_valid;
}

/***
 * @description : increase reference of msg block
 * @param        {msgblk} *mb - pointer to msg block
 * @return       {*}
 */
void msgblk_ref(msgblk *mb)
{
    if (NULL == mb) return;
    kref_get(&(container_of(mb, struct _inner_msgblk, _mblk))->refcount);
}

/***
 * @description : attach data block to msg block
 * @param        {msgblk} *mb - pointer to msg block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
at_error_t msgblk_attach_datablk(msgblk *mb, datablk *db)
{
    if (NULL == mb || NULL == db) return INNER_INVAILD_PARAM;
    if (NULL != g_msgblk_pool._attach) {
        int res = g_msgblk_pool._attach(mb, db, g_msgblk_pool._arg);
        if (INNER_RES_OK != res) return res;
    }
    datablk_ref(db);     // increase reference
    list_add_tail(&db->node_msgdata, &mb->list_datablk);  // attach
    return INNER_RES_OK;
}

/***
 * @description : dettach data block from msg block
 * @param        {msgblk} *mb - pointer to msg block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void msgblk_dettach_datablk(msgblk *mb, datablk *db)
{
    if (NULL == mb || NULL == db) return;

    datablk *pos;
    list_for_each_entry(pos, &mb->list_datablk, node_msgdata)
    {
        if (pos == db)
        {
            // found & del
            list_del(&db->node_msgdata);
            datablk_free(db);
            if (NULL != g_msgblk_pool._dettach) {
                g_msgblk_pool._dettach(mb, db, g_msgblk_pool._arg);
            }
            return;
        }
    }
}

/***
 * @description : push msg block in a circular queue
 * @param        {circ_queue} *queue - pointer to queue
 * @param        {msgblk} *mb - poiner to msg block
 * @param        {int} wait_ms - wait time in ms
 * @return       {*}
 */
at_error_t msgblk_push_circ_queue(circ_queue *queue, msgblk *mb, int wait_ms)
{
    if (NULL == queue || NULL == mb) return INNER_INVAILD_PARAM;
    msgblk_ref(mb);
    return queue->queue_push(queue, (void *)mb, wait_ms);
}

/***
 * @description : pop msg block from a circular queue
 * @param        {circ_queue} *queue - pointer to queue
 * @param        {msgblk} **pmb - pointer to msg block pointer
 * @param        {int} wait_ms - wait time in ms
 * @return       {*}
 */
at_error_t msgblk_pop_circ_queue(circ_queue *queue, msgblk **pmb, int wait_ms)
{
    if (NULL == queue || NULL == pmb) return INNER_INVAILD_PARAM;
    return queue->queue_pop(queue, (void **)pmb, wait_ms);
}

/***
 * @description : get first data block from message block
 * @param        {msgblk} *mb - pointer to message block
 * @return       {*}
 */
datablk *msgblk_first_datablk(msgblk *mb)
{
    if (NULL == mb) return NULL;

    if (list_empty(&mb->list_datablk)) return NULL;

    return list_first_entry(&mb->list_datablk, datablk, node_msgdata);
}