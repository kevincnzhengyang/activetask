/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-29 23:01:35
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-31 00:04:54
 * @FilePath    : /activetask/components/network/mqtt_task.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"

#include "linux_list.h"
#include "linux_hlist.h"
#include "blackboard.h"
#include "wifi_prov.h"
#include "mqtt_task.h"

#define MQTT_TAG "NETWORK"
#define MQTT_DEBUG(fmt, ...)  ESP_LOGD(MQTT_TAG, fmt, ##__VA_ARGS__)
#define MQTT_INFO(fmt, ...)   ESP_LOGI(MQTT_TAG, fmt, ##__VA_ARGS__)
#define MQTT_WARN(fmt, ...)   ESP_LOGW(MQTT_TAG, fmt, ##__VA_ARGS__)
#define MQTT_ERROR(fmt, ...)  ESP_LOGE(MQTT_TAG, fmt, ##__VA_ARGS__)

typedef struct {
    char                        *topic;
    int                       msg_type;
    int                            qos;
    struct list_head              node;
} mqtt_topics;

#define SIZE_MQTT_TOPICS    sizeof(mqtt_topics)

typedef struct {
    active_task               act_task;
    struct list_head       recv_topics;
    struct list_head       send_topics;
    char                   *broker_uri;
    esp_mqtt_client_handle_t    client;
} mqtt_task;

#define SIZE_MQTT_TASK          sizeof(mqtt_task)

static mqtt_topics *get_by_topic(mqtt_task *mt, const char *topic)
{
    if (NULL == mt || NULL == topic) return NULL;

    if (list_empty(&mt->recv_topics)) {
        MQTT_INFO("empty recv topics, can't find %s", topic);
        return NULL;
    }

    mqtt_topics *mq_topic = NULL;
    list_for_each_entry(mq_topic, &mt->recv_topics, node) {
        if (0 == strcmp(topic, mq_topic->topic)) {
            return mq_topic;
        }
    }
    MQTT_ERROR("can't find topic %s", topic);
    return NULL;
}

static mqtt_topics *get_by_msg_type(mqtt_task *mt, int msg_type)
{
    if (NULL == mt || 0 > msg_type) return NULL;

    if (list_empty(&mt->send_topics)) {
        MQTT_INFO("empty send topics, can't find %d", msg_type);
        return NULL;
    }

    mqtt_topics *mq_topic = NULL;
    list_for_each_entry(mq_topic, &mt->recv_topics, node) {
        if (msg_type == mq_topic->msg_type) {
            return mq_topic;
        }
    }
    MQTT_ERROR("can't find msg_type %d", msg_type);
    return NULL;
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        MQTT_ERROR("Last error %s: 0x%x", message, error_code);
    }
}

static void process_mqtt_data(mqtt_task *mt, char *topic, char *data, int data_len)
{
    if (NULL == mt || NULL == data || 0 >= data_len) return;
    if (NULL == mt->act_task.next_task) return;

    // find topic
    mqtt_topics *mq_topic = get_by_topic(mt, topic);
    if (NULL == mq_topic) {
        return;
    }

    // prepare msgblk & datablk
    datablk *db = datablk_malloc(data_len);
    if (NULL == db) {
        MQTT_ERROR("failed to malloc datablk for msg from %s", topic);
        return;
    }

    msgblk *mb = msgblk_malloc(db);
    if (NULL == mb) {
        MQTT_ERROR("failed to malloc msgblk for msg from %s", topic);
        datablk_free(db);
        return;
    }
    mb->msg_type = mq_topic->msg_type;
    memcpy(db->wr_ptr, data, data_len);
    datablk_move_wr(db, data_len);
    mt->act_task.put_message_next(&mt->act_task, mb, mt->act_task.interv_ms);
}

