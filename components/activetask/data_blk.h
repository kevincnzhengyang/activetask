/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-23 10:37:42
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-23 10:37:42
 * @FilePath    : /active_task/include/data_blk.h
 * @Description : data block
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _DATA_BLOCK_H_
#define _DATA_BLOCK_H_

#include "inner_err.h"
#include "linux_macros.h"
#include "linux_list.h"

#ifndef DATABLK_MIN_SIZE
#define DATABLK_MIN_SIZE    32
#endif /* DATABLK_MIN_SIZE */

#ifndef DATABLK_NUM
#define DATABLK_NUM     8
#endif /* DATABLK_NUM */

#ifndef DATABLK_STACK_NUM
#define DATABLK_STACK_NUM    3
#endif /* DATABLK_STACK_NUM */

#ifdef __cplusplus
extern "C" {
#endif

struct datablk_t {
    int                      data_type;
    void                       *rd_ptr;
    void                       *wr_ptr;
    struct list_head      node_msgdata;     // node for message block
};

#define SIZE_DATA_BLOCK_HEADER  sizeof(struct datablk_t)

typedef struct datablk_t datablk;

/*
 * get the length of data to be handled, bytes between write and read pointer
 */
#define datablk_length(db)  ((db)->wr_ptr - (db)->rd_ptr)

/*
 * get the length of data ignored, bytes before read pointer
 */
#define datablk_ignored(db)  ((db)->rd_ptr - datablk_get_base(db))

/*
 * get the length of data ocupied, bytes before write pointer
 */
#define datablk_ocupied(db)   ((db)->wr_ptr - datablk_get_base(db))

/*
 * get the length of space, bytes after write pinter
 */
#define datablk_space(db)  (datablk_get_size(db) - datablk_ocupied(db))

/***
 * @description : callback function after datablk malloc success
 * @param        {datablk} *db - pointer to data block
 * @param        {void} *arg - user defined parameter
 * @return       {*} - not INNER_RES_OK result lead to malloc failed with NULL
 */
typedef at_error_t (*on_datablk_init)(datablk *db, void *arg);

/***
 * @description : callback function before datablk release
 * @param        {datablk} *db - pointer to data block
 * @param        {void} *arg - user defined parameter
 * @return       {*}
 */
typedef void (*on_datablk_fini)(datablk *db, void *arg);

/***
 * @description : init data block pool
 * @param        {on_datablk_init} init_func - callback function after data block malloc
 * @param        {on_datablk_fini} fini_func - callback function before data block release
 * @param        {void} *arg - user defined parameter for callback function
 * @return       {*}
 */
at_error_t datablk_pool_init(on_datablk_init init_func, on_datablk_fini fini_func, void *arg);

/***
 * @description : clear data block pool
 * @return       {*}
 */
void datablk_pool_fini(void);

/***
 * @description : malloc data block
 * @param        {int} size - desired size of data block
 * @return       {*} - pointer to data block, got NULL if failed
 */
datablk * datablk_malloc(int size);

/***
 * @description : clear data block, decrease reference
 *                  data block would be release if reference = 0
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void datablk_free(datablk *db);

/***
 * @description : set valid flag of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void datablk_set_valid(datablk *db);

/***
 * @description : check valida flag
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
bool datablk_is_valid(datablk *db);

/***
 * @description : increase reference of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void datablk_ref(datablk *db);

/***
 * @description : get capacity of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
int datablk_get_cap(datablk *db);

/***
 * @description : get size of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
int datablk_get_size(datablk *db);

/***
 * @description : get base address of data block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void *datablk_get_base(datablk *db);

/***
 * @description : move read ptr with n bytes
 * @param        {datablk} *db - pointer to data block
 * @param        {int} nbytes - bytes
 * @return       {*} - return length if n > length
 */
int datablk_move_rd(datablk *db, int nbytes);

/***
 * @description : move write ptr with n bytes
 * @param        {datablk} *db - pointer to data block
 * @param        {int} nbytes - bytes
 * @return       {*} - return space if n > space
 */
int datablk_move_wr(datablk *db, int nbytes);

#ifdef __cplusplus
}
#endif

#endif /* _DATA_BLOCK_H_ */
