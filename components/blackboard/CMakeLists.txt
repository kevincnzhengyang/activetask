idf_component_register(SRCS "blackboard.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash activetask mbedtls)