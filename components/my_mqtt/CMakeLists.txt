idf_component_register(SRCS "my_mqtt.c"
                    REQUIRES mqtt esp-tls esp_wifi esp_event nvs_flash cjson led2
                    INCLUDE_DIRS "include")
