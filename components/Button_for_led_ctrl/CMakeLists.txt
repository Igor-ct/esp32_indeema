idf_component_register(SRCS "Button_for_led_ctrl.c"
                    REQUIRES button
                    REQUIRES led2
                    REQUIRES Joy_stick_control_led
                    INCLUDE_DIRS "include")
