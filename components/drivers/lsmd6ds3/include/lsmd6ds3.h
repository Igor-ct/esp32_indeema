#pragma once

#include "esp_err.h"
#include <stdint.h>

esp_err_t lsm6ds3_init(int cs_pin);
esp_err_t lsm6ds3_read_accel(int16_t *x, int16_t *y, int16_t *z);