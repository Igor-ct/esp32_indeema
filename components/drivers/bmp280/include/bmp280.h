#pragma once

#include "esp_err.h"
#include <stdint.h>

esp_err_t bmp280_init(void);
esp_err_t bmp280_read_raw(int32_t *raw_temp, int32_t *raw_press);