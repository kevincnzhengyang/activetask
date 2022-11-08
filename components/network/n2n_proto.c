/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-11-05 12:03:53
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-11-08 00:24:41
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
    n2n_instance              instance;
    on_n2n_device_init           _init;
    on_n2n_device_fini           _fini;
    void                         *_arg;
    struct list_head             _devs;
    nvs_handle_t                  hnvs;
};

static struct _n2n_proto_stack g_n2n_proto_stack;

/***
 * @description : protocol stack init, automatically load devices stored in NVS
 * @param        {on_n2n_device_init} on_init - callback function for init
 * @param        {on_n2n_device_fini} on_fini - callback function for fini
 * @param        {void} *arg - user defined argument for callback functions
 * @return       {*}
 */
at_error_t n2n_proto_init(on_n2n_device_init on_init, on_n2n_device_fini on_fini, void *arg)
{
    g_n2n_proto_stack._init = on_init;
    g_n2n_proto_stack._fini = on_fini;
    g_n2n_proto_stack._arg = arg;
    INIT_LIST_HEAD(&g_n2n_proto_stack._devs);

    // open NVS
    esp_err_t err = nvs_open("N2N", NVS_READWRITE, &g_n2n_proto_stack.hnvs);
    if (ESP_OK != err) return err;

    // find N2N namespace in partition NVS_DEFAULT_PART_NAME (“nvs”)
    nvs_iterator_t it;
    nvs_entry_find(NVS_DEFAULT_PART_NAME, "N2N", NVS_TYPE_BLOB, &it);
    err = nvs_entry_find(NVS_DEFAULT_PART_NAME, "N2N", NVS_TYPE_BLOB, &it);
    if (ESP_ERR_NVS_NOT_FOUND == err) {
        //  not exists
        nvs_release_iterator(it);
        N2N_INFO("not stored data for N2N");
    } else if (ESP_OK == err) {
        nvs_entry_info_t info;
        N2N_INFO("open NVS ok");

        n2n_device *dev;
        while (NULL != it) {
            nvs_entry_info(it, &info);
            if(ESP_OK != (err = nvs_entry_next(&it))) {
                N2N_ERROR("next %s failed due to %d", info.key, err);
                nvs_flash_erase();
                nvs_flash_init();
                return err;
            }
            N2N_DEBUG("loading %s from NVS", info.key);

            if (INNER_RES_OK != (err = n2n_device_load(info.key, &dev))) {
                N2N_ERROR("load %s failed due to %d", info.key, err);
                nvs_flash_erase();
                nvs_flash_init();
                return err;
            } else {
                N2N_INFO("device %s[%s] loaded", dev->instance, dev->dev_name);
            }
        }
    } else {
        N2N_ERROR("failed find data due to %d", err);
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
        assert(false);
    }

    return INNER_RES_OK;
}

/***
 * @description : protocol stack clear
 * @return       {*}
 */
void n2n_proto_fini(void)
{
    nvs_close(g_n2n_proto_stack.hnvs);
    n2n_device *dev, *tmp;
    list_for_each_entry_safe(dev, tmp, &g_n2n_proto_stack._devs, dev_node) {
        n2n_device_free(dev);
    }
}

/***
 * @description : malloc a new device
 * @param        {n2n_dev_enum} *dtype - type of device
 * @param        {char} *instance - inner instantce name
 * @param        {char} *name - name for N2N
 * @param        {char} *brand
 * @param        {char} *mfr
 * @param        {char} *model
 * @param        {char} *pd
 * @param        {char} *route
 * @param        {char} *q_schema
 * @param        {char} *p_schema
 * @param        {active_task} *task - task for process requests
 * @param        {n2n_device} *parent - pointer to parent device
 * @return       {*}
 */
