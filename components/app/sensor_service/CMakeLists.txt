idf_component_register(SRCS "sensor_service.c"
                    REQUIRES aht20 bmp280 lsmd6ds3 i2c spi
                    INCLUDE_DIRS "include")
