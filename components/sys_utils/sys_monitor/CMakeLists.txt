idf_component_register(SRCS "sys_monitor.c"
                    PRIV_REQUIRES spi_flash esp_hw_support esp_timer
                    INCLUDE_DIRS "include")
