/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 19:51:34
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-18 21:05:07
 * @FilePath    : /activetask/components/activetask/msg_blk.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"

#include "msg_blk.h"

struct msgblk_pool {
    struct list_head         list_free;
    struct list_head         list_used;
    SemaphoreHandle_t            mutex;
};

static struct msgblk_pool gMsgBlockPool;

static void msgblk_fini(MsgBlk pmsgblk)
{
    // clear data block
    if (!list_empty(&pmsgblk->list_datablk)) {
        DataBlk dblk, temp;
        list_for_each_entry_safe(dblk, temp,
                &pmsgblk->list_datablk, node_msgdata) {
            datablk_free(dblk);
        }
    }

    xSemaphoreTake(gMsgBlockPool.mutex, portMAX_DELAY); // wait until success
    // return to free list
    list_move_tail(&pmsgblk->node_msgblk, &gMsgBlockPool.list_free);
    xSemaphoreGive(gMsgBlockPool.mutex);    // give
}

static void __msgblk_release(struct kref *ref)
{
    MsgBlk mblk = container_of(ref, struct MsgBlk_Stru, refcount);
    msgblk_fini(mblk);
}

/***
 * @description : init message block pool
 * @param        {size_t} max_num - maximum number of message block in the pool
 * @return       {*} result of init
 */
esp_err_t msgblk_pool_init(size_t max_num)
{
    INIT_LIST_HEAD(&gMsgBlockPool.list_free);
    INIT_LIST_HEAD(&gMsgBlockPool.list_used);
    gMsgBlockPool.mutex = xSemaphoreCreateMutex();
    assert("Failed to create mutex for message block pool" \
            && NULL != gMsgBlockPool.mutex);
    for (int i = 0; i < max_num; i++)
    {
        MsgBlk mblk = (MsgBlk)malloc(SIZE_MSGBLK);
        assert("Failed to malloc mem for message block" && NULL != mblk);
        msgblk_init(mblk);
        list_add(&mblk->node_msgblk, &gMsgBlockPool.list_free);
    }
    return ESP_OK;
}

/***
 * @description : clear message block pool
 * @return       {*}
 */
void msgblk_pool_fini(void)
{
    // clear used list
    if (!list_empty(&gMsgBlockPool.list_used)) {
        MsgBlk mblk, temp;
        list_for_each_entry_safe(mblk, temp,
                &gMsgBlockPool.list_used, node_msgblk) {
            msgblk_fini(mblk);  // return to free list
        }
    }

    // clear free list
    if (!list_empty(&gMsgBlockPool.list_free)) {
        MsgBlk mblk, temp;
        list_for_each_entry_safe(mblk, temp,
                &gMsgBlockPool.list_free, node_msgblk) {
            list_del(&mblk->node_msgblk);
            free(mblk);
        }
    }

    vSemaphoreDelete(gMsgBlockPool.mutex);
}

/***
 * @description : malloc a message block
 * @param        {MsgBlk} *ppmsgblk - pointer to a message block pointer
 * @param        {DataBlk} dblk - message data block which would
 *                      attach to this message block
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*} result of malloc
 */
esp_err_t msgblk_malloc(MsgBlk *ppmsgblk, DataBlk dblk,
        uint32_t wait_ms)
{
    if (pdTRUE == xSemaphoreTake(gMsgBlockPool.mutex,
                        pdMS_TO_TICKS(wait_ms))) {
        // got counting
        if (list_empty(&gMsgBlockPool.list_free))
            return MSGBLK_FAILED_MALLOC;

        MsgBlk mblk = list_first_entry(&gMsgBlockPool.list_free,
                            struct MsgBlk_Stru, node_msgblk);
        // move to used list
        list_move_tail(&mblk->node_msgblk, &gMsgBlockPool.list_used);
        xSemaphoreGive(gMsgBlockPool.mutex);    // give
        msgblk_init(mblk);  // init message block
        *ppmsgblk = mblk;
        return msgblk_attach_dbllk(mblk, dblk);
    } else {
        return MALLOC_WAIT_TIMEOUT;
    }
}

/***
 * @description : free a message block
 * @param        {MsgBlk} pmsgblk - pointer to a message block
 * @return       {*} result of free
 */
