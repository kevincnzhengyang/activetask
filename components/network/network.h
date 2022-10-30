/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-29 13:28:31
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-29 13:28:31
 * @FilePath    : /activetask/components/network/network.h
 * @Description : common definitions
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _NETWORK_COMMON_H_
#define _NETWORK_COMMON_H_

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define NET_TAG "NETWORK"
#define NET_DEBUG(fmt, ...)  ESP_LOGD(NET_TAG, fmt, ##__VA_ARGS__)
#define NET_INFO(fmt, ...)   ESP_LOGI(NET_TAG, fmt, ##__VA_ARGS__)
#define NET_WARN(fmt, ...)   ESP_LOGW(NET_TAG, fmt, ##__VA_ARGS__)
#define NET_ERROR(fmt, ...)  ESP_LOGE(NET_TAG, fmt, ##__VA_ARGS__)

#define NET_ERR_BASE                0x411000
#define NET_INCOMPLETE_STACK        (NET_ERR_BASE+ 1)

#define WIFI_CONNECT_EVENT          BIT0
#define WIFI_DISCONNECT_EVENT       BIT1
#define BLE_CONNECT_EVENT           BIT2
#define BLE_DISCONNECT_EVENT        BIT3
#define MQTT_READY_EVENT            BIT4
#define MQTT_LOST_EVENT             BIT5

typedef enum {
    NET_LAYER_READY,
    NET_LAYER_BROKEN,
    NET_LAYER_SLEEP,
    NET_LAYER_STATUS_BUTT
} protocol_layer_status;

typedef struct {
    void                   *layer_data;
    EventGroupHandle_t     event_group;
    protocol_layer_status       status;
} protocol_layer;

#endif /* _NETWORK_COMMON_H_ */
