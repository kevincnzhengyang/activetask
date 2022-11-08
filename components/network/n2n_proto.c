/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-11-05 12:03:53
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-11-08 13:13:01
 * @FilePath    : /activetask/components/network/n2n_proto.c
 * @Description : node to node protocol
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "cJSON.h"

#include "linux_list.h"
#include "linux_macros.h"
#include "inner_err.h"

#include "n2n_proto.h"

#define N2N_TAG "N2N_Proto"
#define N2N_DEBUG(fmt, ...)  ESP_LOGD(N2N_TAG, fmt, ##__VA_ARGS__)
#define N2N_INFO(fmt, ...)   ESP_LOGI(N2N_TAG, fmt, ##__VA_ARGS__)
#define N2N_WARN(fmt, ...)   ESP_LOGW(N2N_TAG, fmt, ##__VA_ARGS__)
#define N2N_ERROR(fmt, ...)  ESP_LOGE(N2N_TAG, fmt, ##__VA_ARGS__)

struct _n2n_proto_stack {
    n2n_device                 _device;
    on_n2n_ep_init               _init;
    on_n2n_ep_fini               _fini;
    void                         *_arg;
    struct list_head              _eps;
    nvs_handle_t                  hnvs;
};

static struct _n2n_proto_stack g_n2n_proto_stack;

