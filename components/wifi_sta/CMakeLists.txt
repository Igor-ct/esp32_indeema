idf_component_register(SRCS "wifi_sta.c"
                    PRIV_REQUIRES esp_wifi nvs_flash bt SNTP_sync HTTP_server led2 my_mqtt
                    INCLUDE_DIRS "include")
