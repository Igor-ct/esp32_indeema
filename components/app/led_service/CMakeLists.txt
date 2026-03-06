idf_component_register(SRCS "led_service.c"
                    REQUIRES ws2812 joystick_controller joystick_button mqtt_wrapper ble uart
                    INCLUDE_DIRS "include")
