/***
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-11-05 12:03:40
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-11-05 12:03:40
 * @FilePath    : /activetask/components/network/n2n_proto.h
 * @Description : node to node protocol
 * @Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#ifndef _NODE_TO_NODE_PROTO_H_
#define _NODE_TO_NODE_PROTO_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "linux_list.h"
#include "active_task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define N2N_PROTO_VER           0x01

#define     N2N_NAME_LEN        16+1
#define     N2N_DATE_LEN        10+1
#define     N2N_ROUTE_LEN       32+1
#define     N2N_SCHEMA_LEN     256+1

typedef enum {
    N2N_DEV_NODE,
    N2N_DEV_ENTRYPOINT,
    N2N_DEV_TERMINAL,
    N2N_DEV_PROXY,
    N2N_DEV_BUTT
} n2n_dev_enum

typedef struct {
    char        hostname[N2N_NAME_LEN];     // inner defined
    char        instname[N2N_NAME_LEN];     // user defined
    char           brand[N2N_NAME_LEN];
    char             mfr[N2N_NAME_LEN];     // manufacture
    char           model[N2N_NAME_LEN];
    char              pd[N2N_DATE_LEN];
    n2n_dev_enum              dev_type;     // type of device
    struct list_head           ep_list;     // entry points list
} n2n_device;

#define SIZE_N2N_DEVICE     sizeof(n2n_device)

typedef struct {
    char          route[N2N_ROUTE_LEN];     // query route
    char      q_schema[N2N_SCHEMA_LEN];     // query result schema
    char      p_schema[N2N_SCHEMA_LEN];     // perform request schema
    struct list_head           ep_node;
    active_task                  *task;
} n2n_ep;

#define SIZE_N2N_EP     sizeof(n2n_ep)

/***
 * @description : callback function after entry point malloc
 * @param        {n2n_ep} *ep - pointer to entry point
 * @param        {void} *arg - user defined argument when proto init
 * @return       {*}
 */
typedef at_error_t (*on_n2n_ep_init)(n2n_ep *ep, void *arg);

/***
 * @description : callback function before entry point free
 * @param        {n2n_ep} *ep - pointer to entry point
 * @param        {void} *arg - user defined argument when proto init
 * @return       {*}
 */
typedef void (*on_n2n_ep_fini)(n2n_ep *ep, void *arg);

/***
 * @description : protocol stack init, automatically load devices stored in NVS
 * @param        {on_n2n_ep_init} on_init - callback function for init
 * @param        {on_n2n_ep_fini} on_fini - callback function for fini
 * @param        {void} *arg - user defined argument for callback functions
 * @return       {*}
 */
at_error_t n2n_proto_init(on_n2n_ep_init on_init, on_n2n_ep_fini on_fini, void *arg);

/***
 * @description : protocol stack clear
 * @return       {*}
 */
void n2n_proto_fini(void);

/***
 * @description : init a device
 * @param        {n2n_dev_enum} *dtype - type of device
 * @param        {char} *hostname - inner defined
 * @param        {char} *instname - user defined
 * @param        {char} *brand
 * @param        {char} *mfr
 * @param        {char} *model
 * @param        {char} *pd
 * @return       {*}
 */
n2n_device *n2n_device_init(n2n_dev_enum *dtype, const char *hostname,
    const char *instname, const char *brand, const char *mfr, const char *model,
    const char *pd);

/***
 * @description : load device from NVS
 * @return       {*}
 */
n2n_device *n2n_device_load(void);

/***
 * @description : save device to NVS
 * @return       {*}
 */
at_error_t n2n_device_save(void);

/***
 * @description : set name of instance
 * @param        {char} *name - new name
 * @return       {*}
 */
at_error_t n2n_device_set_name(const char *name);

/***
 * @description : encode device information into a JSON object
 * @param        {n2n_device} *dev - pointer to a device
 * @param        {char} **pbuff - pointer to pointer of a buff
 * @return       {*}
 */
at_error_t n2n_device_to_json(n2n_device *dev, char **pbuff);

/***
 * @description : malloc a new entry point
 * @param        {char} *route
 * @param        {char} *q_schema
 * @param        {char} *p_schema
 * @param        {active_task} *task - task for process requests
 * @return       {*}
 */
n2n_ep *n2n_ep_malloc(const char *route, const char *q_schema,
    const char *p_schema, active_task *task);

/***
 * @description : free a entry point
 * @param        {n2n_ep} *ep - pointer to entry point
 * @return       {*}
 */
void n2n_ep_free(n2n_ep *ep);

/***
 * @description : get entry point by route
 * @param        {char} *route - route
 * @return       {*}
 */
n2n_ep *n2n_ep_get_by_route(const char *route);

/***
 * @description : get entry point list
 * @return       {*}
 */
struct list_head *n2n_ep_get_list(void);

/***
 * @description : encode entry point information into a JSON object
 * @param        {n2n_ep} *ep - pointer to an entry point
 * @param        {char} **pbuff - pointer to pointer of a buff
 * @return       {*}
 */
at_error_t n2n_ep_to_json(n2n_ep *ep, char **pbuff);

#ifdef __cplusplus
}
#endif

#endif /* _NODE_TO_NODE_PROTO_H_ */
