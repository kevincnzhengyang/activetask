/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-29 23:24:54
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-29 23:24:54
 * @FilePath    : /activetask/components/network/wifi_prov.h
 * @Description : WiFi Provisioning functions
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _WIFI_PROVISION_H_
#define _WIFI_PROVISION_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "inner_err.h"
#include "network.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * WiFi Provision functions for wifi connection
 *
 * assume nvs flash inited
 */

/***
 * @description : check if WiFi STA provisioned
 *           read ssid and psk from nvs
 * @return       {*}
 */
bool wifi_sta_is_provisioned(void);

/***
 * @description : start WiFi STA provision
 * @param        {EventGroupHandle_t} event_group - set bits if connected
 * @return       {*}
 */
at_error_t wifi_sta_start_provision(EventGroupHandle_t event_group);

#ifdef __cplusplus
}
#endif

#endif /* _WIFI_PROVISION_H_ */
