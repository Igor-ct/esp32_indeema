idf_component_register(SRCS "uart.c"
                    REQUIRES esp_driver_uart esp_driver_gpio json_parser ws2812
                    INCLUDE_DIRS "include")
