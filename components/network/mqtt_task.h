/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-29 23:01:26
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-29 23:01:27
 * @FilePath    : /activetask/components/network/mqtt_task.h
 * @Description : mqtt task
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _MQTT_TASK_H_
#define _MQTT_TASK_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "msg_blk.h"
#include "active_task.h"
#include "network.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  mqtt task would push message in subscribed topics to next task
 * and send all msg in its queue to specified topic mapped with msg_type
 */

/***
 * @description : create a mqtt task
 * @param        {char} *name - name of mqtt task
 * @param        {int} stack - stack of task
 * @param        {int} priority - priority
 * @param        {int} core - cpu affinity
 * @param        {size_t} queue_len - length of queue
 * @param        {int} interval - interva time in ms for receiving, 0 means blocked
 * @param        {int} schedule - schedule time in ms
 * @param        {protocol_layer} *layer_data - protocol layer
 * @return       {*}
 */
active_task *mqtt_task_create(const char *name, int stack,
        int priority, int core, size_t queue_len, int interval,
        int schedule, protocol_layer *layer_data);

/***
 * @description : delete a mqtt task
 * @param        {active_task} *task - pointer to mqtt task
 * @return       {*}
 */
void mqtt_task_delete(active_task *task);

/***
 * @description : subscribe topic for receiving
 * @param        {active_task} *task - pointer to task
 * @param        {char} *topic - topic to be subscribed
 * @param        {int} mtype - message type
 * @param        {int} qos - qos for topic
 * @return       {*}
 */
at_error_t mqtt_task_subscribe(active_task *task, const char *topic, int mtype, int qos);

/***
 * @description : create a map from msg_type to topic which decide which topic would be published
 * @param        {active_task} *task - pointer to task
 * @param        {int} mtype - message type
 * @param        {char} *topic - topic to be published
 * @param        {int} qos - qos for topic
 * @return       {*}
 */
at_error_t mqtt_task_map_topic(active_task *task, int mtype, const char *topic, int qos);

#ifdef __cplusplus
}
#endif

#endif /* _MQTT_TASK_H_ */
