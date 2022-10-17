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

struct circ_buf {
    size_t                        size;
	char                          *buf;
	int                           head;
	int                           tail;
};

// size : 1 << N - 1

#define CIRC_INIT(bits) {\
    .size = 1 << (bits) - 1;
    .buf = (char *)malloc(1 << (bits)), \
    .head = 0, \
    .tail = 0, \
    \}

/* Return count in buffer.  */
#define __CIRC_CNT(head,tail,size) (((head) - (tail)) & (size))

#define CIRC_CNT(pbuff) __CIRC_CNT((pbuff)->head, (pbuff)->tail, (pbuff)->size+1)

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(pbuff) __CIRC_CNT((pbuff)->tail, (pbuff)->head+1, (pbuff)->size+1)

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

// TODO atomic acquire and release, read and write

#endif