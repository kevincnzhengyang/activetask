/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-11-07 17:54:35
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-11-07 17:54:35
 * @FilePath    : /activetask/components/network/transport_task.h
 * @Description :
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _COAP_TASK_H_
#define _COAP_TASK_H_

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
 * PDU between Nodes
 */
#pragma pack(1)
typedef struct {
    uint8_t                    version;     // version
    uint8_t                       type;     // pdu type
    uint16_t                    msg_id;     // ID of message
    uint16_t                      code;     // code for message
    uint16_t                header_len;     // PDU header length
    char                   pdu_data[0];     // route + json
} n2n_pdu;
#pragma pack()

#define SIZE_N2N_PDU_HEAD    sizeof(n2n_pdu)

#define N2N_PDU_GET_ROUTE_LEN(p) ((p)->header_len - SIZE_N2N_PDU_HEAD)

#define N2N_PDU_GET_ROUTE(p)     ((p)->pdu_data)

#define N2N_PDU_GET_JSON(p) (&(p)->pdu_data[N2N_PDU_GET_ROUTE_LEN(p)])

/**
 * PDU type
 */
typedef enum {
    N2N_PT_INVALID,
    N2N_PT_ACK,         // Ack for Auth, Query, Subscriber, Command
    N2N_PT_AUTH,
    N2N_PT_QUERY,
    N2N_PT_SUBSCRIBE,
    N2N_PT_COMMAND,
    N2N_PT_REPORT,
    N2N_PT_NOTIFY,
    N2N_PT_BUTT
} n2n_pdu_type;

/**
 * Message Block Type
 */
typedef enum {
    N2N_MT_N_IN,        // msgblk for normal udp diagram received
    N2N_MT_N_OUT,       // msgblk for normal udp diagram sent
    N2N_MT_BC_IN,       // msgblk for broadcast receiving
    N2N_MT_BC_OUT,      // msgblk for broadcast sending
    N2N_MT_BUTT
} n2n_msg_type;

/**
 * Data Block Type
 */
typedef enum {
    N2N_DT_TRANS,       // dblk for transport
    N2N_DT_PDU,         // dblk for PDU
    N2N_DT_BUTT
} n2n_data_type;

/**
 * Transport Task can act as a UDP client and/or a UDP Server
 */

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

#ifdef __cplusplus
}
#endif

#endif /* _COAP_TASK_H_ */
