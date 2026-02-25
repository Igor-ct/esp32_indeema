idf_component_register(SRCS "BLE_BT.c"
                    REQUIRES bt nvs_flash led2 Joy_stick_control_led
                    INCLUDE_DIRS "include")
