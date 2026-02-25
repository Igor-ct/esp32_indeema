#pragma once
#include "esp_err.h"
#include "esp_log.h"

#define BUTTON_EVENT_PRESS_DOWN       BUTTON_PRESS_DOWN
#define BUTTON_EVENT_SINGLE_CLICK     BUTTON_SINGLE_CLICK
#define BUTTON_EVENT_DOUBLE_CLICK     BUTTON_DOUBLE_CLICK
#define BUTTON_EVENT_LONG_PRESS       BUTTON_LONG_PRESS_START



esp_err_t button_for_led_ctrl_init(void);