static at_error_t save_ep_to_nvs(const char *ep_name, n2n_ep *ep)
{
    at_error_t res = ESP_FAIL;
    // doc
    cJSON *pdoc = cJSON_CreateObject();
    if (NULL == pdoc) {
        N2N_ERROR("save entry point %s failed to create JSON", ep_name);
        return N2N_DEV_JSON_FAILED;
    }
    // route
    if (NULL == cJSON_AddStringToObject(pdoc, "route",
                        (const char *)ep->route)) {
        N2N_ERROR("entry point %s encode route failed", ep_name);
        res = N2N_DEV_ROUTE_MISSED;
        goto clearup;
    }
    // q_schema
    if (NULL == cJSON_AddStringToObject(pdoc, "q_schema",
                        (const char *)ep->q_schema)) {
        N2N_ERROR("entry point %s encode q_schema failed", ep_name);
        res = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    // p_schema
    if (NULL == cJSON_AddStringToObject(pdoc, "p_schema",
                        (const char *)dev->p_schema)) {
        N2N_ERROR("entry point %s encode p_schema failed", ep_name);
        res = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    char *pbuff = cJSON_PrintUnformatted(pdoc);
    cJSON_Delete(pdoc); // release pdoc
    res = nvs_set_str(g_n2n_proto_stack.hnvs, ep_name, pbuff);
    N2N_INFO("entry point %s save as %s", ep_name, pbuff);
    free(pbuff);        // release encode buff
    return ESP_OK != err ? err : nvs_commit(g_n2n_proto_stack.hnvs);

clearup:
    cJSON_Delete(pdoc); // release pdoc
    return res;
}

static at_error_t restore_ep_from_nvs(const char *name)
{
    if (NULL == name) return INNER_INVAILD_PARAM;
    char buff[1024] = "\0";
    size_t buff_len = 1024;

    // get data from NVS
    esp_err_t err = nvs_get_str(g_n2n_proto_stack.hnvs, name,
                            buff, &buff_len);
    if (ESP_ERR_NVS_NOT_FOUND == err) return N2N_EMPTY_DEV_INFO;
    else if (ESP_OK != err) return err;

    if (0 == buff_len) {
        N2N_ERROR("empty entry point %s", name);
        return N2N_EMPTY_DEV_INFO;
    }

    n2n_ep *ep = n2n_ep_malloc(NULL, NULL, NULL, NULL);
    if (NULL == ep) return N2N_DEV_MALLOC_FAILED;

    cJSON *pdoc = NULL, *pvalue = NULL;
    // parse JSON
    if (NULL == (pdoc = cJSON_ParseWithLength(buff, buff_len)))
    {
        N2N_ERROR("parse entry point in JSON failed, raw=%.*s", buff_len, buff);
        n2n_ep_free(ep);
        return N2N_DEV_PARSE_FAILED;
    }
    // route
    pvalue = cJSON_GetObjectItem(pdoc, "route");
    if (NULL == pvalue || !cJSON_IsString(pvalue))
    {
        N2N_ERROR("ep without route");
        err = N2N_DEV_ROUTE_MISSED;
        goto clearup;
    }
    snprintf(ep->route, N2N_ROUTE_LEN, pvalue->valuestring);
    // q_schema
    pvalue = cJSON_GetObjectItem(pdoc, "q_schema");
    if (NULL == pvalue || !cJSON_IsString(pvalue))
    {
        N2N_ERROR("device without q_schema");
        err = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    snprintf(ep->q_schema, N2N_SCHEMA_LEN, pvalue->valuestring);
    // p_schema
    pvalue = cJSON_GetObjectItem(pdoc, "p_schema");
    if (NULL == pvalue || !cJSON_IsString(pvalue))
    {
        N2N_ERROR("device without p_schema");
        err = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    snprintf(ep->p_schema, N2N_SCHEMA_LEN, pvalue->valuestring);

    cJSON_Delete(pdoc);
    return INNER_RES_OK;

clearup:
    n2n_ep_free(ep);
    cJSON_Delete(pdoc);
    return err;
}

static at_error_t restore_device_from_nvs(void)
{
    char buff[1024] = "\0";
    size_t buff_len = 1024;
    // get data from NVS
    esp_err_t err = nvs_get_str(g_n2n_proto_stack.hnvs, "_N2N_Device_",
                            buff, &buff_len);
    if (ESP_ERR_NVS_NOT_FOUND == err) return N2N_EMPTY_DEV_INFO;
    else if (ESP_OK != err) return err;

    if (0 == buff_len) {
        N2N_ERROR("empty device");
        return N2N_EMPTY_DEV_INFO;
    }

    n2n_device *dev = &g_n2n_proto_stack._device;
    cJSON *pdoc = NULL, *pvalue = NULL;
    // parse JSON
    if (NULL == (pdoc = cJSON_ParseWithLength(buff, buff_len)))
    {
        N2N_ERROR("parse action in JSON failed, raw=%.*s", buff_len, buff);
        return N2N_DEV_PARSE_FAILED;
    }
    // hostname
    pvalue = cJSON_GetObjectItem(pdoc, "hostname");
    if (NULL == pvalue || !cJSON_IsString(pvalue))
    {
        N2N_ERROR("device without name");
        err = N2N_DEV_NAME_MISSED;
        goto clearup;
    }
    snprintf(dev->hostname, N2N_NAME_LEN, pvalue->valuestring);
    // instname
    pvalue = cJSON_GetObjectItem(pdoc, "instname");
    if (NULL == pvalue || !cJSON_IsString(pvalue))
    {
        N2N_ERROR("device without name");
        err = N2N_DEV_NAME_MISSED;
        goto clearup;
    }
    snprintf(dev->instname, N2N_NAME_LEN, pvalue->valuestring);
    // brand
    pvalue = cJSON_GetObjectItem(pdoc, "brand");
    if (NULL != pvalue && cJSON_IsString(pvalue))
    {
        snprintf(dev->brand, N2N_NAME_LEN, pvalue->valuestring);
    }
    // mfr
    pvalue = cJSON_GetObjectItem(pdoc, "mfr");
    if (NULL != pvalue && cJSON_IsString(pvalue))
    {
        snprintf(dev->mfr, N2N_NAME_LEN, pvalue->valuestring);
    }
    // model
    pvalue = cJSON_GetObjectItem(pdoc, "model");
    if (NULL != pvalue && cJSON_IsString(pvalue))
    {
        snprintf(dev->model, N2N_NAME_LEN, pvalue->valuestring);
    }
    // pd
    pvalue = cJSON_GetObjectItem(pdoc, "pd");
    if (NULL != pvalue && cJSON_IsString(pvalue))
    {
        snprintf(dev->pd, N2N_DATE_LEN, pvalue->valuestring);
    }
    // dev_type
    pvalue = cJSON_GetObjectItem(pdoc, "dev_type");
    if (NULL == pvalue || !cJSON_IsNumber(pvalue))
    {
        N2N_ERROR("device without dev_type");
        err = N2N_DEV_TYPE_MISSED;
        goto clearup;
    }
    dev->dev_type = (n2n_dev_enum)pvalue->valueint;

    // get entry points array
    cJSON *children = cJSON_GetObjectItem(pdoc, "entry_points");
    if (NULL == children || !cJSON_IsArray(children))
    {
        cJSON_Delete(pdoc);
        return INNER_RES_OK;
    }

    // itera all entry points
    for (int j = 0; j < cJSON_GetArraySize(children); j++) {
        pvalue = cJSON_GetArrayItem(children, j);
        if (NULL == pvalue || !cJSON_IsString(pvalue))
        {
            N2N_ERROR("device %s[%s] child %d without string",
                    dev->hostname, dev->instname, j);
            err = N2N_DEV_CHILD_MISSED;
            goto clearup;
        }
        // load the child with this dev as parent
        err = restore_ep_from_nvs(pvalue->valuestring);
        if (INNER_RES_OK != err) goto clearup;
    }
    cJSON_Delete(pdoc);
    return INNER_RES_OK;

clearup:
    cJSON_Delete(pdoc);
    return err;
}

/***
 * @description : protocol stack init, automatically load devices stored in NVS
 * @param        {on_n2n_ep_init} on_init - callback function for init
 * @param        {on_n2n_ep_fini} on_fini - callback function for fini
 * @param        {void} *arg - user defined argument for callback functions
 * @return       {*}
 */
at_error_t n2n_proto_init(on_n2n_ep_init on_init, on_n2n_ep_fini on_fini, void *arg)
{
    g_n2n_proto_stack._init = on_init;
    g_n2n_proto_stack._fini = on_fini;
    g_n2n_proto_stack._arg = arg;
    INIT_LIST_HEAD(&g_n2n_proto_stack._eps);

    // open NVS
    esp_err_t err = nvs_open("N2N", NVS_READWRITE, &g_n2n_proto_stack.hnvs);
    if (ESP_OK != err) return err;

    if (INNER_RES_OK != (err = restore_device_from_nvs())) {
        N2N_ERROR("load dvice failed due to %d", err);
        return err;
    } else {
        N2N_INFO("device %s[%s] loaded", g_n2n_proto_stack._device.instance,
                g_n2n_proto_stack._device.dev_name);
        return INNER_RES_OK;
    }
}

/***
 * @description : protocol stack clear
 * @return       {*}
 */
void n2n_proto_fini(void)
{
    nvs_close(g_n2n_proto_stack.hnvs);
    n2n_ep *ep, *tmp;
    list_for_each_entry_safe(ep, tmp, &g_n2n_proto_stack._eps, ep_node) {
        n2n_ep_free(ep);
    }
}

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
    const char *pd)
{
    n2n_device *dev = &g_n2n_proto_stack._device;
    snprintf(dev->hostname, N2N_NAME_LEN, hostname);
    snprintf(dev->instname, N2N_NAME_LEN, instname);
    snprintf(dev->brand, N2N_NAME_LEN, brand);
    snprintf(dev->mfr, N2N_NAME_LEN, mfr);
    snprintf(dev->model, N2N_NAME_LEN, model);
    snprintf(dev->pd, N2N_DATE_LEN, pd);
    dev->dev_type = dtype;
    return dev;
}

/***
 * @description : load device from NVS
 * @return       {*}
 */
n2n_device *n2n_device_load(void)
{
    return &g_n2n_proto_stack._device;
}

/***
 * @description : save a device to NVS
 * @return       {*}
 */
at_error_t n2n_device_save(void)
{
    n2n_device *dev = &g_n2n_proto_stack._device;

    at_error_t res = ESP_FAIL;

    // save ep first
    if (!list_empty(&dev->ep_list)) {
        // save each entry point
        n2n_ep *ep, *tmp;
        list_for_each_entry_safe(ep, tmp, &dev->ep_list, ep_node) {
            if (INNER_RES_OK != (res = save_ep_to_nvs(ep->route, ep))) return res;
            i++;
        }
    }

    char *buff = NULL;
    if (INNER_RES_OK != (res = n2n_device_to_json(dev, &buff))) {
        N2N_ERROR("device %s[%s] failed to json", dev->hostname, dev->instname);
        return res;
    }
    res = nvs_set_str(g_n2n_proto_stack.hnvs, dev->hostname, buff);
    N2N_INFO("device %s[%s] save as %s", dev->hostname, dev->instname, buff);
    free(buff);        // release encode buff
    return ESP_OK != err ? err : nvs_commit(g_n2n_proto_stack.hnvs);
}

/***
 * @description : set name of a device
 * @param        {n2n_device} *dev - pointer to a device
 * @param        {char} *name - new name
 * @return       {*}
 */
at_error_t n2n_device_set_name(n2n_device *dev, const char *name)
{
    if (NULL == dev || NULL == name) return INNER_INVAILD_PARAM;

    snprintf(dev->instname, N2N_NAME_LEN, "%s", name);
    return INNER_RES_OK;
}

/***
 * @description : encode device information into a JSON object
 * @param        {n2n_device} *dev - pointer to a device
 * @param        {char} **pbuff - pointer to pointer of a buff
 * @return       {*}
 */
at_error_t n2n_device_to_json(n2n_device *dev, char **pbuff)
{
    if (NULL == dev || NULL == pbuff) return INNER_INVAILD_PARAM;

    n2n_device *dev = &g_n2n_proto_stack._device;

    at_error_t res = ESP_FAIL;
    char **routers = NULL;
    int count = 0;
    *pdoc = NULL;

    // save ep first
    if (!list_empty(&dev->ep_list)) {
        // get count of entry point
        list_for_each_entry_safe(ep, tmp, &dev->ep_list, ep_node) {
            count++;
        }
        // prepare routers
        routers = (char **)malloc(sizeof(char *)*count);
        if (NULL == routers) {
            N2N_ERROR("device %s[%s] failed to create routers for JSON",
                    dev->hostname, dev->instname);
            return MEMORY_MALLOC_FAILED;
        }
        memset(routers, 0, sizeof(char *)*count);

        int i = 0;
        n2n_ep *ep, *tmp;
        list_for_each_entry_safe(ep, tmp, &dev->ep_list, ep_node) {
            routers[i] = ep->route;
            i++;
        }
    }

    // doc
    cJSON *doc = cJSON_CreateObject();
    if (NULL == doc) {
        N2N_ERROR("save device %s[%s] failed to create JSON",
                dev->hostname, dev->instname);
        if (NULL != names) free(names);
        return N2N_DEV_JSON_FAILED;
    }
    // hostname
    if (NULL == cJSON_AddStringToObject(doc, "hostname",
                        (const char *)dev->hostname)) {
        N2N_ERROR("device %s[%s] encode hostname failed", dev->hostname, dev->instname);
        res = N2N_DEV_NAME_MISSED;
        goto clearup;
    }
    // instname
    if (NULL == cJSON_AddStringToObject(doc, "instname",
                        (const char *)dev->instname)) {
        N2N_ERROR("device %s[%s] encode instname failed", dev->hostname, dev->instname);
        res = N2N_DEV_NAME_MISSED;
        goto clearup;
    }
    // brand
    if (0 < strlen(dev->brand)) {
        if (NULL == cJSON_AddStringToObject(doc, "brand",
                            (const char *)dev->brand)) {
            N2N_ERROR("device %s[%s] encode brand failed", dev->hostname, dev->instname);
        }
    }
    // mfr
    if (0 < strlen(dev->mfr)) {
        if (NULL == cJSON_AddStringToObject(doc, "mfr",
                            (const char *)dev->mfr)) {
            N2N_ERROR("device %s[%s] encode mfr failed", dev->hostname, dev->instname);
        }
    }
    // model
    if (0 < strlen(dev->model)) {
        if (NULL == cJSON_AddStringToObject(doc, "model",
                            (const char *)dev->model)) {
            N2N_ERROR("device %s[%s] encode model failed", dev->hostname, dev->instname);
        }
    }
    // pd
    if (0 < strlen(dev->pd)) {
        if (NULL == cJSON_AddStringToObject(doc, "pd",
                            (const char *)dev->pd)) {
            N2N_ERROR("device %s[%s] encode pd failed", dev->hostname, dev->instname);
        }
    }
    // dev_type
    if (NULL == cJSON_AddNumberToObject(doc, "dev_type",
                        (int)dev->dev_type))
    {
        N2N_ERROR("device %s[%s] encode dev_type failed", dev->hostname, dev->instname);
        res = N2N_DEV_TYPE_MISSED;
        goto clearup;
    }
    N2N_DEBUG("device %s[%s] encoded basic", dev->hostname, dev->instname);
    if (!list_empty(&dev->ep_list)) {
        // children
        if (NULL == cJSON_AddItemToArray(doc, "entry_points",
                        cJSON_CreateStringArray(routers, count))) {
            N2N_ERROR("device %s[%s] encode entry points array failed",
                        dev->hostname, dev->instname);
            res = N2N_DEV_CHILDREN_FAILED;
            goto clearup;
        }

        // free routers
        for (int i = 0; i < count; i++) {
            if (NULL != routers[i]) free(routers[i]);
        }
        free(routers);

        N2N_DEBUG("device %s[%s] encoded entry points array",
                    dev->hostname, dev->instname);
    }
    *pbuff = cJSON_PrintUnformatted(doc);
    cJSON_Delete(doc); // release doc
    N2N_INFO("device %s[%s] json: %s", dev->hostname, dev->instname, *pbuff);
    return INNER_RES_OK;
clearup:
    cJSON_Delete(doc);
    if (NULL != routers) free(routers);
    return res;
}

/***
 * @description : malloc a new entry point
 * @param        {char} *route
 * @param        {char} *q_schema
 * @param        {char} *p_schema
 * @param        {active_task} *task - task for process requests
 * @return       {*}
 */
n2n_ep *n2n_ep_malloc(const char *route, const char *q_schema,
    const char *p_schema, active_task *task)
{
    n2n_ep *ep = (n2n_ep *)malloc(SIZE_N2N_EP);
    if (NULL == ep) {
        N2N_ERROR("failed to malloc entry point for %s", route);
        return NULL;
    }
    memset(ep, 0, SIZE_N2N_EP);
    INIT_LIST_HEAD(&ep->ep_node);
    ep->task = task;
    if (NULL != route) snprintf(ep->route, N2N_ROUTE_LEN, "%s", route);
    if (NULL != q_schema) snprintf(ep->q_schema, N2N_SCHEMA_LEN, "%s", q_schema);
    if (NULL != p_schema) snprintf(ep->p_schema, N2N_SCHEMA_LEN, "%s", p_schema);

    N2N_DEBUG("entry point %s malloc ok", route);
    if (NULL != g_n2n_proto_stack._init) {
        if (INNER_RES_OK != g_n2n_proto_stack._init(ep, g_n2n_proto_stack._arg)) {
            N2N_ERROR("entry point %s init failed, abandon", route);
            free(ep);
            return NULL;
        }
    }

    list_add_tail(&ep->ep_node, &g_n2n_proto_stack._eps);
    N2N_INFO("entry point %s malloc ok %p", route, ep);
    return ep;
}

/***
 * @description : free a entry point
 * @param        {n2n_ep} *ep - pointer to entry point
 * @return       {*}
 */
void n2n_ep_free(n2n_ep *ep)
{
    if (NULL == ep) return;
    N2N_DEBUG("entry point %s free %p ...", ep->route, ep);
    if (NULL != g_n2n_proto_stack._fini) g_n2n_proto_stack._fini(ep, g_n2n_proto_stack._arg);
    list_del(&ep->ep_node);
    N2N_INFO("entry point %s free %p OK", ep->route, ep);
    free(ep);
}

/***
 * @description : get entry point by route
 * @param        {char} *route - route
 * @return       {*}
 */
n2n_ep *n2n_ep_get_by_route(const char *route)
{
    if (!list_empty(&g_n2n_proto_stack._eps)) {
        n2n_ep *child, *tmp;
        list_for_each_entry_safe(child, tmp, &g_n2n_proto_stack._eps, ep_node) {
            if (0 == strcmp(route, child->route)) return child;
        }
    }
    return NULL;
}

/***
 * @description : get entry point list
 * @return       {*}
 */
struct list_head *n2n_ep_get_list(void)
{
    return &g_n2n_proto_stack._eps;
}

/***
 * @description : encode entry point information into a JSON object
 * @param        {n2n_ep} *ep - pointer to an entry point
 * @param        {char} **pbuff - pointer to pointer of a buff
 * @return       {*}
 */
at_error_t n2n_ep_to_json(n2n_ep *ep, char **pbuff)
{
    if (NULL == ep || NULL == pbuff) return INNER_INVAILD_PARAM;

    at_error_t res = ESP_FAIL;
    *pbuff = NULL;
    // doc
    cJSON *doc = cJSON_CreateObject();
    if (NULL == doc) {
        N2N_ERROR("save entry point %s failed to create JSON", ep_name);
        return N2N_DEV_JSON_FAILED;
    }
    // route
    if (NULL == cJSON_AddStringToObject(doc, "route",
                        (const char *)ep->route)) {
        N2N_ERROR("entry point %s encode route failed", ep_name);
        res = N2N_DEV_ROUTE_MISSED;
        goto clearup;
    }
    // q_schema
    if (NULL == cJSON_AddStringToObject(doc, "q_schema",
                        (const char *)ep->q_schema)) {
        N2N_ERROR("entry point %s encode q_schema failed", ep_name);
        res = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    // p_schema
    if (NULL == cJSON_AddStringToObject(doc, "p_schema",
                        (const char *)dev->p_schema)) {
        N2N_ERROR("entry point %s encode p_schema failed", ep_name);
        res = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    *pbuff = cJSON_PrintUnformatted(doc);
    cJSON_Delete(doc); // release doc
    N2N_INFO("entry point %s to json: %s", ep_name, *pbuff);
    return INNER_RES_OK;
clearup:
    cJSON_Delete(doc); // release doc
    return res;
}
