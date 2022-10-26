/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 13:56:17
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-17 13:56:17
 * @FilePath    : /activetask/components/activetask/linux_circ.h
 * @Description : https://github.com/torvalds/linux/blob/master/include/linux/circ_buf.h
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _LINUX_CIRCUAR_BUFF_H_
#define _LINUX_CIRCUAR_BUFF_H_

#include <stddef.h>
#include <string.h>
#include <stdatomic.h>

#include "linux_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline int get_powerof2(int i)
{
    int temp = i - 1;
    temp |= temp >> 1;
    temp |= temp >> 2;
    temp |= temp >> 4;
    temp |= temp >> 8;
    temp |= temp >> 16;
    return temp < 0 ? 1 : temp + 1;
};

struct circ_buf {
	atomic_int                    head;     // changing
    char   pad_head[__Pad1Size__(int)];
	atomic_int                    tail;     // changing
    char   pad_tail[__Pad1Size__(int)];
    size_t                        size;     // fixed after init
	void                          *buf;     // fixed after init
    char pads[__Pad2Size__(size_t, char *)];
};

// size : 1 << N - 1
#define CIRC_INIT(bits) {\
    .head = ATOMIC_VAR_INIT(0), \
    .tail = ATOMIC_VAR_INIT(0), \
    .size = (1 << (bits)) - 1, \
    .buf = NULL, \
}

/* Return count in buffer.  */
#define __CIRC_CNT(head,tail,size) (((head) - (tail)) & (size))

#define CIRC_CNT(pbuff) \
    __CIRC_CNT((pbuff)->head, (pbuff)->tail, (pbuff)->size)

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(pbuff) \
    __CIRC_CNT((pbuff)->tail, (pbuff)->head+1, (pbuff)->size+1)

/* Return count up to the end of the buffer.  Carefully avoid
   accessing head and tail more than once, so they can change
   underneath us without returning inconsistent results.  */
#define CIRC_CNT_TO_END(pbuff) \
	({int end = (pbuff)->size + 1 - (pbuff)->tail; \
	  int n = ((pbuff)->head + end) & (pbuff)->size; \
	  n < end ? n : end;})

/* Return space available up to the end of the buffer.  */
#define CIRC_SPACE_TO_END(pbuff) \
	({int end = (pbuff)->size - (pbuff)->head; \
	  int n = (end + (pbuff)->tail) & (pbuff)->size; \
	  n <= end ? n : end+1;})

#define CIRC_IS_EMPTY(pbuff) \
    (atomic_load(&(pbuff)->head)  == atomic_load(&(pbuff)->tail))

#define CIRC_IS_FULL(pbuff) \
    (atomic_load(&(pbuff)->head) + (pbuff)->size - 1 < atomic_load(&(pbuff)->tail))

/* return head index available before adding one atomatically */
#define CIRC_READ_ONE(pbuff)  \
    (CIRC_IS_EMPTY(pbuff) ? -1 : atomic_fetch_add(&(pbuff)->head, 1))

/* return tail index available before adding one atomatically */
#define CIRC_WRITE_ONE(pbuff)  \
    (CIRC_IS_FULL(pbuff) ? -1 : atomic_fetch_add(&(pbuff)->tail, 1))

/* get index of array from position */
#define CIRC_GET_INDEX(pbuff, pos)  ((pos) & (pbuff)->size)

#ifdef __cplusplus
}
#endif

#endif