idf_component_register(SRCS "telemetry_service.c"
                    REQUIRES ble mqtt_wrapper uart lsmd6ds3 bmp280 aht20 i2c spi
                    INCLUDE_DIRS "include")
