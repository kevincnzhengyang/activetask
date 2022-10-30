/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-22 14:16:25
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-22 14:16:25
 * @FilePath    : /active_task/include/circ_queue.h
 * @Description : queue based on circular buffer
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _CIRCULAR_QUEUE_H_
#define _CIRCULAR_QUEUE_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "inner_err.h"
#include "linux_circ.h"

#define QUEUE_INTV_MS     10

#ifdef __cplusplus
extern "C" {
#endif

typedef struct circ_queue_t circ_queue;

struct circ_queue_t {
    struct circ_buf          _circ_buf;

    /***
     * @description : push a pointer after tail of the queue
     * @param        {circ_queue} *pqueue - queue
     * @param        {void} *arg - pointer
     * @param        {int} wait_ms - wait time in ms
     * @return       {*}
     */
    at_error_t (*queue_push)(circ_queue *pqueue, void *arg, int wait_ms);

    /***
     * @description : pop a pointer from head of the queue
     * @param        {circ_queue} *pqueue - queue
     * @param        {void} **arg - pointer to pointer
     * @param        {int} wait_ms - wait time in ms
     * @return       {*}
     */
    at_error_t (*queue_pop)(circ_queue *pqueue, void **arg, int wait_ms);
};

/***
 * @description : create a circualr queue
 * @param        {size_t} queue_length
 * @return       {*}
 */
circ_queue * circ_queue_create(size_t queue_length);

/***
 * @description : delete a circular queue
 * @param        {circ_queue} *pqueue
 * @return       {*}
 */
void circ_queue_delete(circ_queue *pqueue);

#define circ_queue_is_empty(pqueue) CIRC_IS_EMPTY(&(pqueue)->_circ_buf)

#define circ_queue_is_full(pqueue) CIRC_IS_FULL(&(pqueue)->_circ_buf)

#ifdef __cplusplus
}
#endif

#endif /* _CIRCULAR_QUEUE_H_ */
