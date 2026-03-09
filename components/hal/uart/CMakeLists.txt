idf_component_register(SRCS "uart.c"
                    REQUIRES esp_driver_uart esp_driver_gpio json_parser led_service https_ota motor_service
                    INCLUDE_DIRS "include")
