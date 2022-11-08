/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-11-07 17:54:46
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-11-08 00:24:49
 * @FilePath    : /activetask/components/network/transport_task.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "mdns.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "inner_err.h"
#include "linux_list.h"
#include "n2n_proto.h"
#include "transport_task.h"

#define TRANS_TAG "N2N_Proto"
#define TRANS_DEBUG(fmt, ...)  ESP_LOGD(TRANS_TAG, fmt, ##__VA_ARGS__)
#define TRANS_INFO(fmt, ...)   ESP_LOGI(TRANS_TAG, fmt, ##__VA_ARGS__)
#define TRANS_WARN(fmt, ...)   ESP_LOGW(TRANS_TAG, fmt, ##__VA_ARGS__)
#define TRANS_ERROR(fmt, ...)  ESP_LOGE(TRANS_TAG, fmt, ##__VA_ARGS__)

typedef struct {
    struct sockaddr_in       peer_addr;     // remote addr
    struct sockaddr_in      local_addr;     // local addr
    int                         socket;     // socket
} n2n_transpport;

static at_error_t start_mdns_service(void)
{
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    // set all device in Node, Terminal and Proxy
    mdns_hostname_set("my-esp32");
    mdns_instance_name_set("Jhon's ESP32 Thing");
}

static at_error_t add_mdns_services(void)
{

}

static at_error_t resolve_mdns_host(const char *dev_name, struct sockaddr_in *addr)
{

}

static at_error_t find_mdns_service(const char *dev_name, char *route, size_t route_len)
{

}

/***
 * @description : create a Transport task
 * @param        {char} *name - name of task
 * @param        {int} stack - stack of task
 * @param        {int} priority - priority
 * @param        {int} core - cpu affinity
 * @param        {size_t} queue_len - length of queue
 * @param        {int} interval - interva time in ms for receiving, 0 means blocked
 * @param        {int} schedule - schedule time in ms
 * @param        {protocol_layer} *layer_data - protocol layer
 * @return       {*}
 */
active_task *trans_task_create(const char *name, int stack,
        int priority, int core, size_t queue_len, int interval,
        int schedule, protocol_layer *layer_data);

/***
 * @description : delete a Transport task
 * @param        {active_task} *task - pointer to a Transport task
 * @return       {*}
 */
void trans_task_delete(active_task *task);

/***
 * @description : assign task to specific devcie
 * @param        {active_task} *task - pointer to Transport task
 * @param        {char} *instance - instance name of device
 * @param        {active_task} *dev_task - pointer to task for device
 * @return       {*}
 */
at_error_t trans_task_assgin_dev_task(active_task *task, const char *instance,
        active_task *dev_task);
