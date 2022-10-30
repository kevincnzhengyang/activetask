/*
 * @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @Date        : 2022-10-29 18:29:49
 * @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
 * @LastEditTime: 2022-10-30 23:40:24
 * @FilePath    : /activetask/components/blackboard/blackboard.c
 * @Description :
 * Copyright (c) 2022 by Zheng, Yang, All Rights Reserved.
 */
#include <stdatomic.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "mbedtls/md5.h"

#include "inner_err.h"
#include "linux_llist.h"

#include "blackboard.h"

#define BB_TAG "BlackBoard"
#define BB_DEBUG(fmt, ...)  ESP_LOGD(BB_TAG, fmt, ##__VA_ARGS__)
#define BB_INFO(fmt, ...)   ESP_LOGI(BB_TAG, fmt, ##__VA_ARGS__)
#define BB_WARN(fmt, ...)   ESP_LOGW(BB_TAG, fmt, ##__VA_ARGS__)
#define BB_ERROR(fmt, ...)  ESP_LOGE(BB_TAG, fmt, ##__VA_ARGS__)

#define BB_MAP_SIZE (1<<BLACKBOARD_MAP_BITS)

typedef struct
{
    char                          *key;
    bool                     persisted;
    size_t                   data_size;
    struct llist_node             node;
    void                       *rd_ptr;
} bb_datablk;

#define SIZE_BB_DATABLK     sizeof(bb_datablk)

typedef struct
{
    size_t                   buff_size;
    struct llist_head bb_map[BB_MAP_SIZE];
    void                         *base;
    atomic_int                  wr_pos;
    nvs_handle_t                  hnvs;
} bb_datamap;

static bb_datamap g_bb_map;

/***
 * @description : init black board
 * @param        {size_t} buff_size - size of black board
 * @return       {*}
 */
void blackboard_init(size_t buff_size)
{
    if (NULL != g_bb_map.base) return;  // already inited
    g_bb_map.buff_size = buff_size;
    g_bb_map.base = malloc(buff_size);
    assert("Failed to malloc for Black Board" && NULL != g_bb_map.base);
    g_bb_map.wr_pos = ATOMIC_VAR_INIT(0);
    for (int i = 0; i < BB_MAP_SIZE; i++) {
        init_llist_head(&g_bb_map.bb_map[i]);
    }

    // open NVS
    esp_err_t err = nvs_open("BlackBoard", NVS_READWRITE, &g_bb_map.hnvs);
    assert("Failed to open NVS" && ESP_OK == err);

    // find blackboard namespace in partition NVS_DEFAULT_PART_NAME (“nvs”)
    nvs_iterator_t it;
    nvs_entry_find(NVS_DEFAULT_PART_NAME, "BlackBoard", NVS_TYPE_BLOB, &it);
    err = nvs_entry_find(NVS_DEFAULT_PART_NAME,
                    "BlackBoard", NVS_TYPE_BLOB, &it);
    if (ESP_ERR_NVS_NOT_FOUND == err)
    {
        //  not exists
        nvs_release_iterator(it);
        BB_INFO("failed find data for BlackBoard");
    }
    else if (ESP_OK == err)
    {
        nvs_entry_info_t info;
        BB_INFO("open NVS ok");

        size_t cap = g_bb_map.buff_size;
        size_t length = cap;
        while (NULL != it)
        {
            nvs_entry_info(it, &info);
            if(ESP_OK != (err = nvs_entry_next(&it)))
            {
                BB_ERROR("next %s failed due to %d", info.key, err);
                ESP_ERROR_CHECK(nvs_flash_erase());
                nvs_flash_init();
                assert(false);
            }
            BB_INFO("loading %s from NVS", info.key);
            if (ESP_OK != (err = nvs_get_blob(g_bb_map.hnvs, info.key,
                    g_bb_map.base + atomic_load(&g_bb_map.wr_pos), &length)))
            {
                BB_ERROR("load %s failed due to %d", info.key, err);
                ESP_ERROR_CHECK(nvs_flash_erase());
                nvs_flash_init();
                assert(false);
            }
            // move wr_pos in blackboard_register

            err = blackboard_register(info.key, length, true);
            if (ESP_OK != err)
            {
                BB_ERROR("register %s failed due to %d", info.key, err);
                assert(false);
            }
            cap -= length;
            length = cap;
        }
    }
    else
    {
        BB_ERROR("failed find due to %d", err);
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
        assert(false);
    }

    BB_INFO("init ok");
}

