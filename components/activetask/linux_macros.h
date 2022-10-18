/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 11:59:58
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-17 11:59:58
 * @FilePath    : /activetask/components/activetask/linux_macros.h
 * @Description : some macros from linux kernel
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _LINUX_MACROS_H_
#define _LINUX_MACROS_H_

#include "esp_log.h"
#include "esp_err.h"

#define KRNL_TAG "App"
#define KRNL_DEBUG(fmt, ...)  ESP_LOGD(KRNL_TAG, fmt, ##__VA_ARGS__)
#define KRNL_INFO(fmt, ...)   ESP_LOGI(KRNL_TAG, fmt, ##__VA_ARGS__)
#define KRNL_WARN(fmt, ...)   ESP_LOGW(KRNL_TAG, fmt, ##__VA_ARGS__)
#define KRNL_ERROR(fmt, ...)  ESP_LOGE(KRNL_TAG, fmt, ##__VA_ARGS__)

#define	EINVAL		22	/* Invalid argument */

/* Are two types/vars the same type (ignoring qualifiers)? */
#define SAME_TYPE(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct {int:(-!!(e)); }))

#define MUST_BE_ARRAY(a) BUILD_BUG_ON_ZERO(SAME_TYPE((a), &(a)[0]))

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]) + MUST_BE_ARRAY(arr))

#define MAX_ERRNO	4095

#define IS_ERR_VALUE(x) (unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO

#define ERR_PTR(e) ((void *)e)

#define PTR_ERR(p) ((long)p)

#define IS_ERR(p) IS_ERR_VALUE((unsigned long)p)

#define IS_ERR_OR_NULL(p) ((!(p)) || IS_ERR_VALUE((unsigned long)p))

#define ERR_CAST(p) ((void *)p)

#define PTR_ERR_OR_ZERO(p) (IS_ERR(p) ? PTR_ERR(p) : 0)

#endif /* _LINUX_MACROS_H_ */
