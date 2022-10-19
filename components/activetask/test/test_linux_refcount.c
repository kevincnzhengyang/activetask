/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-18 23:11:28
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-18 23:19:53
 * @FilePath    : /activetask/components/activetask/test/test_linux_refcount.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "unity.h"

#include "linux_refcount.h"

struct my_data
{
    int                index;
    struct kref     refcount;
};

TEST_CASE("test reference count", "[kernel][function]")
{

}
