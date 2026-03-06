idf_component_register(SRCS "platform_init.c"
                    REQUIRES nvs_flash joystick_button joystick_controller uart ws2812 
                    INCLUDE_DIRS "include")
