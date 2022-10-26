/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-23 23:27:42
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-23 23:27:42
 * @FilePath    : /active_task/include/msg_blk.h
 * @Description : message block
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _MESSAGE_BLOCK_H_
#define _MESSAGE_BLOCK_H_

#include "inner_err.h"
#include "linux_macros.h"
#include "linux_list.h"
#include "circ_queue.h"
#include "data_blk.h"

#ifndef MSGBLK_NUM
#define MSGBLK_NUM     8
#endif /* MSGBLK_NUM */

struct msgblk_t {
    int                       msg_type;
    struct list_head      list_datablk;     // message datablock list
};

#define SIZE_MSG_BLOCK_HEADER  sizeof(struct msgblk_t)

typedef struct msgblk_t msgblk;

/***
 * @description : callback function after msgblk malloc success
 * @param        {msgblk} *mb - pointer to msg block
 * @param        {void} *arg - user defined parameter
 * @return       {*} - not INNER_RES_OK result lead to malloc failed with NULL
 */
typedef at_error_t (*on_msgblk_init)(msgblk *mb, void *arg);

/***
 * @description : callback function before msgblk release
 * @param        {msgblk} *mb - pointer to msg block
 * @param        {void} *arg - user defined parameter
 * @return       {*}
 */
typedef void (*on_msgblk_fini)(msgblk *mb, void *arg);

/***
 * @description : callback function after attach datablk
 * @param        {msgblk} *mb - pointer to msg block
 * @param        {datablk} *db - pointer to msg block
 * @param        {void} *arg - user defined parameter
 * @return       {*} - not INNER_RES_OK result lead to malloc failed to attach
 */
typedef at_error_t (*on_datablk_attach)(msgblk *mb, datablk *db, void *arg);

/***
 * @description : callback function before dettach datablk
 * @param        {msgblk} *mb - pointer to msg block
 * @param        {datablk} *db - pointer to msg block
 * @param        {void} *arg - user defined parameter
 * @return       {*}
 */
typedef void (*on_datablk_dettach)(msgblk *mb, datablk *db, void *arg);

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
        on_datablk_attach attch_func, on_datablk_dettach dettach_func, void *arg);

/***
 * @description : clear msg block pool
 * @return       {*}
 */
void msgblk_pool_fini(void);

/***
 * @description : malloc msg block with a data block attached
 * @param        {msgblk} *db - pointer to data block attached
 * @return       {*} - pointer to msg block, got NULL if failed
 */
msgblk * msgblk_malloc(datablk *db);

/***
 * @description : clear msg block, decrease reference
 *                  msg block would be release if reference = 0
 * @param        {msgblk} *mb - pointer to msg block
 * @return       {*}
 */
void msgblk_free(msgblk *mb);

/***
 * @description : set valid flag of msg block
 * @param        {msgblk} *mb - pointer to msg block
 * @return       {*}
 */
void msgblk_set_valid(msgblk *mb);

/***
 * @description : check valida flag
 * @param        {msgblk} *mb - pointer to msg block
 * @return       {*}
 */
bool msgblk_is_valid(msgblk *mb);

/***
 * @description : increase reference of msg block
 * @param        {msgblk} *mb - pointer to msg block
 * @return       {*}
 */
void msgblk_ref(msgblk *mb);

/***
 * @description : attach data block to msg block
 * @param        {msgblk} *mb - pointer to msg block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
at_error_t msgblk_attach_datablk(msgblk *mb, datablk *db);

/***
 * @description : dettach data block from msg block
 * @param        {msgblk} *mb - pointer to msg block
 * @param        {datablk} *db - pointer to data block
 * @return       {*}
 */
void msgblk_dettach_datablk(msgblk *mb, datablk *db);

/***
 * @description : push msg block in a circular queue
 * @param        {circ_queue} *queue - pointer to queue
 * @param        {msgblk} *mb - poiner to msg block
 * @param        {int} wait_ms - wait time in ms
 * @return       {*}
 */
at_error_t msgblk_push_circ_queue(circ_queue *queue, msgblk *mb, int wait_ms);

/***
 * @description : pop msg block from a circular queue
 * @param        {circ_queue} *queue - pointer to queue
 * @param        {msgblk} **pmb - pointer to msg block pointer
 * @param        {int} wait_ms - wait time in ms
 * @return       {*}
 */
at_error_t msgblk_pop_circ_queue(circ_queue *queue, msgblk **pmb, int wait_ms);

/***
 * @description : get first data block from message block
 * @param        {msgblk} *mb - pointer to message block
 * @return       {*}
 */
datablk *msgblk_first_datablk(msgblk *mb);

#endif /* _MESSAGE_BLOCK_H_ */

