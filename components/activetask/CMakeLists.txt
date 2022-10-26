idf_component_register(SRCS "circ_queue.c" "data_blk.c" "mem_blk.c"
                            "msg_blk.c" "active_task.c"
                    INCLUDE_DIRS "."
                    REQUIRES mbedtls)