/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-24 17:29:15
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-24 17:29:15
 * @FilePath    : /active_task/include/active_task.h
 * @Description : active task
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _ACTIVE_TASK_H_
#define _ACTIVE_TASK_H_
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#if defined(__linux__) || defined(__linux)
#include <pthread.h>
#elif defined(CONFIG_FreeRTOS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif /* _ESP_PLATFORM */

#include "inner_err.h"
#include "circ_queue.h"
#include "msg_blk.h"

typedef struct active_task_t active_task;

struct active_task_t {
    char                         *name;
    int                    stack_depth;     // stack depth of freeRTOS task
    int                       priority;     // priority
#if defined(__linux__) || defined(__linux)
    pthread_t             task_handler;
#elif defined(CONFIG_FreeRTOS)
    TaskHandle_t          task_handler;     // task handler
#endif /* _ESP_PLATFORM */
    int                        core_id;     // cpu core id
    int                      interv_ms;
    int                    schedule_ms;
    int                 last_scheduled;

    circ_queue                  *queue;
    int                   queue_length;
    active_task             *next_task;     // next task in streamly processing
    void                     *app_data;     // reserved for app


    /***
     * @description : start running of an active task
     * @param        {active_task} *task - pointer to active task
     * @return       {*}
     */
    at_error_t (*task_begin)(active_task *task);

    /***
     * @description : main loop of an active task
     * @param        {active_task} *task - pointer to active task
     * @return       {*}
     */
    at_error_t (*task_svc)(active_task *task);

    /***
     * @description : put a message block into a queue of an active task
     * @param        {active_task} *task - pointer to active task
     * @param        {msgblk} *mblk - pointer to message block
     * @param        {int} wait_ms - wait time in ms
     * @return       {*}
     */
    at_error_t (*put_message)(active_task *task, msgblk *mblk, int wait_ms);

    /***
     * @description : put a message block into a queue of next active task
     * @param        {active_task} *task - pointer to active task
     * @param        {msgblk} *mblk - pointer to message block
     * @param        {int} wait_ms - wait time in ms
     * @return       {*}
     */
    at_error_t (*put_message_next)(active_task *task, msgblk *mblk, int wait_ms);

    /***
     * @description : callback when an Acitve Task before running
     * @param        {active_task} *task - pointer to active task
     * @return       {*}
     */
    at_error_t (*on_init)(active_task *task);

    /***
     * @description : callback when an Acitve Task running
     * @param        {active_task} *task - pointer to active task
     * @return       {*} -
     */
    at_error_t (*on_loop)(active_task *task);

    /***
     * @description : callback when a message block got from the queue
     * @param        {active_task} *task - pointer to active task
     * @param        {msgblk} *mblk - pointer to message block
     * @return       {*}
     */
    at_error_t (*on_message)(active_task *task, msgblk *mblk);

    /***
     * @description : callback when scheduled timeout occurs
     * @param        {active_task} *task - pointer to active task
     * @return       {*}
     */
    at_error_t (*on_schedule)(active_task *task);
};

#define SIZE_ACTIVE_TASK    sizeof(struct active_task_t)

/***
 * @description : create an active task
 * @param        {char} *name - name of active task
 * @param        {int} stack - stack of task
 * @param        {int} priority - priority
 * @param        {int} core - cpu affinity
 * @param        {size_t} queue_len - length of queue
 * @param        {int} interval - interva time in ms for receiving, 0 means blocked
 * @param        {int} schedule - schedule time in ms
 * @param        {void} *app_data - application data
 * @return       {*}
 */
active_task *active_task_create(const char *name, int stack,
        int priority, int core, size_t queue_len, int interval,
        int schedule, void *app_data);

/***
 * @description : delete an active task
 * @param        {active_task} *task - pointer to active task
 * @return       {*}
 */
void active_task_delete(active_task *task);

#endif /* _ACTIVE_TASK_H_ */