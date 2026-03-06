idf_component_register(SRCS "mqtt_wrapper.c"
                    REQUIRES mqtt esp-tls esp_wifi esp_event nvs_flash led_service json_parser motor_service
                    INCLUDE_DIRS "include")
