idf_component_register(SRCS "wifi_service.c"
                    REQUIRES led_service wifi_ap wifi_sta esp_wifi joystick_controller
                    INCLUDE_DIRS "include")