esp_err_t msgblk_free(MsgBlk pmsgblk)
{
    if (NULL == pmsgblk) return ESP_OK;
    // decrease reference, if 0 recycle with fini
    kref_put(&pmsgblk->refcount, __msgblk_release);  // decrease reference
    return ESP_OK;
}

/***
 * @description : init message block
 * @param        {MsgBlk} pmsgblk - pointer to a message block
 * @return       {*} None
 */
void msgblk_init(MsgBlk pmsgblk)
{
    if (NULL == pmsgblk) return;
    INIT_LIST_HEAD(&pmsgblk->list_datablk);
    pmsgblk->msg_type = -1;
    kref_init(&pmsgblk->refcount);  // reset reference to 1
}

/***
 * @description : increase reference of a message block
 * @param        {MsgBlk} pmsgblk - pointer to a message block
 * @return       {*}
 */
void msgblk_refer(MsgBlk pmsgblk)
{
    kref_get(&pmsgblk->refcount);   // increase reference
}

/***
 * @description : init message block queue
 * @param        {QueueHandle_t} *pqueue - pointer to queue
 * @param        {uint32_t} length - maxinum length of queue
 * @return       {*} result of init
 */
esp_err_t msgblk_queue_init(QueueHandle_t *pqueue, uint32_t length)
{
    *pqueue = xQueueCreate(length, sizeof(MsgBlk));
    return NULL != *pqueue ? ESP_OK : ESP_FAIL;
}

/***
 * @description : clear message block queue
 * @param        {QueueHandle_t} queue
 * @return       {*}
 */
void msgblk_queue_fini(QueueHandle_t queue)
{
    vQueueDelete(queue);
}

/***
 * @description : push a message block into a queue
 * @param        {QueueHandle_t} queue - queue for pushing
 * @param        {MsgBlk} pmsgblk - pointer to a message block
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*} result of push
 */
esp_err_t msgblk_push_queue(QueueHandle_t queue, MsgBlk pmsgblk, uint32_t wait_ms)
{
    if (NULL == queue || NULL == pmsgblk) return INNER_INVAILD_PARAM;

    msgblk_refer(pmsgblk);      // increase reference
    return pdTRUE == xQueueSend(queue,
                (void *)&pmsgblk, pdMS_TO_TICKS(wait_ms)) \
            ? ESP_OK : MSGBLK_FAILED_PUSH;
}

/***
 * @description : pop a message block from a queue
 * @param        {QueueHandle_t} queue - queue for poping
 * @param        {MsgBlk} *ppmsgblk - pointer to a message block pointer
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*} result of pop
 */
esp_err_t msgblk_pop_queue(QueueHandle_t queue, MsgBlk *ppmsgblk, uint32_t wait_ms)
{
    if (NULL == queue || NULL == ppmsgblk) return INNER_INVAILD_PARAM;
    return pdTRUE == xQueueReceive(queue,
                (void *)ppmsgblk, pdMS_TO_TICKS(wait_ms)) \
            ? ESP_OK : MSGBLK_FAILED_POP;
}

/***
 * @description : attach data block to a message block
 * @param        {MsgBlk} pmblk - pointer to a message block
 * @param        {DataBlk} pdblk - pointer to a data bock
 * @return       {*}
 */
esp_err_t msgblk_attach_dbllk(MsgBlk pmblk, DataBlk pdblk)
{
    if (NULL == pmblk || NULL == pdblk) return INNER_INVAILD_PARAM;
    datablk_ref(pdblk);     // increase reference
    list_add_tail(&pdblk->node_msgdata, &pmblk->list_datablk);  // attach
    return ESP_OK;
}

/***
 * @description : detach data block from a message bock
 * @param        {MsgBlk} pmblk
 * @param        {DataBlk} pdblk
 * @return       {*}
 */
esp_err_t msgblk_detach_dbllk(MsgBlk pmblk, DataBlk pdblk)
{
    if (NULL == pmblk || NULL == pdblk) return INNER_INVAILD_PARAM;
    DataBlk pos;
    list_for_each_entry(pos, &pmblk->list_datablk, node_msgdata)
    {
        if (pos == pdblk)
        {
            // found & del
            list_del(&pdblk->node_msgdata);
            datablk_free(pdblk);
            return ESP_OK;
        }
    }
    return INNER_ITEM_NOT_FOUND;
}
