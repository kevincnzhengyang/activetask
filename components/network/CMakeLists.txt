idf_component_register(SRCS "wifi_prov.c" "mqtt_task.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash driver blackboard esp_event esp_wifi mqtt wifi_provisioning qrcode)