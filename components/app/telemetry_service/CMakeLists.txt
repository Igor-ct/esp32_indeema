idf_component_register(SRCS "telemetry_service.c"
                    REQUIRES ble mqtt_wrapper uart sensor_service motor_service
                    INCLUDE_DIRS "include")
