idf_component_register(SRCS "sntp_sync.c"
                    REQUIRES esp_netif lwip
                    INCLUDE_DIRS "include")
