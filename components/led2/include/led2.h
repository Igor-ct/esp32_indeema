#pragma once
#include <stdio.h>
#include "esp_err.h"
#include "led_strip.h"




esp_err_t led2_init(void);
esp_err_t led2_set_rgb(uint8_t r, uint8_t g, uint8_t b);
esp_err_t led2_deinit(void);
void led2_toggle_lock(void);
bool led2_is_initialized(void);
