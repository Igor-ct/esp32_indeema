idf_component_register(SRCS "ble.c"
                    REQUIRES bt nvs_flash ws2812 led_service motor_service https_ota
                    INCLUDE_DIRS "include")
