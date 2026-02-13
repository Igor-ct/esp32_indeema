idf_component_register(SRCS "Joy_stick_control_led.c" 
                    REQUIRES esp_adc
                    REQUIRES led2
                    REQUIRES Button_for_led_ctrl
                    INCLUDE_DIRS "include")
