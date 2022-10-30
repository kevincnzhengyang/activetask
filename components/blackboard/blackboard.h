/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-29 18:29:41
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-29 18:29:41
 * @FilePath    : /activetask/components/backboard/blackboard.h
 * @Description : blackboard for data publish and store on chip
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _BLACKBOARD_H_
#define _BLACKBOARD_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "inner_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLACKBOARD_MAP_BITS     3

#define BB_ERR_BASE                 0x411000
#define BB_LACK_SPACE               (BB_ERR_BASE+ 1)
#define BB_KEY_NOT_EXIST            (BB_ERR_BASE+ 2)

/***
 * @description : init black board
 * @param        {size_t} buff_size - size of black board
 * @return       {*}
 */
void blackboard_init(size_t buff_size);

/***
 * @description : fini black board
 * @return       {*}
 */
void blackboard_fini(void);

/***
 * @description : get data from black board
 * @param        {char} *key - name of data
 * @return       {*}
 */
void *blackboard_get(const char *key);

/***
 * @description : register data into black board
 * @param        {char} *key - name of data
 * @param        {size_t} data_size - size of data
 * @param        {bool} persisted - if data is persisted
 * @return       {*}
 */
at_error_t blackboard_register(const char *key, size_t data_size, bool persisted);

/***
 * @description : flush black board into nvs
 * @param        {char} *key - name of data
 * @return       {*}
 */
at_error_t blackboard_flush(const char *key);

/**
 * get data from black board as specified type
 */
#define blackboard_get_as(key, T)  ((T *)blackboard_get(key))

/**
 * register a persist data into black board
 */
#define blackboard_persist(key, data_size) blackboard_register(key, data_size, true)

/**
 * register a temperary data into black board
 */
#define blackboard_temperary(key, data_size) blackboard_register(key, data_size, false)

#ifdef __cplusplus
}
#endif

#endif /* _BLACKBOARD_H_ */