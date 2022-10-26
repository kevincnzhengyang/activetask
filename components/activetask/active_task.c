/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-24 17:29:26
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-26 22:51:07
 * @FilePath    : /activetask/components/activetask/active_task.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__) || defined(__linux)
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sched.h>
#include <ctype.h>
#endif /* __linux__ */

#include "linux_macros.h"
#include "active_task.h"

#if defined(__linux__) || defined(__linux)
static void *dft_task_func(void *param)
#elif defined(CONFIG_FreeRTOS)
static void dft_task_func(void *param)
#endif /* _ESP_PLATFORM */
{
    active_task *task = (active_task *)param;
    KRNL_DEBUG("task %s ready to run\n", task->name);
    task->task_svc(task);
    #if defined(__linux__) || defined(__linux)
    pthread_exit(NULL);
    return param;
    #elif defined(CONFIG_FreeRTOS)
    vTaskDelete(NULL);
    #endif /* _ESP_PLATFORM */
}

/***
 * @description : start running of an active task
 * @param        {active_task} *task - pointer to active task
 * @return       {*}
 */
at_error_t dft_task_begin(active_task *task)
{
    if (NULL == task) return INNER_INVAILD_PARAM;
    KRNL_DEBUG("task %s begin\n", task->name);

    // create queue in need
    if (task->queue_length > 0) {
        if (NULL == (task->queue = circ_queue_create(task->queue_length))) {
            KRNL_ERROR("task %s failed to init queue\n", task->name);
            return TASK_FAILED_QUEUE;
        }
    }

    // on_init
    if (NULL != task->on_init) {
        at_error_t res;
        if (INNER_RES_OK != (res = task->on_init(task))) {
            KRNL_ERROR("task %s exit due to init\n", task->name);
            return res;
        }
        KRNL_ERROR("task %s on_init ok\n", task->name);
    }

    // reset last scheduled
    task->last_scheduled = get_sys_ms();

#if defined(__linux__) || defined(__linux)
    // TODO
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    struct sched_param s_param = {.sched_priority = task->priority};
    pthread_attr_setschedparam(&attr, &s_param); // priority
    pthread_attr_setstacksize(&attr, (size_t)task->stack_depth); // stack size
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);  //detached
    pthread_create(&task->task_handler, &attr, dft_task_func, (void *)task);

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(task->core_id, &mask);
    pthread_setaffinity_np(task->task_handler, sizeof(mask), &mask);  //cpu core

    pthread_detach(task->task_handler);
#elif defined(CONFIG_FreeRTOS)
    // FreeRTOS task
    BaseType_t result = xTaskCreatePinnedToCore(dft_task_func, task->name,
            task->stack_depth, (void *)task, task->priority,
            &task->task_handler, task->core_id);
    assert("Failed to create task" && result == pdPASS);
#endif
    return INNER_RES_OK;
}

/***
 * @description : main loop of an active task
 * @param        {active_task} *task - pointer to active task
 * @return       {*}
 */
at_error_t dft_task_svc(active_task *task)
{
    if (NULL == task) return INNER_INVAILD_PARAM;
    KRNL_DEBUG("task %s running...\n", task->name);

    at_error_t ret = -1;
    msgblk *mblk = NULL;
    unsigned long now = 0;

    while (true) {
        // on_loop
        if (NULL != task->on_loop) {
            KRNL_DEBUG("task %s call on_loop\n", task->name);
            ret = task->on_loop(task);
            if (TASK_SVC_CONTINUE == ret) continue;
            else if (TASK_SVC_BREAK == ret) break;
        }

        // receive message
        if (NULL != task->queue && INNER_RES_OK == msgblk_pop_circ_queue(
                task->queue, &mblk, task->interv_ms)) {
            // on_message
            if (NULL != task->on_message) {
                KRNL_DEBUG("task %s call on_message %p\n", task->name, mblk);
                ret = task->on_message(task, mblk);
                msgblk_free(mblk);   // decrease refer
                if (TASK_SVC_CONTINUE == ret) continue;
                else if (TASK_SVC_BREAK == ret) break;
            } else {
                // drop the message
                KRNL_DEBUG("task %s drop message %p\n", task->name, mblk);
                msgblk_free(mblk);   // decrease refer
            }
        }

        // on schedule
        now = get_sys_ms();
        if (0 < task->schedule_ms \
            && NULL != task->on_schedule \
            && (now - task->last_scheduled) > task->schedule_ms) {
                KRNL_DEBUG("task %s call on_schedule\n", task->name);
                task->last_scheduled = now;
                ret = task->on_schedule(task);
                if (TASK_SVC_CONTINUE == ret) continue;
                else if (TASK_SVC_BREAK == ret) break;
            }
    }
    KRNL_DEBUG("task %s svc end\n", task->name);
    return INNER_RES_OK;
}

