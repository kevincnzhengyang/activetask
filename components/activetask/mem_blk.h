/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-22 10:37:52
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-22 10:37:53
 * @FilePath    : /active_task/include/mem_blk.h
 * @Description : memory block in lock less
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _MEM_BLOCK_H_
#define _MEM_BLOCK_H_

#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "inner_err.h"

#ifndef MEMBLK_MIN_SIZE
#define MEMBLK_MIN_SIZE    8
#endif /* MEMBLK_MIN_SIZE */

#ifndef MEMBLK_NUM
#define MEMBLK_NUM     32
#endif /* MEMBLK_NUM */

#ifndef MEMBLK_STACK_NUM
#define MEMBLK_STACK_NUM    3
#endif /* MEMBLK_STACK_NUM */

#ifdef __cplusplus
extern "C" {
#endif

/***
 * @description : malloc a memory block
 * @param        {int} size - size of memory block
 * @return       {*} pointer to data buffer
 */
void *memblk_malloc(int size);

/***
 * @description : recyle a memory block
 * @param        {void} *arg - pointer to memory block
 * @return       {*}
 */
void memblk_free(void *arg);

/***
 * @description : set the flag of memory block to valid
 * @param        {void} *arg
 * @return       {*}
 */
void memblk_set_valid(void *arg);

/***
 * @description : check if memory block is valid
 * @param        {void} *arg
 * @return       {*}
 */
bool memblk_is_valid(void *arg);

/***
 * @description : init memory block pool
 * @return       {*}
 */
at_error_t memblk_pool_init(void);

/***
 * @description : clear memory block pool
 * @return       {*}
 */
void memblk_pool_fini(void);

#ifdef __cplusplus
}
#endif

#endif /* _MEM_BLOCK_H_ */
