/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 19:50:43
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-17 19:50:43
 * @FilePath    : /activetask/components/activetask/data_blk.h
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _DATA_BLOCK_H_
#define _DATA_BLOCK_H_

#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_err.h"

#include "inner_err.h"
#include "linux_list.h"
#include "linux_refcount.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DataBlk_Stru {
    int32_t                  data_type;
    struct list_head      node_msgdata;     // node for message block
    struct list_head      node_datablk;     // node of data block
    struct kref               refcount;     // reference counter
    size_t                   buff_size;
    unsigned char            *base_ptr;
    unsigned char              *rd_ptr;
    unsigned char              *wr_ptr;
};

#define SIZE_DATABLK     sizeof(struct DataBlk_Stru)

typedef struct DataBlk_Stru * DataBlk;

#define datablk_is_empty(dblk) ((dblk)->rd_ptr == (dblk)->wr_ptr)

#define datablk_is_full(dblk) ((dblk)->buff_size == ((dblk)->wr_ptr - (dblk)->base))

#define datablk_count(dblk) ((dblk)->wr_ptr - (dblk)->rd_ptr)

#define datablk_space(dblk) ((dblk)->buff_size - ((dblk)->wr_ptr - (dblk)->base_ptr))

/***
 * @description : init data block pool
 * @param        {size_t} max_num - maximum number of data block
 * @param        {size_t} buff_size - total buff size for data
 * @return       {*}
 */
esp_err_t datablk_pool_init(size_t max_num, size_t buff_size);

/***
 * @description : clear data block pool
 * @return       {*}
 */
void datablk_pool_fini(void);

/***
 * @description : malloc data block
 * @param        {DataBlk} *ppdblk - pointer to a data block pointer
 * @param        {size_t} data_size - size of data to be stored
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*}
 */
esp_err_t datablk_malloc(DataBlk *ppdblk, size_t data_size, uint32_t wait_ms);

/***
 * @description : recyle data block
 * @param        {DataBlk} pdblk - pinter to a data block
 * @return       {*}
 */
esp_err_t datablk_free(DataBlk pdblk);

/***
 * @description : init a data block
 * @param        {DataBlk} pdblk - pinter to a data block
 * @return       {*}
 */
void datablk_init(DataBlk pdblk);

/***
 * @description : increase reference of a data block
 * @param        {DataBlk} pdblk - pointer to a data block
 * @return       {*}
 */
void datablk_ref(DataBlk pdblk);

/***
 * @description : read from a data block to a buffer
 * @param        {DataBlk} pdblk - pointer to a data block
 * @param        {void} *buff - pointer to buffer
 * @param        {size_t} *psize - pointer to size to be readed
 * @return       {*} bytes readed in psize
 */
esp_err_t datablk_read(DataBlk pdblk, void *buff, size_t *psize);

/***
 * @description : write to a data block from a buffer
 * @param        {DataBlk} pdblk - pointer to a data block
 * @param        {void} *buff - pointer to buffer
 * @param        {size_t} *psize - pointer to size to be wrote
 * @return       {*} bytes wrote in psize
 */
esp_err_t datablk_write(DataBlk pdblk, void *buff, size_t *psize);

#ifdef __cplusplus
}
#endif

#endif /* _DATA_BLOCK_H_ */