/***
 * @description : fini black board
 * @return       {*}
 */
void blackboard_fini(void)
{
    if (NULL == g_bb_map.base) return;
    nvs_close(g_bb_map.hnvs);
    free(g_bb_map.base);
    g_bb_map.wr_pos = ATOMIC_VAR_INIT(0);

    for (int i = 0; i < BB_MAP_SIZE; i++) {
        while (!llist_empty(&g_bb_map.bb_map[i])) {
            struct llist_node *pnode = llist_del_first(&g_bb_map.bb_map[i]);
            bb_datablk *db = llist_entry(pnode, bb_datablk, node);
            BB_INFO("recycle %s", db->key);
            free(db->key);
            free(db);
        }
    }
}

static inline unsigned int hash_min(const char *val, size_t bits)
{
    mbedtls_md5_context ctx;
    unsigned char decrypt[16];

    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, (unsigned char *)val, strlen(val));
    mbedtls_md5_finish(&ctx, decrypt);
    mbedtls_md5_free(&ctx);
    BB_DEBUG("%s md5 is:[", val);
    for (int i = 0; i < 16; i++)
        BB_DEBUG("%02x", decrypt[i]);
    BB_DEBUG("]\n");

    uint32_t digest = (decrypt[3] << 24 | decrypt[2] << 16 \
                        | decrypt[1] << 8 | decrypt[0]) >> (32 - bits);
    BB_DEBUG("%s md5 digest is:<%lx>", val, digest);
    return digest;
}

/***
 * @description : get data from black board
 * @param        {char} *key - name of data
 * @return       {*}
 */
void *blackboard_get(const char *key)
{
    struct llist_head mhead = g_bb_map.bb_map[hash_min(key, BLACKBOARD_MAP_BITS)];

    if (llist_empty(&mhead)) return NULL;

    bb_datablk *db = NULL;
    llist_for_each_entry(db, &mhead, node) {
        if (0 == strcmp(db->key, key)) {
            BB_DEBUG("find %s at %p", key, db);
            return db->rd_ptr;
        }
    }
    return NULL;
}

static bool has_space(size_t s)
{
    return g_bb_map.buff_size - atomic_load(&g_bb_map.wr_pos) >= s;
}

/***
 * @description : register data into black board
 * @param        {char} *key - name of data
 * @param        {size_t} data_size - size of data
 * @param        {bool} persisted - if data is persisted
 * @return       {*}
 */
at_error_t blackboard_register(const char *key, size_t data_size,
        bool persisted)
{
    if (NULL == key) return INNER_INVAILD_PARAM;

    bb_datablk *db = (bb_datablk *)malloc(SIZE_BB_DATABLK);
    if (NULL == db) {
        BB_ERROR("failed to malloc for bb datablk");
        return BB_LACK_SPACE;
    }
    db->key = strdup(key);
    db->persisted = persisted;
    db->data_size = data_size;

    if (!has_space(data_size)) {
        BB_ERROR("failed to check space");
        free(db);
        return BB_LACK_SPACE;
    }
    int pos = atomic_fetch_add(&g_bb_map.wr_pos, data_size);
    // check before wirte
    if (g_bb_map.buff_size - pos < data_size) {
        // other writer move wr_pos before this fetch
        BB_ERROR("failed to double check space");
        free(db);
        return BB_LACK_SPACE;
    }
    db->rd_ptr = g_bb_map.base + pos;   // assgin again

    struct llist_head mhead = g_bb_map.bb_map[hash_min(key, BLACKBOARD_MAP_BITS)];
    llist_add(&db->node, &mhead);   // add to hash map
    return INNER_RES_OK;
}

/***
 * @description : flush black board into nvs
 * @param        {char} *key - name of data
 * @return       {*}
 */
at_error_t blackboard_flush(const char *key)
{
    if (NULL == key) return INNER_INVAILD_PARAM;

    void *rd = blackboard_get(key);
    if (NULL == rd) return BB_KEY_NOT_EXIST;

    bb_datablk *db = container_of(rd, bb_datablk, rd_ptr);

    // save data to NVS
    esp_err_t err = nvs_set_blob(g_bb_map.hnvs, key, db->rd_ptr, db->data_size);
    return ESP_OK != err ? err : nvs_commit(g_bb_map.hnvs);
}
