/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-05 17:44:27
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-05 17:44:28
 * @FilePath    : /LovelyLight/app/test/main.cpp
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"
// #include "esp_task_wdt.h"

#include "linux_macros.h"
#include "data_blk.h"
#include "active_task.h"
#include "blackboard.h"
#include "mqtt_task.h"

#define APP_TAG "App"
#define APP_DEBUG(fmt, ...)  ESP_LOGD(APP_TAG, fmt, ##__VA_ARGS__)
#define APP_INFO(fmt, ...)   ESP_LOGI(APP_TAG, fmt, ##__VA_ARGS__)
#define APP_WARN(fmt, ...)   ESP_LOGW(APP_TAG, fmt, ##__VA_ARGS__)
#define APP_ERROR(fmt, ...)  ESP_LOGE(APP_TAG, fmt, ##__VA_ARGS__)

#define P_NUM    3
#define C_NUM    4

bool start_flag = false;
bool run_flag = true;

at_error_t p_on_loop(active_task *task)
{
    if (!start_flag) {
        delay_ms(200);
        return TASK_SVC_CONTINUE;
    }

    datablk *data = NULL;
    msgblk *msg = NULL;
    static int i;

    data = datablk_malloc(64);
    if (NULL == data) return TASK_SVC_BREAK;
    int len = snprintf(data->wr_ptr, 63, "%s-%d", task->name, i++);
    if (len != datablk_move_wr(data, len)) {
        KRNL_ERROR("P >>> %s move wr %d\n", task->name, len);
        return TASK_SVC_CONTINUE;
    }
    KRNL_DEBUG("P %s:move wr with %d\n", task->name, len);
    KRNL_DEBUG("P %s: %s\n", task->name, (char *)data->rd_ptr);

    msg = msgblk_malloc(data);

    int res = task->put_message_next(task, msg, 200);
    if (INNER_RES_OK != res) {
        KRNL_ERROR("P >>> %s push failed %d\n", task->name, res);
        return TASK_SVC_CONTINUE;
    }
    KRNL_DEBUG("P %s:put queue %p\n", task->name, msg);
    msgblk_free(msg);
    KRNL_DEBUG("P %s:free data %p\n", task->name, msg);
    delay_ms((rand() & 0xFF));
    return TASK_SVC_CONTINUE;
}


at_error_t c_on_loop(active_task *task)
{
    if (!start_flag) {
        delay_ms(2000);
        return TASK_SVC_CONTINUE;
    }
    return INNER_RES_OK;
}

at_error_t c_on_message(active_task *task, msgblk *mblk)
{
    datablk *data = msgblk_first_datablk(mblk);
    if (NULL == data) KRNL_ERROR("P null data block %s: %p\n", task->name, mblk);
    KRNL_DEBUG("C %s: %s\n", task->name, (char *)data->rd_ptr);
    int pos = datablk_move_rd(data, datablk_length(data));
    KRNL_DEBUG("C %s:move rd with %d\n", task->name, pos);
    delay_ms((rand() & 0xFF));

    if (circ_queue_is_empty(task->queue)) {
        run_flag = false;
        return TASK_SVC_BREAK;
    }
    else  return  TASK_SVC_CONTINUE;
}

void app_main()
{
    // set log level
    esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("LEDs", ESP_LOG_DEBUG);
    // esp_log_level_set("BlackBoard", ESP_LOG_DEBUG);
    // esp_log_level_set("ASR", ESP_LOG_DEBUG);
    // esp_log_level_set("Sensors", ESP_LOG_DEBUG);

    APP_INFO("Startup..");
    APP_INFO("Free Heap Size: %lu bytes", esp_get_free_heap_size());
    APP_INFO("IDF version: %s", esp_get_idf_version());

    // create event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // watchdog
    // esp_task_wdt_config_t wdtConfig =
    // {
    //     .timeout_ms = 10000,
    //     .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    //     .trigger_panic = false,
    // };
    // ESP_ERROR_CHECK(esp_task_wdt_init(&wdtConfig));
    // APP_INFO("Watch Dog init");

    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    APP_INFO("NVS flash init");

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    APP_INFO("event loop created");

    /* Initialize the blackboard */
    blackboard_init(128);
    APP_INFO("blackboard init");


    // while (1) {
    //     vTaskDelay(3000 / portTICK_PERIOD_MS);
    // }

    if (INNER_RES_OK != datablk_pool_init(NULL, NULL, NULL)) {
        datablk_pool_fini();
    }
    if (INNER_RES_OK != msgblk_pool_init(NULL, NULL, NULL, NULL, NULL)) {
        msgblk_pool_fini();
    }

    // char name[64] = "\0";
    // active_task *p_handler[P_NUM];
    // snprintf(name, 63, "consumer");
    // active_task *c_handler = active_task_create(name, 1024, 1, 1, 16, 100, 0, NULL);
    // c_handler->on_loop = c_on_loop;
    // c_handler->on_message = c_on_message;

    // for (int i = 0; i < P_NUM; i++) {
    //     snprintf(name, 63, "activetask_%d", i);
    //     p_handler[i] = active_task_create(name, 1024, 1, 1, 0, 100, 0, NULL);
    //     p_handler[i]->on_loop = p_on_loop;
    //     p_handler[i]->next_task = c_handler;
    // }

    // for (int i = 0; i < P_NUM; i++) {
    //     p_handler[i]->task_begin(p_handler[i]);
    // }
    // c_handler->task_begin(c_handler);
    APP_INFO("------start------\n");
    start_flag = true;
    while (run_flag) {
        delay_ms(5000);
    }
    APP_INFO("------end------\n");
    msgblk_pool_fini();
    datablk_pool_fini();
    APP_INFO("------cleared------\n");
}
