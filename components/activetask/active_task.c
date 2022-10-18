/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-17 10:32:20
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-18 21:23:25
 * @FilePath    : /activetask/components/activetask/active_task.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */

#include "linux_hlist.h"
#include "active_task.h"

DEFINE_HASHTABLE(gTaskHashTable, 3);

static void dft_task_func(void *param)
{
    ActiveTask *task = (ActiveTask *)param;
    KRNL_DEBUG("task %s ready to run\n", task->name);
    task->task_svc(task);
    vTaskDelete(NULL);
}

/***
 * @description : start running of an Active Task
 * @param        {ActiveTask} *task - pointer to Active Task
 * @return       {*}
 */
esp_err_t dft_task_begin(ActiveTask *task)
{
    if (NULL == task) return INNER_INVAILD_PARAM;
    KRNL_DEBUG("task %s begin\n", task->name);

    // create queue in need
    if (task->msg_queue_len > 0) {
        if (ESP_OK != msgblk_queue_init(&task->msg_queue, task->msg_queue_len)) {
            KRNL_ERROR("task %s failed to init queue\n", task->name);
            return TASK_FAILED_QUEUE;
        }
    }

    // on_init
    if (NULL != task->on_init)
        if (ESP_OK != task->on_init(task)) {
            KRNL_ERROR("task %s exit due to init\n", task->name);
            return ESP_FAIL;
        }

    // reset last scheduled
    task->last_scheduled = xTaskGetTickCount();

    // FreeRTOS task
    BaseType_t result = xTaskCreatePinnedToCore(dft_task_func, task->name,
            task->stack_depth, (void *)task, task->priority,
            &task->task_handler, task->core_id);
    assert("Failed to create task" && result == pdPASS);
    return ESP_OK;
}

/***
 * @description : main loop of an Active Task
 * @param        {ActiveTask} *task - pointer to Active Task
 * @return       {*}
 */
esp_err_t dft_task_svc(ActiveTask *task)
{
    assert (NULL != task);
    KRNL_DEBUG("task %s running...\n", task->name);

    uint32_t timeout = (0 == task->interv_ms) \
            ? portMAX_DELAY : task->interv_ms;

    esp_err_t ret = ESP_FAIL;
    MsgBlk mblk = NULL;
    uint32_t now = 0;

    while (true) {
        // on_loop
        if (NULL != task->on_loop) {
            KRNL_DEBUG("task %s call on_loop\n", task->name);
            ret = task->on_loop(task);
            if (TASK_SVC_CONTINUE == ret) continue;
            else if (TASK_SVC_BREAK == ret) break;
        }

        // receive message
        if (ESP_OK == msgblk_pop_queue(task->msg_queue, &mblk, timeout)) {
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
        now = xTaskGetTickCount();
        if (0 < task->schedule_ms \
            && NULL != task->on_schedule \
            && pdTICKS_TO_MS(now - task->last_scheduled) > task->schedule_ms) {
                KRNL_DEBUG("task %s call on_schedule\n", task->name);
                task->last_scheduled = now;
                ret = task->on_schedule(task);
                if (TASK_SVC_CONTINUE == ret) continue;
                else if (TASK_SVC_BREAK == ret) break;
            }
    }
    return ESP_OK;
}

/***
 * @description : put a message block into a queue of an Active Task
 * @param        {ActiveTask} *task - pointer to Active Task
 * @param        {MsgBlk} mblk - pointer to message block
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*}
 */
esp_err_t dft_put_message(ActiveTask *task, MsgBlk mblk, uint32_t wait_ms)
{
    if (NULL == task || NULL == mblk || NULL == task->msg_queue)
        return INNER_INVAILD_PARAM;
    KRNL_DEBUG("task %s put message %p\n", task->name, mblk);
    return msgblk_push_queue(task->msg_queue, mblk, wait_ms);
}

/***
 * @description : put a message block into a queue of next Active Task
 * @param        {ActiveTask} *task - pointer to Active Task
 * @param        {MsgBlk} mblk - pointer to message block
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*}
 */
esp_err_t dft_put_message_next(ActiveTask *task, MsgBlk mblk, uint32_t wait_ms)
{
    if (NULL == task || NULL == mblk) return INNER_INVAILD_PARAM;
    if (NULL == task->next_task || NULL == task->next_task->msg_queue)
        return TASK_NEXT_NOT_EXIST;
    KRNL_DEBUG("task %s put_next message %p\n", task->name, mblk);
    return msgblk_push_queue(task->next_task->msg_queue, mblk, wait_ms);
}

/***
 * @description : init Active Task poo
 * @param        {size_t} bits - bits number of hash tabe
 * @return       {*}
 */
void active_task_pool_init(size_t bits)
{
    hash_init(gTaskHashTable);
    KRNL_DEBUG("task pool inited\n");
}

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
        UBaseType_t priority, BaseType_t core,
        size_t queue_len, uint32_t interval,
        uint32_t schedule, void *data)
{
    ActiveTask *task = (ActiveTask *)malloc(SIZE_ACTIVE_TASK);
    assert ("Failed to malloc for task" && NULL != task);
    task->name = strdup(name),
    task->stack_depth = stack,
    task->priority = priority,
    task->core_id = core,
    task->interv_ms = interval,
    task->schedule_ms = schedule,
    task->last_scheduled = 0,
    task->msg_queue = NULL,
    task->msg_queue_len = queue_len,
    task->next_task = NULL,
    task->app_data = data,
    task->task_begin = dft_task_begin,
    task->task_svc = dft_task_svc,
    task->put_message = dft_put_message,
    task->put_message_next = dft_put_message_next,
    task->on_init = NULL,
    task->on_loop = NULL,
    task->on_message = NULL,
    task->on_schedule = NULL,
    INIT_HLIST_NODE(&task->node_task);
    KRNL_DEBUG("task %s created\n", task->name);

    // add to hashtable
    hash_add(gTaskHashTable, &task->node_task,
            task->name, Bits_gTaskHashTable);

    return task;
}


/***
 * @description : find an instance of Active Task by name
 * @param        {char} *name - name of desired Active Task
 * @return       {*} - pointer to an Active Task
 */
ActiveTask *active_task_find(const  char *name)
{
    if (NULL == name) return NULL;
    KRNL_DEBUG("task with name %s finding...\n", name);
    ActiveTask *task = NULL;
    hash_for_each_possible(gTaskHashTable, task,
            node_task, name, Bits_gTaskHashTable) {
        if (0 == strcmp(task->name, name)) return task;
    }
    KRNL_DEBUG("task with name %s not existed\n", name);
    return NULL;
}

/***
 * @description : send a message block to an Active Task iwth name
 * @param        {char} *name - name of Active Task which supposed to receive
 * @param        {MsgBlk} mblk - pinter to a message block
 * @param        {uint32_t} wait_ms - wait time in ms
 * @return       {*} - result of send
 */
esp_err_t  active_task_send_to(const char *name, MsgBlk mblk, uint32_t wait_ms)
{
    if (NULL == name || NULL == mblk) return INNER_INVAILD_PARAM;
    ActiveTask *task = active_task_find(name);
    if (NULL == task) return TASK_NOT_EXIST;
    return task->put_message(task, mblk, wait_ms);
}
