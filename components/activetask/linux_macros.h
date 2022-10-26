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

#include <stdio.h>

#if defined(__linux__) || defined(__linux)
#include <stdlib.h>

#define INNER_RES_OK                0

#define KRNL_DEBUG(...) do {fprintf(stderr, __VA_ARGS__);} while(0)
#define KRNL_INFO(...) do {fprintf(stderr, __VA_ARGS__);} while(0)
#define KRNL_WARN(...) do {fprintf(stderr, __VA_ARGS__);} while(0)
#define KRNL_ERROR(...) do {fprintf(stderr, __VA_ARGS__);} while(0)

#elif defined(CONFIG_FreeRTOS)

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define INNER_RES_OK                ESP_OK

#define KRNL_TAG "KERNEL"
#define KRNL_DEBUG(fmt, ...)  ESP_LOGD(KRNL_TAG, fmt, ##__VA_ARGS__)
#define KRNL_INFO(fmt, ...)   ESP_LOGI(KRNL_TAG, fmt, ##__VA_ARGS__)
#define KRNL_WARN(fmt, ...)   ESP_LOGW(KRNL_TAG, fmt, ##__VA_ARGS__)
#define KRNL_ERROR(fmt, ...)  ESP_LOGE(KRNL_TAG, fmt, ##__VA_ARGS__)
#endif /* _ESP_PLATFORM */

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

#define typeof_member(T, m)	typeof(((T*)0)->m)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	((type *)(__mptr - offsetof(type, member))); })

/*
#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	static_assert(SAME_TYPE(*(ptr), ((type *)0)->member) ||	\
		      SAME_TYPE(*(ptr), void),			\
		      "pointer type mismatch in container_of()");	\
	((type *)(__mptr - offsetof(type, member))); })
*/

#ifndef __SIZEOF_L1_CACHE__
#define __SIZEOF_L1_CACHE__     64
#endif /* __SIZEOF_L1_CACHE__ */

#define __Pad1Size__(type) \
    ((__SIZEOF_L1_CACHE__ - sizeof(type))/sizeof(char))

#define __Pad2Size__(type1, type2) \
    ((__SIZEOF_L1_CACHE__ - sizeof(type1) - sizeof(type2))/sizeof(char))


#define NO_LESS_THAN(val, limit) ((val) < (limit) ? (limit) : (val))

#define NO_MORE_THAN(val, limit) ((val) > (limit) ? (limit) : (val))

#if defined(__linux__) || defined(__linux)
#include <stdlib.h>
#include <sys/time.h>

#define delay_ms(ms)    usleep((ms)*1000)

#ifdef __cplusplus
extern "C" {
#endif

static inline unsigned long get_sys_ms(void)
{
    struct timeval start;
    gettimeofday(&start, NULL);
    return (unsigned long) (start.tv_sec*1000+start.tv_usec/1000);
};

#ifdef __cplusplus
}
#endif

#elif defined(CONFIG_FreeRTOS)

#define delay_ms(ms)    vTaskDelay(pdMS_TO_TICKS(ms))
#define get_sys_ms()    pdTICKS_TO_MS(xTaskGetTickCount())
#endif /* _ESP_PLATFORM */

#endif /* _LINUX_MACROS_H_ */