/***
 * @description : put a message block into a queue of an active task
 * @param        {active_task} *task - pointer to active task
 * @param        {msgblk} *mblk - pointer to message block
 * @param        {int} wait_ms - wait time in ms
 * @return       {*}
 */
at_error_t dft_put_message(active_task *task, msgblk *mblk, int wait_ms)
{
    if (NULL == task || NULL == mblk || NULL == task->queue)
        return INNER_INVAILD_PARAM;
    KRNL_DEBUG("task %s put message %p\n", task->name, mblk);
    return msgblk_push_circ_queue(task->queue, mblk, wait_ms);
}

/***
 * @description : put a message block into a queue of next active task
 * @param        {active_task} *task - pointer to active task
 * @param        {msgblk} *mblk - pointer to message block
 * @param        {int} wait_ms - wait time in ms
 * @return       {*}
 */
at_error_t dft_put_message_next(active_task *task, msgblk *mblk, int wait_ms)
{
    if (NULL == task || NULL == mblk) return INNER_INVAILD_PARAM;
    if (NULL == task->next_task || NULL == task->next_task->queue)
        return TASK_NEXT_NOT_EXIST;
    KRNL_DEBUG("task %s put_next message %p\n", task->name, mblk);
    return msgblk_push_circ_queue(task->next_task->queue, mblk, wait_ms);
}

/***
 * @description : create a task instantce
 * @param        {char *} name - name of the task
 * @param        {uint32_t} stack - stack size of FreeRTOS task
 * @param        {UBaseType_t} priority - prority of FreeRTOS task
 * @param        {BaseType_t} core - cpu affinity
 * @param        {size_t} queue_length - length of the message queue
 * @param        {uint32_t} interval - interva time in ms for receiving, 0 means blocked
 * @param        {uint32_t} schedule - schedule time in ms
 * @param        {void} *data - user defined data
 * @return       {*} - pointer to an active task
 */
active_task *active_task_create(const char *name, int stack,
        int priority, int core, size_t queue_len, int interval,
        int schedule, void *app_data)
{
    active_task *task = (active_task *)malloc(SIZE_ACTIVE_TASK);
    if (NULL == task) {
        KRNL_ERROR("Failed to malloc for task");
        return NULL;
    }
    task->name = strdup(name);
    task->stack_depth = stack;
    task->priority = priority;
    task->core_id = core;
    task->interv_ms = interval;
    task->schedule_ms = schedule;
    task->last_scheduled = 0;
    task->queue = NULL;
    task->queue_length = queue_len;
    task->next_task = NULL;
    task->app_data = app_data;
    task->task_begin = dft_task_begin;
    task->task_svc = dft_task_svc;
    task->put_message = dft_put_message;
    task->put_message_next = dft_put_message_next;
    task->on_init = NULL;
    task->on_loop = NULL;
    task->on_message = NULL;
    task->on_schedule = NULL;
    KRNL_DEBUG("task %s created\n", task->name);
    return task;
}

/***
 * @description : delete an active task
 * @param        {active_task} *task - pointer to active task
 * @return       {*}
 */
void active_task_delete(active_task *task)
{
    if (NULL == task) return;
    // TODO stop thread
    if (NULL != task->queue) circ_queue_delete(task->queue);
    if (NULL != task->name) free(task->name);
    free(task);
}