static void mqtt_subscrib_topics(mqtt_task *mt)
{
    if (NULL == mt) return;

    if (list_empty(&mt->recv_topics)) {
        MQTT_INFO("empty recv topics");
        return;
    }

    int msg_id;
    mqtt_topics *mq_topic = NULL;
    list_for_each_entry(mq_topic, &mt->recv_topics, node) {
        msg_id = esp_mqtt_client_subscribe(mt->client, mq_topic->topic, mq_topic->qos);
        MQTT_INFO("subscribe topic %s with msg_id %d", mq_topic->topic, msg_id);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
        int32_t event_id, void *event_data)
{
    MQTT_DEBUG("Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    mqtt_task *mt = (mqtt_task *)handler_args;
    protocol_layer *layer = (protocol_layer *)mt->act_task.app_data;

    esp_mqtt_event_handle_t event = event_data;
    // esp_mqtt_client_handle_t client = event->client;
    // int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        MQTT_INFO("MQTT_EVENT_CONNECTED");
        mqtt_subscrib_topics(mt);   // subscribe topics once connected
        xEventGroupSetBits(layer->event_group, MQTT_READY_EVENT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        MQTT_INFO("MQTT_EVENT_DISCONNECTED");
        xEventGroupSetBits(layer->event_group, MQTT_LOST_EVENT);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        MQTT_INFO("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        MQTT_INFO("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        MQTT_INFO("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        MQTT_INFO("MQTT_EVENT_DATA");
        // printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        // printf("DATA=%.*s\r\n", event->data_len, event->data);
        process_mqtt_data(mt, event->topic, event->data, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        MQTT_INFO("MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            MQTT_INFO("Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        MQTT_INFO("Other event id:%d", event->event_id);
        break;
    }
}

static at_error_t mqtt_on_init(active_task *task)
{
    if (NULL == task) return INNER_INVAILD_PARAM;
    MQTT_DEBUG("%s init", task->name);
    at_error_t res = ESP_FAIL;
    mqtt_task *mt = container_of(task, mqtt_task, act_task);
    protocol_layer *layer = (protocol_layer *)task->app_data;
    layer->status = NET_LAYER_STATUS_BUTT;

    // prepare WiFi
    if (INNER_RES_OK != (res = wifi_sta_start_provision(layer->event_group))) {
        MQTT_INFO("WiFi failed to provisioned");
        return res;
    }
    MQTT_INFO("WiFi connected");

    const char uri_key[] = "mqtt_broker_uri\0";

    // init mqtt client
    mt->broker_uri = (char *)blackboard_get(uri_key);
    if (NULL == mt->broker_uri) {
        // register and flush
        at_error_t res = blackboard_persist(uri_key, strlen(CONFIG_MQTT_BROKER_URL)+1);
        if (INNER_RES_OK != res) {
            MQTT_ERROR("failed to register broker URI");
            return res;
        }
        mt->broker_uri = (char *)blackboard_get(uri_key);
        sprintf(mt->broker_uri, "%s", CONFIG_MQTT_BROKER_URL);
        res = blackboard_flush(uri_key);
        if (INNER_RES_OK != res) {
            MQTT_ERROR("failed to store broker URI");
            return res;
        }
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mt->broker_uri,
    };

    mt->client = esp_mqtt_client_init(&mqtt_cfg);
    MQTT_INFO("MQTT Client init");
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mt->client, ESP_EVENT_ANY_ID, mqtt_event_handler, task);
    esp_mqtt_client_start(mt->client);
    MQTT_INFO("MQTT Client starting ...");
    return TASK_SVC_CONTINUE;
}

static at_error_t mqtt_on_loop(active_task *task)
{
    if (NULL == task) return INNER_INVAILD_PARAM;
    MQTT_DEBUG("%s on_loop", task->name);
    protocol_layer *layer = (protocol_layer *)task->app_data;

    if (NET_LAYER_READY != layer->status) {
        // delay and check again when not ready
        delay_ms(task->interv_ms);
        return TASK_SVC_CONTINUE;
    }
    return INNER_RES_OK;
}

static at_error_t mqtt_on_message(active_task *task, msgblk *mblk)
{
    if (NULL == task || NULL == mblk) return INNER_INVAILD_PARAM;
    MQTT_DEBUG("%s handle message %p", task->name, mblk);
    datablk *db = msgblk_first_datablk(mblk);
    if (NULL == db) {
        MQTT_ERROR("ignore msg %p due to empty datablk", mblk);
        return INNER_RES_OK;    // mblk released in task_svc function
    }


    mqtt_task *mt = container_of(task, mqtt_task, act_task);
    // protocol_layer *layer = (protocol_layer *)task->app_data;

    // find topic according to msg_type
    mqtt_topics *mq_topic = get_by_msg_type(mt, mblk->msg_type);
    if (NULL == mq_topic) {
        MQTT_ERROR("ignore msg %p due to type %d", mblk, mblk->msg_type);
        return INNER_RES_OK;    // mblk released in task_svc function
    }

    // TODO handle databllk list in msgblk
    // send msg
    int msg_id = esp_mqtt_client_publish(mt->client, mq_topic->topic,
            db->rd_ptr, datablk_length(db),
            mq_topic->qos, 0);
    if (-1 == msg_id) {
        MQTT_ERROR("failed publish msg %p to %s", mblk, mq_topic->topic);
    } else {
        MQTT_INFO("publish msg %p to %s, msgid %d", mblk, mq_topic->topic, msg_id);
    }
    return INNER_RES_OK;    // mblk released in task_svc function
}

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
        int schedule, protocol_layer *layer_data)
{
    mqtt_task *mt = (mqtt_task *)active_task_create(name, SIZE_MQTT_TASK,
            stack, priority, core, queue_len, interval, schedule, layer_data);
    if (NULL == mt) {
        MQTT_ERROR("failed to create task %s", name);
        return NULL;
    }

    INIT_LIST_HEAD(&mt->recv_topics);
    INIT_LIST_HEAD(&mt->send_topics);

    mt->act_task.on_init = mqtt_on_init;
    mt->act_task.on_loop = mqtt_on_loop;
    mt->act_task.on_message = mqtt_on_message;
    return (active_task *)mt;
}

/***
 * @description : delete a mqtt task
 * @param        {active_task} *task - pointer to mqtt task
 * @return       {*}
 */
void mqtt_task_delete(active_task *task)
{
    if (NULL == task) return;

    mqtt_task *mt = container_of(task, mqtt_task, act_task);
    esp_mqtt_client_stop(mt->client);
    esp_mqtt_client_destroy(mt->client);
    mqtt_topics *mq_topic = NULL, *temp = NULL;
    if (!list_empty(&mt->recv_topics)) {
        list_for_each_entry_safe(mq_topic, temp, &mt->recv_topics, node) {
            free(mq_topic->topic);
            free(mq_topic);
        }
    }
    if (!list_empty(&mt->send_topics)) {
        list_for_each_entry_safe(mq_topic, temp, &mt->send_topics, node) {
            free(mq_topic->topic);
            free(mq_topic);
        }
    }
    active_task_delete(task);
}

/***
 * @description : subscribe topic for receiving
 * @param        {active_task} *task - pointer to task
 * @param        {char} *topic - topic to be subscribed
 * @param        {int} mtype - message type
 * @param        {int} qos - qos for topic
 * @return       {*}
 */
at_error_t mqtt_task_subscribe(active_task *task, const char *topic, int mtype, int qos)
{
    if (NULL == task || NULL == topic) return INNER_INVAILD_PARAM;
    mqtt_task *mt = container_of(task, mqtt_task, act_task);

    mqtt_topics *mq_topic = (mqtt_topics *)malloc(SIZE_MQTT_TOPICS);
    if (NULL == mq_topic) {
        MQTT_ERROR("failed to malloc mq topic %s", topic);
        return MEMORY_MALLOC_FAILED;
    }
    mq_topic->topic = strdup(topic);
    mq_topic->msg_type = mtype;
    mq_topic->qos = qos;
    list_add_tail(&mq_topic->node, &mt->recv_topics);
    MQTT_INFO("add recv topic %s msg_type %d", topic, mtype);
    return INNER_RES_OK;
}

/***
 * @description : create a map from msg_type to topic which decide which topic would be published
 * @param        {active_task} *task - pointer to task
 * @param        {int} mtype - message type
 * @param        {char} *topic - topic to be published
 * @param        {int} qos - qos for topic
 * @return       {*}
 */
at_error_t mqtt_task_map_topic(active_task *task, int mtype, const char *topic, int qos)
{
    if (NULL == task || NULL == topic) return INNER_INVAILD_PARAM;
    mqtt_task *mt = container_of(task, mqtt_task, act_task);

    mqtt_topics *mq_topic = (mqtt_topics *)malloc(SIZE_MQTT_TOPICS);
    if (NULL == mq_topic) {
        MQTT_ERROR("failed to malloc mq topic %s", topic);
        return MEMORY_MALLOC_FAILED;
    }
    mq_topic->topic = strdup(topic);
    mq_topic->msg_type = mtype;
    mq_topic->qos = qos;
    list_add_tail(&mq_topic->node, &mt->send_topics);
    MQTT_INFO("add send msg_type %d topic %s", mtype, topic);
    return INNER_RES_OK;
}
