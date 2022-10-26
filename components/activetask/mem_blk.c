/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-22 11:01:22
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-26 21:45:26
 * @FilePath    : /activetask/components/activetask/mem_blk.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stdlib.h>
#include <string.h>

#include "linux_llist.h"
#include "linux_macros.h"

#include "mem_blk.h"

struct _memblk {
    int _capacity;
    int _size;
    bool _valid;
    struct llist_node _node;
    char data[0];
};

#define SIZE_MEMBLK_HEAD    sizeof(struct _memblk)

#define MEMBLK_INIT(cap, size)  { \
    ._capacity = cap, \
    ._size = size,\
    ._valid = false, \
    ._node = {NULL}, \
}

#define MEMBLK_SIZE(arg)    ((container_of((arg), struct _memblk, data))->_size)

#define MEMBLK_CAP(arg)    ((container_of((arg), struct _memblk, data))->_capacity)

struct _memblk_pool {
    struct llist_head mem_stack[MEMBLK_STACK_NUM];
};

#define MEMBLK_STACK_INDEX(blk) ((int)(((blk)->_capacity-1)/MEMBLK_MIN_SIZE) < MEMBLK_STACK_NUM \
    ? (int)(((blk)->_capacity-1)/MEMBLK_MIN_SIZE) : -1)

static struct _memblk_pool g_memblk_pool;

/***
 * @description : malloc a memory block
 * @param        {int} size - size of memory block
 * @return       {*} pointer to data buffer
 */
void *memblk_malloc(int size)
{
    int stack_index = (int)((size-1)/MEMBLK_MIN_SIZE) < MEMBLK_STACK_NUM \
            ? (int)((size-1)/MEMBLK_MIN_SIZE) : -1;
    KRNL_DEBUG("malloc size %d, stack %d\n", size, stack_index);
    if (-1 == stack_index) return NULL;

    // find possible stack
    for (; stack_index < MEMBLK_STACK_NUM; stack_index++) {
        if (!llist_empty(&g_memblk_pool.mem_stack[stack_index])) break;
    }
    KRNL_DEBUG("malloc size %d, serached stack %d\n", size, stack_index);
    if (MEMBLK_STACK_NUM <= stack_index) return NULL;

    struct llist_node *node = llist_del_first(
                &g_memblk_pool.mem_stack[stack_index]);
    if (NULL == node) return NULL;
    struct _memblk *blk = container_of(node, struct _memblk, _node);
    KRNL_DEBUG("malloc size %d, serached stack %d, node %p, data %p, blk %p, cap %d\n",
            size, stack_index, node, blk->data, blk, blk->_capacity);
    blk->_size = size;
    blk->_valid = false;
    memset(blk->data, 0, blk->_capacity);
    KRNL_DEBUG("===malloc size %d, serached stack %d, node %p, data %p, blk %p, cap %d\n",
            size, stack_index, node, blk->data, blk, blk->_capacity);
    return blk->data;
}

/***
 * @description : recyle a memory block
 * @param        {void} *arg - pointer to memory block
 * @return       {*}
 */
void memblk_free(void *arg)
{
    if (NULL == arg) return;
    struct _memblk *blk = container_of(arg, struct _memblk, data);

    KRNL_DEBUG("free size %d, stack %d, node %p, data %p, blk %p, cap %d\n",
            blk->_size, MEMBLK_STACK_INDEX(blk), &blk->_node, arg, blk, blk->_capacity);
    blk->_size = 0;
    blk->_valid = false;
    llist_add(&blk->_node, &g_memblk_pool.mem_stack[MEMBLK_STACK_INDEX(blk)]);
}

/***
 * @description : set the flag of memory block to valid
 * @param        {void} *blk
 * @return       {*}
 */
void memblk_set_valid(void *arg)
{
    if (NULL == arg) return;
    container_of(arg, struct _memblk, data)->_valid = true;
}

/***
 * @description : check if memory block is valid
 * @param        {void} *arg
 * @return       {*}
 */
bool memblk_is_valid(void *arg)
{
    if (NULL == arg) return false;
    return container_of(arg, struct _memblk, data)->_valid;
}

/***
 * @description : init memory block pool
 * @return       {*}
 */
at_error_t memblk_pool_init(void)
{
    memset(&g_memblk_pool, 0, sizeof(g_memblk_pool));
    for (int i = 0; i < MEMBLK_STACK_NUM; i++) {
        g_memblk_pool.mem_stack[i].first = NULL;
        int blk_num = NO_LESS_THAN(MEMBLK_NUM >> i, 2);     // decrease blk num to half
        int blk_cap = MEMBLK_MIN_SIZE * (i + 1);
        struct _memblk *blk = NULL;
        for (int j = 0; j < blk_num; j++) {
            blk = (struct _memblk *)malloc(blk_cap + SIZE_MEMBLK_HEAD);
            if (NULL == blk) return MEMORY_MALLOC_FAILED;
            blk->_capacity = blk_cap;
            blk->_size = 0;
            blk->_valid = false;
            llist_add(&blk->_node, &g_memblk_pool.mem_stack[i]);
        }
    }
    return INNER_RES_OK;
}

/***
 * @description : clear memory block pool
 * @return       {*}
 */
void memblk_pool_fini(void)
{
    for (int i = 0; i < MEMBLK_STACK_NUM; i++) {
        struct llist_node *node = NULL;
        while (NULL != (node = llist_del_first(&g_memblk_pool.mem_stack[i]))) {
            struct _memblk *blk = container_of(node, struct _memblk, _node);
            free(blk);
        }
    }
}
