idf_component_register(SRCS "SNTP_sync.c"
                    REQUIRES esp_netif lwip
                    INCLUDE_DIRS "include")
