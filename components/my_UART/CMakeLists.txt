idf_component_register(SRCS "my_UART.c"
                    REQUIRES esp_driver_uart esp_driver_gpio cjson led2
                    INCLUDE_DIRS "include")
