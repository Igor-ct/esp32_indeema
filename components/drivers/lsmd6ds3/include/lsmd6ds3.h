#pragma once

#include "esp_err.h"
#include <stdint.h>

esp_err_t lsm6ds3_init(int cs_pin);
esp_err_t lsm6ds3_read_accel(float *x, float *y, float *z);