n2n_device *n2n_device_malloc(n2n_dev_enum *dtype, const char *instance,
    const char *name, const char *brand, const char *mfr, const char *model,
    const char *pd, const char *route, const char *q_schema,
    const char *p_schema, active_task *task, n2n_device *parent)
{
    if (NULL == instance) return NULL;
    n2n_device *dev = (n2n_device *)malloc(SIZE_N2N_DEVICE);
    if (NULL == dev) {
        N2N_ERROR("failed to malloc dev for %s", instance);
        return NULL;
    }
    memset(dev, 0, SIZE_N2N_DEVICE);
    INIT_LIST_HEAD(&dev->dev_list);
    INIT_LIST_HEAD(&dev->dev_node);
    snprintf(dev->instance, N2N_NAME_LEN, "%s", instance);
    dev->dev_type = dtype;
    dev->task = task;
    if (NULL != name) snprintf(dev->dev_name, N2N_NAME_LEN, "%s", name);
    if (NULL != brand) snprintf(dev->brand, N2N_NAME_LEN, "%s", brand);
    if (NULL != mfr) snprintf(dev->mfr, N2N_NAME_LEN, "%s", mfr);
    if (NULL != model) snprintf(dev->model, N2N_NAME_LEN, "%s", model);
    if (NULL != pd) snprintf(dev->pd, N2N_DATE_LEN, "%s", pd);
    if (NULL != route) snprintf(dev->route, N2N_ROUTE_LEN, "%s", route);
    if (NULL != q_schema) snprintf(dev->q_schema, N2N_SCHEMA_LEN, "%s", q_schema);
    if (NULL != p_schema) snprintf(dev->p_schema, N2N_SCHEMA_LEN, "%s", p_schema);

    N2N_DEBUG("device %s[%s] malloc ok", instance, name);
    if (NULL != g_n2n_proto_stack._init) {
        if (INNER_RES_OK != g_n2n_proto_stack._init(dev, g_n2n_proto_stack._arg)) {
            N2N_ERROR("device %s init failed, abandon", instance);
            free(dev);
            return NULL;
        }
    }

    if (NULL == parent) {
        // add as root dev
        list_add_tail(&dev->dev_node, &g_n2n_proto_stack._devs);
    } else {
        list_add_tail(&dev->dev_node, &parent->dev_list);
    }
    N2N_INFO("device %s[%s] malloc ok %p", instance, name, dev);
    return dev;
}

/***
 * @description : free a device
 * @param        {n2n_device} *dev - pointer to device
 * @return       {*}
 */
void n2n_device_free(n2n_device *dev)
{
    if (NULL == dev) return;
    N2N_DEBUG("device %s[%s] free %p ...", dev->instance, dev->dev_name, dev);
    if (!list_empty(&dev->dev_list)) {
        n2n_device *child, *tmp;
        list_for_each_entry_safe(child, tmp, &dev->dev_list, dev_node) {
            n2n_device_free(child);
        }
    }
    if (NULL != g_n2n_proto_stack._fini) g_n2n_proto_stack._fini(dev, g_n2n_proto_stack._arg);
    list_del(&dev->dev_node);
    N2N_INFO("device %s[%s] free %p OK", dev->instance, dev->dev_name, dev);
    free(dev);
}

/***
 * @description : load a device from NVS
 * @param        {char} *instance - inner instance name
 * @param        {n2n_device} **pdev - pointer to pointer of a device
 * @param        {n2n_device} *parent - pointer to parent device
 * @return       {*}
 */
