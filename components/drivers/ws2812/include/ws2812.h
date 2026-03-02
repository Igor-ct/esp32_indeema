#pragma once

#include "esp_err.h"
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_cmd_t;

esp_err_t ws2812_init(void);
esp_err_t ws2812_set_rgb(uint8_t r, uint8_t g, uint8_t b);
esp_err_t ws2812_clear(void); 