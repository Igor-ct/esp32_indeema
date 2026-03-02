idf_component_register(SRCS "wifi_sta.c"
                    PRIV_REQUIRES esp_wifi nvs_flash bt sntp_sync http_server  mqtt_wrapper led_service
                    INCLUDE_DIRS "include")
