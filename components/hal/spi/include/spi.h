#pragma once

#include "driver/spi_master.h"

void spi_bus_init(void);

spi_host_device_t spi_bus_get_host(void);