at_error_t n2n_device_load(const char *instance, n2n_device **pdev, n2n_device *parent)
{
    if (NULL == instance || NULL == pdev) return INNER_INVAILD_PARAM;
    char buff[1024] = "\0";
    size_t buff_len = 1024;

    // get data from NVS
    esp_err_t err = nvs_get_str(g_n2n_proto_stack.hnvs, info.key,
                            buff, &buff_len);
    if (ESP_OK != err) return err;

    if (0 == buff_len) {
        N2N_ERROR("empty device %s", instance);
        return N2N_EMPTY_DEV_INFO;
    }

    n2n_device *dev = n2n_device_malloc(N2N_DEV_BUTT, instance,
                        NULL, NULL, NULL, NULL,
                        NULL, NULL, NULL,
                        NULL, NULL, parent);
    if (NULL == dev) return N2N_DEV_MALLOC_FAILED;

    cJSON *pdoc = NULL, *pvalue = NULL;
    // parse JSON
    if (NULL == (pdoc = cJSON_ParseWithLength(buff, buff_len)))
    {
        N2N_ERROR("parse action in JSON failed, raw=%.*s", buff_len, buff);
        n2n_device_free(dev);
        return N2N_DEV_PARSE_FAILED;
    }
    // name
    pvalue = cJSON_GetObjectItem(pdoc, "name");
    if (NULL == pvalue || !cJSON_IsString(pvalue))
    {
        N2N_ERROR("device without name");
        err = N2N_DEV_NAME_MISSED;
        goto clearup;
    }
    snprintf(dev->name, N2N_NAME_LEN, pvalue->valuestring);
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
    // route
    pvalue = cJSON_GetObjectItem(pdoc, "route");
    if (NULL == pvalue || !cJSON_IsString(pvalue))
    {
        N2N_ERROR("device without route");
        err = N2N_DEV_ROUTE_MISSED;
        goto clearup;
    }
    snprintf(dev->route, N2N_ROUTE_LEN, pvalue->valuestring);
    // q_schema
    pvalue = cJSON_GetObjectItem(pdoc, "q_schema");
    if (NULL == pvalue || !cJSON_IsString(pvalue))
    {
        N2N_ERROR("device without q_schema");
        err = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    snprintf(dev->q_schema, N2N_SCHEMA_LEN, pvalue->valuestring);
    // p_schema
    pvalue = cJSON_GetObjectItem(pdoc, "p_schema");
    if (NULL == pvalue || !cJSON_IsString(pvalue))
    {
        N2N_ERROR("device without p_schema");
        err = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    snprintf(dev->p_schema, N2N_SCHEMA_LEN, pvalue->valuestring);
    // dev_type
    pvalue = cJSON_GetObjectItem(pdoc, "dev_type");
    if (NULL == pvalue || !cJSON_IsNumber(pvalue))
    {
        N2N_ERROR("device without dev_type");
        err = N2N_DEV_TYPE_MISSED;
        goto clearup;
    }
    snprintf(dev->p_schema, N2N_SCHEMA_LEN, pvalue->valuestring);
    dev->dev_type = (n2n_dev_enum)pvalue->valueint;

    // get children
    cJSON *children = cJSON_GetObjectItem(pdoc, "children");
    if (NULL == children || !cJSON_IsArray(children))
    {
        return INNER_RES_OK;
    }

    // itera all children
    for (int j = 0; j < cJSON_GetArraySize(children); j++)
    {
        n2n_device *child = NULL;
        pvalue = cJSON_GetArrayItem(children, j);
        if (NULL == pvalue || !cJSON_IsString(pvalue))
        {
            N2N_ERROR("device %s[%s] child %d without string",
                    dev->instance, dev->dev_name, j);
            err = N2N_DEV_CHILD_MISSED;
            goto clearup;
        }
        // load the child with this dev as parent
        err = n2n_device_load(pvalue->valuestring, &child, dev);
        if (INNER_RES_OK != err) goto clearup;

    return INNER_RES_OK

clearup:
    n2n_device_free(dev);
    cJSON_Delete(pdoc);
    return err;
}

/***
 * @description : save a device to NVS
 * @param        {n2n_device} *dev
 * @return       {*}
 */
at_error_t n2n_device_save(n2n_device *dev)
{
    if (NULL == dev) return INNER_INVAILD_PARAM;

    at_error_t res = ESP_FAIL;
    char **names = NULL;
    int count = 0;

    // save children first
    if (!list_empty(&dev->dev_list)) {
        n2n_device *child, *tmp;
        list_for_each_entry_safe(child, tmp, &dev->dev_list, dev_node) {
            if (INNER_RES_OK != (res = n2n_device_save(child))) return res;
            count++;
        }

        names = (char **)malloc(sizeof(char *)*count);
        if (NULL == names) {
            N2N_ERROR("save device %s[%s] failed to create names",
                    dev->instance, dev->dev_name);
            return MEMORY_MALLOC_FAILED;
        }
        int i = 0;
        list_for_each_entry_safe(child, tmp, &dev->dev_list, dev_node) {
            names[i] = child->instance;
            i++;
        }
    }
    // doc
    cJSON *pdoc = cJSON_CreateObject();
    if (NULL == pdoc) {
        N2N_ERROR("save device %s[%s] failed to create JSON",
                dev->instance, dev->dev_name);
        if (NULL != names) free(names);
        return N2N_DEV_JSON_FAILED;
    }
    // name
    if (NULL == cJSON_AddStringToObject(pdoc, "name",
                        (const char *)dev->name)) {
        N2N_ERROR("device %s[%s] encode name failed", dev->instance, dev->name);
        res = N2N_DEV_NAME_MISSED;
        goto clearup;
    }
    // brand
    if (0 < strlen(dev->brand)) {
        if (NULL == cJSON_AddStringToObject(pdoc, "brand",
                            (const char *)dev->brand)) {
            N2N_ERROR("device %s[%s] encode brand failed", dev->instance, dev->name);
        }
    }
    // mfr
    if (0 < strlen(dev->mfr)) {
        if (NULL == cJSON_AddStringToObject(pdoc, "mfr",
                            (const char *)dev->mfr)) {
            N2N_ERROR("device %s[%s] encode mfr failed", dev->instance, dev->name);
        }
    }
    // model
    if (0 < strlen(dev->model)) {
        if (NULL == cJSON_AddStringToObject(pdoc, "model",
                            (const char *)dev->model)) {
            N2N_ERROR("device %s[%s] encode model failed", dev->instance, dev->name);
        }
    }
    // pd
    if (0 < strlen(dev->pd)) {
        if (NULL == cJSON_AddStringToObject(pdoc, "pd",
                            (const char *)dev->pd)) {
            N2N_ERROR("device %s[%s] encode pd failed", dev->instance, dev->name);
        }
    }
    // route
    if (NULL == cJSON_AddStringToObject(pdoc, "route",
                        (const char *)dev->route)) {
        N2N_ERROR("device %s[%s] encode route failed", dev->instance, dev->name);
        res = N2N_DEV_ROUTE_MISSED;
        goto clearup;
    }
    // q_schema
    if (NULL == cJSON_AddStringToObject(pdoc, "q_schema",
                        (const char *)dev->q_schema)) {
        N2N_ERROR("device %s[%s] encode q_schema failed", dev->instance, dev->name);
        res = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    // p_schema
    if (NULL == cJSON_AddStringToObject(pdoc, "p_schema",
                        (const char *)dev->p_schema)) {
        N2N_ERROR("device %s[%s] encode p_schema failed", dev->instance, dev->name);
        res = N2N_DEV_SCHEMA_MISSED;
        goto clearup;
    }
    // dev_type
    if (NULL == cJSON_AddNumberToObject(pdoc, "dev_type",
                        (int)dev->dev_type))
    {
        N2N_ERROR("device %s[%s] encode dev_type failed", dev->instance, dev->name);
        res = N2N_DEV_TYPE_MISSED;
        goto clearup;
    }
    N2N_DEBUG("device %s[%s] encoded basic", dev->instance, dev->name);
    if (!list_empty(&dev->dev_list)) {
        // children
        if (NULL == cJSON_AddItemToArray(pdoc, "children",
                        cJSON_CreateStringArray(names, count))) {
            N2N_ERROR("device %s[%s] encode children failed", dev->instance, dev->name);
            res = N2N_DEV_CHILDREN_FAILED;
            goto clearup;
        }
        free(names);    // release names
        N2N_DEBUG("device %s[%s] encoded children", dev->instance, dev->name);
    }
    char *pbuff = cJSON_PrintUnformatted(pdoc);
    cJSON_Delete(pdoc); // release pdoc
    res = nvs_set_str(g_n2n_proto_stack.hnvs, dev->instance, pbuff);
    N2N_INFO("device %s[%s] save as %s", dev->instance, dev->name, pbuff);
    free(pbuff);        // release encode buff
    return ESP_OK != err ? err : nvs_commit(g_n2n_proto_stack.hnvs);

clearup:
    cJSON_Delete(pdoc);
    if (NULL != names) free(names);
    return res;
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

    snprintf(dev->dev_name, N2N_NAME_LEN, "%s", name);
    return INNER_RES_OK;
}

/***
 * @description : encode device information into a JSON object
 * @param        {n2n_device} *dev - pointer to a device
 * @param        {cJSON} *doc - pointer to a JSON object
 * @return       {*}
 */
at_error_t n2n_device_to_json(n2n_device *dev, cJSON *doc)
{
    if (NULL == dev || NULL == doc) return INNER_INVAILD_PARAM;
}

static n2n_device *n2n_device_find_name(struct list_head *list, const char *name)
{
    if (!list_empty(list)) {
        n2n_device *child, *tmp;
        list_for_each_entry_safe(child, tmp, list, dev_node) {
            if (0 == strcmp(name, child->dev_name)) return child;
            if (list_empty(child->dev_list)) continue;
            n2n_device *dev = n2n_device_find_name(&child->dev_list, name);
            if (NULL != dev) return dev;
        }
    }
    return NULL;
}

/***
 * @description : get device by name
 * @param        {char} *name - name of device
 * @return       {*}
 */
n2n_device *n2n_device_get_by_name(const char *name)
{
    return n2n_device_find_name(&g_n2n_proto_stack._devs, name);
}

static n2n_device *n2n_device_find_instance(struct list_head *list, const char *instance)
{
    if (!list_empty(list)) {
        n2n_device *child, *tmp;
        list_for_each_entry_safe(child, tmp, list, dev_node) {
            if (0 == strcmp(instance, child->instance)) return child;
            if (list_empty(child->dev_list)) continue;
            n2n_device *dev = n2n_device_find_instance(&child->dev_list, instance);
            if (NULL != dev) return dev;
        }
    }
    return NULL;
}

/***
 * @description : get device by instance name
 * @param        {char} *instance - name of instance
 * @return       {*}
 */
n2n_device *n2n_device_get_by_instance(const char *instance)
{
    return n2n_device_find_instance(&g_n2n_proto_stack._devs, instance);
}
