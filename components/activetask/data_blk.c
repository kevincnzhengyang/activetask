/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 19:50:59
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-18 21:02:26
 * @FilePath    : /activetask/components/activetask/data_blk.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "data_blk.h"

struct datablk_pool {
    struct list_head         list_free;
    struct list_head         list_used;
    SemaphoreHandle_t            mutex;
};

static struct datablk_pool gDataBlockPool;

static void datablk_fini(DataBlk pdblk)
{
    xSemaphoreTake(gDataBlockPool.mutex, portMAX_DELAY); // wait until success

    // xRingbufferReceiveUpTo(buf_handle, &item_size, pdMS_TO_TICKS(1000), sizeof(tx_item));
    // vRingbufferReturnItem(gDataBlockPool.ringbuff, (void *)pdblk->base_ptr);

    // return to free list
    list_move_tail(&pdblk->node_datablk, &gDataBlockPool.list_free);
    xSemaphoreGive(gDataBlockPool.mutex);    // give
    free(pdblk->base_ptr);  // free mem for data
}

static void __datablk_release(struct kref *ref)
{
    DataBlk dblk = container_of(ref, struct DataBlk_Stru, refcount);
    datablk_fini(dblk);
}

/***
 * @description : init data block pool
 * @param        {size_t} max_num - maximum number of data block
 * @param        {size_t} buff_size - total buff size for data
 * @return       {*}
 */
esp_err_t datablk_pool_init(size_t max_num, size_t buff_size)
{
    INIT_LIST_HEAD(&gDataBlockPool.list_free);
    INIT_LIST_HEAD(&gDataBlockPool.list_used);
    gDataBlockPool.mutex = xSemaphoreCreateMutex();
    assert("Failed to create mutex for data block pool" \
            && NULL != gDataBlockPool.mutex);
    for (int i = 0; i < max_num; i++)
    {
        DataBlk dblk = (DataBlk)malloc(SIZE_DATABLK);
        assert("Failed to malloc mem for data block" && NULL != dblk);
        datablk_init(dblk);
        list_add(&dblk->node_datablk, &gDataBlockPool.list_free);
    }
    return true;
}

/***
 * @description : clear data block pool
 * @return       {*}
 */
void datablk_pool_fini(void)
{
    // clear used list
    if (!list_empty(&gDataBlockPool.list_used)) {
        DataBlk dblk, temp;
        list_for_each_entry_safe(dblk, temp,
                &gDataBlockPool.list_used, node_datablk) {
            datablk_fini(dblk); // return to free list
        }
    }

    // clear free list
    if (!list_empty(&gDataBlockPool.list_free)) {
        DataBlk dblk, temp;
        list_for_each_entry_safe(dblk, temp,
                &gDataBlockPool.list_free, node_datablk) {
            list_del(&dblk->node_datablk);
            free(dblk);
        }
    }

    vSemaphoreDelete(gDataBlockPool.mutex);
}

/***
 * @description : malloc data block
 * @param        {DataBlk} *ppdblk - pointer to a data block pointer
 * @param        {size_t} data_size - size of data to be stored
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*}
 */
esp_err_t datablk_malloc(DataBlk *ppdblk, size_t data_size, uint32_t wait_ms)
{
    if (0 == data_size || NULL == ppdblk) return INNER_INVAILD_PARAM;

    char *buff = (char *)malloc(data_size);
    if (NULL == buff) return DATABLK_FAILED_MALLOC;

    if (pdTRUE == xSemaphoreTake(gDataBlockPool.mutex,
                        pdMS_TO_TICKS(wait_ms))) {
        // got counting
        if (list_empty(&gDataBlockPool.list_free))
        {
            free(buff);
            return DATABLK_FAILED_MALLOC;
        }

        DataBlk dblk = list_first_entry(&gDataBlockPool.list_free,
                            struct DataBlk_Stru, node_datablk);
        // move to used list
        list_move_tail(&dblk->node_datablk, &gDataBlockPool.list_used);
        xSemaphoreGive(gDataBlockPool.mutex);    // give
        datablk_init(dblk);  // init data block
        dblk->base_ptr = dblk->rd_ptr = dblk->wr_ptr = (unsigned char *)buff;
        dblk->buff_size = data_size;
        *ppdblk = dblk;
        return ESP_OK;
    } else {
        free(buff);
        return MALLOC_WAIT_TIMEOUT;
    }
}

/***
 * @description : recyle data block
 * @param        {DataBlk} pdblk - pinter to a data block
 * @return       {*}
 */
esp_err_t datablk_free(DataBlk pdblk)
{
    if (NULL == pdblk) return ESP_OK;
    // decrease reference, if 0 recycle with fini
    kref_put(&pdblk->refcount, __datablk_release);  // decrease reference
    return ESP_OK;
}

/***
 * @description : increase reference of a data block
 * @param        {DataBlk} pdblk - pointer to a data block
 * @return       {*}
 */
void datablk_ref(DataBlk pdblk)
{
    kref_get(&pdblk->refcount);   // increase reference
}

/***
 * @description : init a data block
 * @param        {DataBlk} pdblk - pinter to a data block
 * @return       {*}
 */
void datablk_init(DataBlk pdblk)
{
    if (NULL == pdblk) return;
    INIT_LIST_HEAD(&pdblk->node_msgdata);
    pdblk->data_type = -1;
    kref_init(&pdblk->refcount);  // reset reference to 1
    pdblk->buff_size = 0;
    pdblk->base_ptr = pdblk->rd_ptr = pdblk->wr_ptr = NULL;
}

/***
 * @description : read from a data block to a buffer
 * @param        {DataBlk} pdblk - pointer to a data block
 * @param        {void} *buff - pointer to buffer
 * @param        {size_t} *psize - pointer to size to be readed
 * @return       {*} bytes readed in psize
 */
esp_err_t datablk_read(DataBlk pdblk, void *buff, size_t *psize)
{
    if (NULL == buff || NULL == psize || 0 == *psize)
        return INNER_INVAILD_PARAM;

    size_t count = datablk_count(pdblk);
    count = count > *psize ? *psize : count;

    memcpy(buff, pdblk->rd_ptr, count);
    pdblk->rd_ptr += count;
    *psize = count;
    return ESP_OK;
}

/***
 * @description : write to a data block from a buffer
 * @param        {DataBlk} pdblk - pointer to a data block
 * @param        {void} *buff - pointer to buffer
 * @param        {size_t} *psize - pointer to size to be wrote
 * @return       {*} bytes wrote in psize
 */
esp_err_t datablk_write(DataBlk pdblk, void *buff, size_t *psize)
{
    if (NULL == buff || NULL == psize || 0 == *psize)
        return INNER_INVAILD_PARAM;

    size_t len = datablk_space(pdblk);
    len = len > *psize ? *psize : len;

    memcpy(pdblk->wr_ptr, buff, len);
    pdblk->wr_ptr += len;
    *psize = len;
    return ESP_OK;
}
