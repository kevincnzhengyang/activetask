/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-22 17:57:36
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-24 20:34:07
 * @FilePath    : /active_task/src/circ_queue.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>

#include "linux_macros.h"
#include "circ_queue.h"

static at_error_t dft_queue_push(circ_queue *pqueue, void *arg, int wait_ms)
{
    if (NULL == pqueue || NULL == arg) return INNER_INVAILD_PARAM;
    int index = -1;
    for (int i = QUEUE_INTV_MS;
        (0 == wait_ms || i < wait_ms) && -1 == (index = CIRC_WRITE_ONE(&(pqueue->_circ_buf)));
        i+=QUEUE_INTV_MS)
    {
        delay_ms(QUEUE_INTV_MS);
    }
    if (-1 == index) return OPR_WAIT_TIMEOUT;

    // got index to write
    ((void **)pqueue->_circ_buf.buf)[CIRC_GET_INDEX(&(pqueue->_circ_buf), index)] = arg;
    KRNL_DEBUG("push queue %p pos %ld with %p\n",
            pqueue, index & pqueue->_circ_buf.size, arg);
    return INNER_RES_OK;
}

static at_error_t dft_queue_pop(circ_queue *pqueue, void **arg, int wait_ms)
{
    if (NULL == pqueue || NULL == arg) return INNER_INVAILD_PARAM;
    int index = -1;
    for (int i = QUEUE_INTV_MS;
        (0 == wait_ms || i < wait_ms) && -1 == (index = CIRC_READ_ONE(&pqueue->_circ_buf));
        i+=QUEUE_INTV_MS)
    {
        delay_ms(QUEUE_INTV_MS);
    }
    if (-1 == index) return OPR_WAIT_TIMEOUT;

    // got index to read
    *arg = ((void **)pqueue->_circ_buf.buf)[CIRC_GET_INDEX(&(pqueue->_circ_buf), index)];
    KRNL_DEBUG("pop queue %p pos %ld with %p\n",
            pqueue, index & pqueue->_circ_buf.size, *arg);
    return INNER_RES_OK;
}

circ_queue * circ_queue_create(size_t queue_length)
{
    int upto2 = get_powerof2(queue_length);
    KRNL_DEBUG("upto2 of %ld: %d\n", queue_length, upto2);
    if (0 == queue_length) return NULL;
    circ_queue *pqueue = (circ_queue *)malloc(sizeof(circ_queue));
    if (NULL == pqueue) return NULL;
    pqueue->_circ_buf.head = ATOMIC_VAR_INIT(0);
    pqueue->_circ_buf.tail = ATOMIC_VAR_INIT(0);
    pqueue->_circ_buf.size = upto2 - 1;
    pqueue->_circ_buf.buf = malloc(pqueue->_circ_buf.size * sizeof(void *));
    if (NULL == pqueue->_circ_buf.buf) {
        free(pqueue);
        return NULL;
    }
    pqueue->queue_push = dft_queue_push;
    pqueue->queue_pop = dft_queue_pop;
    return pqueue;
};

void circ_queue_delete(circ_queue *pqueue)
{
    if (NULL == pqueue) return;
    if (NULL != pqueue->_circ_buf.buf) free(pqueue->_circ_buf.buf);
    free(pqueue);
}
