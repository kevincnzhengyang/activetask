/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-23 11:47:23
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-24 17:14:07
 * @FilePath    : /active_task/src/data_blk.c
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

#include "data_blk.h"

struct _inner_datablk {
    int                      _capacity;
    int                          _size;
    bool                        _valid;
    struct llist_node            _node;
    void                        *_base;
    struct kref               refcount;     // reference counter
    datablk                      _dblk;
};

#define SIZE_INNER_DATABLK_HEAD    sizeof(struct _inner_datablk)

#define TO_INNER_DATABLK(dblk)  container_of(dblk, struct _inner_datablk, _dblk)

struct _datablk_pool {
    on_datablk_init              _init;
    on_datablk_fini              _fini;
    void                         *_arg;
    struct llist_head data_stack[DATABLK_STACK_NUM];
};

#define DATABLK_STACK_INDEX(blk) ((int)(((blk)->_capacity-1)/DATABLK_MIN_SIZE) < DATABLK_STACK_NUM \
    ? (int)(((blk)->_capacity-1)/DATABLK_MIN_SIZE) : -1)

static struct _datablk_pool g_datablk_pool;

/***
 * @description : init data block pool
 * @param        {on_int} init_func - callback function after data block malloc
 * @param        {on_datablk_fini} fini_func - callback function before data block release
 * @param        {void} *arg - user defined parameter for callback function
 * @return       {*}
 */
at_error_t datablk_pool_init(on_datablk_init init_func, on_datablk_fini fini_func, void *arg)
{
    memset(&g_datablk_pool, 0, sizeof(g_datablk_pool));
    for (int i = 0; i < DATABLK_STACK_NUM; i++) {
        g_datablk_pool.data_stack[i].first = NULL;
        int blk_num = NO_LESS_THAN(DATABLK_NUM >> i, 2);     // decrease blk num to half
        int blk_cap = DATABLK_MIN_SIZE * (i + 1);
        struct _inner_datablk *blk = NULL;
        for (int j = 0; j < blk_num; j++) {
            blk = (struct _inner_datablk *)malloc(blk_cap + SIZE_INNER_DATABLK_HEAD);
            if (NULL == blk) return MEMORY_MALLOC_FAILED;
            blk->_capacity = blk_cap;
            blk->_size = 0;
            blk->_valid = false;
            blk->_base = (void *)blk + SIZE_INNER_DATABLK_HEAD;
            blk->_dblk.rd_ptr = blk->_dblk.wr_ptr = blk->_base;
            kref_init(&blk->refcount);
            blk->_dblk.data_type = -1;
            INIT_LIST_HEAD(&blk->_dblk.node_msgdata);
            llist_add(&blk->_node, &g_datablk_pool.data_stack[i]);
        }
    }
    g_datablk_pool._init = init_func;
    g_datablk_pool._fini = fini_func;
    g_datablk_pool._arg = arg;
    KRNL_DEBUG("data block pool init\n");
    return INNER_RES_OK;
}

/***
 * @description : clear data block pool
 * @return       {*}
 */
void datablk_pool_fini(void)
{
    for (int i = 0; i < DATABLK_STACK_NUM; i++) {
        struct llist_node *node = NULL;
        while (NULL != (node = llist_del_first(&g_datablk_pool.data_stack[i]))) {
            struct _inner_datablk *blk = container_of(node, struct _inner_datablk, _node);
            free(blk);
        }
    }
    KRNL_DEBUG("data block pool fini\n");
}

/***
 * @description : malloc data block
 * @param        {int} size - desired size of data block
 * @return       {*} - pointer to data block, got NULL if failed
 */
