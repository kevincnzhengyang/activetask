/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 21:49:52
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-17 21:49:52
 * @FilePath    : /activetask/components/activetask/inner_err.h
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _INNER_ERROR_H_
#define _INNER_ERROR_H_

#include "esp_err.h"

#define INNER_ERR_BASE              0x400000

#define INNER_MSG_ERR_BASE          0x410000
#define INNER_INVAILD_PARAM         (INNER_MSG_ERR_BASE+ 1)
#define INNER_ITEM_NOT_FOUND        (INNER_MSG_ERR_BASE+ 1)
#define MALLOC_WAIT_TIMEOUT         (INNER_MSG_ERR_BASE+ 1)
#define MSGBLK_FAILED_MALLOC        (INNER_MSG_ERR_BASE+ 2)
#define MSGBLK_FAILED_PUSH          (INNER_MSG_ERR_BASE+ 2)
#define MSGBLK_FAILED_POP           (INNER_MSG_ERR_BASE+ 2)
#define DATABLK_FAILED_MALLOC       (INNER_MSG_ERR_BASE+ 2)
#define TASK_FAILED_QUEUE           (INNER_MSG_ERR_BASE+ 2)
#define TASK_SVC_CONTINUE           (INNER_MSG_ERR_BASE+ 2)
#define TASK_SVC_BREAK              (INNER_MSG_ERR_BASE+ 2)
#define TASK_NOT_EXIST              (INNER_MSG_ERR_BASE+ 2)
#define TASK_NEXT_NOT_EXIST         (INNER_MSG_ERR_BASE+ 2)

#endif /* _INNER_ERROR_H_ */