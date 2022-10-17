/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 19:51:26
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-17 19:51:26
 * @FilePath    : /activetask/components/activetask/msg_blk.h
 * @Description : message block based on FreeRTOS Ring Buffer
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _MESSAGE_BLOCK_H_
#define _MESSAGE_BLOCK_H_

#include <stddef.h>
#include <string.h>

#include "esp_err.h"
#include "freertos/queue.h"

#include "inner_err.h"
#include "linux_refcount.h"
#include "linux_list.h"
#include "data_blk.h"

struct MsgBlk_Stru {
    int32_t                   msg_type;     // message type
    struct list_head      list_datablk;     // message datablock list
    struct list_head       node_msgblk;     // node of message block
    struct kref               refcount;     // reference counter
};

#define SIZE_MSGBLK     sizeof(struct MsgBlk_Stru)

typedef struct MsgBlk_Stru *   MsgBlk;

/***
 * @description : init message block pool
 * @param        {size_t} max_num - maximum number of message block in the pool
 * @return       {*} result of init
 */
err_t msgblk_pool_init(size_t max_num);

/***
 * @description : clear message block pool
 * @return       {*}
 */
void msgblk_pool_fini(void);

/***
 * @description : malloc a message block
 * @param        {MsgBlk} *ppmsgblk - pointer to a message block pointer
 * @param        {DataBlk} dblk - message data block which would
 *                      attach to this message block
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*} result of malloc
 */
err_t msgblk_malloc(MsgBlk *ppmsgblk, DataBlk dblk, uint32_t wait_ms);

/***
 * @description : free a message block
 * @param        {MsgBlk} pmsgblk - pointer to a message block
 * @return       {*} result of free
 */
err_t msgblk_free(MsgBlk pmsgblk);

/***
 * @description : init message block
 * @param        {MsgBlk} pmsgblk - pointer to a message block
 * @return       {*} None
 */
void msgblk_init(MsgBlk pmsgblk);

/***
 * @description : increase reference of a message block
 * @param        {MsgBlk} pmsgblk - pointer to a message block
 * @return       {*}
 */
void msgblk_refer(MsgBlk pmsgblk);

/***
 * @description : init message block queue
 * @param        {QueueHandle_t} *pqueue - pointer to queue
 * @param        {uint32_t} length - maxinum length of queue
 * @return       {*} result of init
 */
err_t msgblk_queue_init(QueueHandle_t *pqueue, uint32_t length);

/***
 * @description : clear message block queue
 * @param        {QueueHandle_t} queue
 * @return       {*}
 */
void msgblk_queue_fini(QueueHandle_t queue);

/***
 * @description : push a message block into a queue
 * @param        {QueueHandle_t} queue - queue for pushing
 * @param        {MsgBlk} pmsgblk - pointer to a message block
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*} result of push
 */
err_t msgblk_push_queue(QueueHandle_t queue, MsgBlk pmsgblk, uint32_t wait_ms);

/***
 * @description : pop a message block from a queue
 * @param        {QueueHandle_t} queue - queue for poping
 * @param        {MsgBlk} *ppmsgblk - pointer to a message block pointer
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*} result of pop
 */
err_t msgblk_pop_queue(QueueHandle_t queue, MsgBlk *ppmsgblk, uint32_t wait_ms);

/***
 * @description : attach data block to a message block
 * @param        {MsgBlk} pmblk - pointer to a message block
 * @param        {DataBlk} pdblk - pointer to a data bock
 * @return       {*}
 */
err_t msgblk_attach_dbllk(MsgBlk pmblk, DataBlk pdblk);

/***
 * @description : detach data block from a message bock
 * @param        {MsgBlk} pmblk
 * @param        {DataBlk} pdblk
 * @return       {*}
 */
err_t msgblk_detach_dbllk(MsgBlk pmblk, DataBlk pdblk);

#endif /* _MESSAGE_BLOCK_H_ */

