/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 10:32:30
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-17 10:32:30
 * @FilePath    : /activetask/components/activetask/active_task.h
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _ACTIVE_TASK_H_
#define _ACTIVE_TASK_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "inner_err.h"
#include "linux_refcount.h"
#include "linux_hlist.h"
#include "msg_blk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ActiveTask_Stru  ActiveTask;

struct ActiveTask_Stru {
    char                         *name;
    uint32_t               stack_depth;     // stack depth of freeRTOS task
    UBaseType_t               priority;     // priority
    TaskHandle_t          task_handler;     // FreeRTOS task handler
    BaseType_t                 core_id;     // cpu core id
    uint32_t                 interv_ms;
    uint32_t               schedule_ms;
    uint32_t            last_scheduled;

    QueueHandle_t            msg_queue;
    size_t               msg_queue_len;

    struct hlist_node        node_task;     // node for task hash table

    ActiveTask              *next_task;     // next task in streamly processing
    void                     *app_data;     // reserved for app

    /***
     * @description : start running of an Active Task
     * @param        {ActiveTask} *task - pointer to Active Task
     * @return       {*}
     */
    esp_err_t (*task_begin)(ActiveTask *task);

    /***
     * @description : main loop of an Active Task
     * @param        {ActiveTask} *task - pointer to Active Task
     * @return       {*}
     */
    esp_err_t (*task_svc)(ActiveTask *task);

    /***
     * @description : put a message block into a queue of an Active Task
     * @param        {ActiveTask} *task - pointer to Active Task
     * @param        {MsgBlk} mblk - pointer to message block
     * @param        {uint32_t} wait_ms - wait time in ms
     * @return       {*}
     */
    esp_err_t (*put_message)(ActiveTask *task, MsgBlk mblk, uint32_t wait_ms);

    /***
     * @description : put a message block into a queue of next Active Task
     * @param        {ActiveTask} *task - pointer to Active Task
     * @param        {MsgBlk} mblk - pointer to message block
     * @param        {uint32_t} wait_ms - wait time in ms
     * @return       {*}
     */
    esp_err_t (*put_message_next)(ActiveTask *task, MsgBlk mblk, uint32_t wait_ms);

    /***
     * @description : callback when an Acitve Task before running
     * @param        {ActiveTask} *task - pointer to Active Task
     * @return       {*}
     */
    esp_err_t (*on_init)(ActiveTask *task);

    /***
     * @description : callback when an Acitve Task running
     * @param        {ActiveTask} *task - pointer to Active Task
     * @return       {*} -
     */
    esp_err_t (*on_loop)(ActiveTask *task);

    /***
     * @description : callback when a message block got from the queue
     * @param        {ActiveTask} *task - pointer to Active Task
     * @param        {MsgBlk} mblk - pointer to message block
     * @return       {*}
     */
    esp_err_t (*on_message)(ActiveTask *task, MsgBlk mblk);

    /***
     * @description : callback when scheduled timeout occurs
     * @param        {ActiveTask} *task - pointer to Active Task
     * @return       {*}
     */
    esp_err_t (*on_schedule)(ActiveTask *task);
};

#define SIZE_ACTIVE_TASK    sizeof(struct ActiveTask_Stru)

/***
 * @description : init Active Task poo
 * @param        {size_t} bits - bits number of hash tabe
 * @return       {*}
 */
void active_task_pool_init(size_t bits);

/***
 * @description : create a task instantce
 * @param        {char *} name - name of the task
 * @param        {uint32_t} stack - stack size of FreeRTOS task
 * @param        {UBaseType_t} priority - prority of FreeRTOS task
 * @param        {BaseType_t} core - cpu affinity
 * @param        {size_t} queue_len - length of the message queue
 * @param        {uint32_t} interval - interva time in ms for receiving, 0 means blocked
 * @param        {uint32_t} schedule - schedule time in ms
 * @param        {void} *data - user defined data
 * @return       {*} - pointer to an Active Task
 */
ActiveTask *active_task_create(const char * name, uint32_t stack,
        UBaseType_t priority, BaseType_t core ,
        size_t queue_len, uint32_t interval,
        uint32_t schedule, void *data);


/***
 * @description : find an instance of Active Task by name
 * @param        {char} *name - name of desired Active Task
 * @return       {*} - pointer to an Active Task
 */
ActiveTask *active_task_find(const  char *name);

/***
 * @description : send a message block to an Active Task iwth name
 * @param        {char} *name - name of Active Task which supposed to receive
 * @param        {MsgBlk} mblk - pinter to a message block
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*} - result of send
 */
esp_err_t  active_task_send_to(const char *name, MsgBlk mblk, uint32_t wait_ms);

#ifdef __cplusplus
}
#endif

#endif /* _ACTIVE_TASK_H_ */