datablk * datablk_malloc(int size)
{
    int stack_index = (int)((size-1)/DATABLK_MIN_SIZE) < DATABLK_STACK_NUM \
            ? (int)((size-1)/DATABLK_MIN_SIZE) : -1;
    KRNL_DEBUG("malloc size %d, stack %d\n", size, stack_index);
    if (-1 == stack_index) return NULL;

    // find possible stack
    for (; stack_index < DATABLK_STACK_NUM; stack_index++) {
        if (!llist_empty(&g_datablk_pool.data_stack[stack_index])) break;
        else KRNL_DEBUG("malloc size %d, stack %d empty\n", size, stack_index);
    }
    KRNL_DEBUG("malloc size %d, serached stack %d\n", size, stack_index);
    if (DATABLK_STACK_NUM <= stack_index) return NULL;

    struct llist_node *node = llist_del_first(
                &g_datablk_pool.data_stack[stack_index]);
    if (NULL == node) return NULL;
    struct _inner_datablk *blk = container_of(node, struct _inner_datablk, _node);
    KRNL_DEBUG("malloc size %d, serached stack %d, node %p, data %p, blk %p, cap %d\n",
            size, stack_index, node, &blk->_dblk, blk, blk->_capacity);
    blk->_size = size;
    blk->_valid = false;
    INIT_LIST_HEAD(&blk->_dblk.node_msgdata);
    blk->_dblk.data_type = -1;
    kref_init(&blk->refcount);  // reset reference to 1
    blk->_dblk.rd_ptr = blk->_dblk.wr_ptr = blk->_base;
    memset(blk->_base, 0, blk->_capacity);
    KRNL_DEBUG("===malloc size %d, serached stack %d, node %p, data %p, blk %p, cap %d\n",
            size, stack_index, node, &blk->_dblk, blk, blk->_capacity);
    if (NULL != g_datablk_pool._init)
        if (INNER_RES_OK != g_datablk_pool._init(&blk->_dblk, g_datablk_pool._arg))
        {
            datablk_free(&blk->_dblk);
            return NULL;
        }
    return &blk->_dblk;
}

static void __datablk_release(struct kref *ref)
{
    struct _inner_datablk *blk = container_of(ref, struct _inner_datablk, refcount);
    if (NULL != g_datablk_pool._fini)
        g_datablk_pool._fini(&blk->_dblk, g_datablk_pool._arg);

    KRNL_DEBUG("free size %d, stack %d, node %p, data %p, blk %p, cap %d\n",
            blk->_size, DATABLK_STACK_INDEX(blk), &blk->_node, &blk->_dblk, blk, blk->_capacity);
    blk->_size = 0;
    blk->_valid = false;
    INIT_LIST_HEAD(&blk->_dblk.node_msgdata);
    llist_add(&blk->_node, &g_datablk_pool.data_stack[DATABLK_STACK_INDEX(blk)]);
}

/***
 * @description : clear data block, decrease reference
 *                  data block would be release if reference = 0
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void datablk_free(datablk *db)
{
    if (NULL == db) return;

    struct _inner_datablk *blk = container_of(db, struct _inner_datablk, _dblk);
    // decrease reference, if 0 recycle with fini
    kref_put(&blk->refcount, __datablk_release);  // decrease reference
}

/***
 * @description : set valid flag of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void datablk_set_valid(datablk *db)
{
    if (NULL == db) return;
    (container_of(db, struct _inner_datablk, _dblk))->_valid = true;
}

/***
 * @description : check valida flag
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
bool datablk_is_valid(datablk *db)
{
    if (NULL == db) return false;
    return (container_of(db, struct _inner_datablk, _dblk))->_valid;
}

/***
 * @description : increase reference of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void datablk_ref(datablk *db)
{
    if (NULL == db) return;
    kref_get(&(container_of(db, struct _inner_datablk, _dblk))->refcount);
}

/***
 * @description : get capacity of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
int datablk_get_cap(datablk *db)
{
    if (NULL == db) return -1;
    return (container_of(db, struct _inner_datablk, _dblk))->_capacity;
}

/***
 * @description : get size of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
int datablk_get_size(datablk *db)
{
    if (NULL == db) return -1;
    return (container_of(db, struct _inner_datablk, _dblk))->_size;
}

/***
 * @description : get base address of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void *datablk_get_base(datablk *db)
{
    if (NULL == db) return NULL;
    return (container_of(db, struct _inner_datablk, _dblk))->_base;
}

/***
 * @description : move read ptr with n bytes
 * @param        {datablk} *db - pointer to data block
 * @param        {int} nbytes - bytes
 * @return       {*} - return length if n > length
 */
int datablk_move_rd(datablk *db, int nbytes)
{
    if (NULL == db) return -1;

    int n = nbytes > datablk_length(db) ? datablk_length(db) : nbytes;

    db->rd_ptr += n;
    return n;
}

/***
 * @description : move write ptr with n bytes
 * @param        {datablk} *db - pointer to data block
 * @param        {int} nbytes - bytes
 * @return       {*} - return space if n > space
 */
int datablk_move_wr(datablk *db, int nbytes)
{
    if (NULL == db) return -1;

    int n = nbytes > datablk_space(db) ? datablk_space(db) : nbytes;

    db->wr_ptr += n;
    return n;
}
