idf_component_register(SRCS "Joy_stick_control_led.c" 
                    REQUIRES esp_adc
                    REQUIRES led2
                    REQUIRES Button_for_led_ctrl
                    REQUIRES wifi_ap
                    REQUIRES wifi_sta
                    REQUIRES esp_wifi
                    REQUIRES my_mqtt BLE_BT my_UART
                    INCLUDE_DIRS "include")
