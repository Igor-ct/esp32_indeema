#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define BUTTON_EVENT_SINGLE_CLICK  BUTTON_SINGLE_CLICK
#define BUTTON_EVENT_DOUBLE_CLICK BUTTON_DOUBLE_CLICK
#define BUTTON_EVENT_PRESS_DOWN BUTTON_PRESS_DOWN
#define BUTTON_EVENT_LONG_PRESS BUTTON_LONG_PRESS_START

typedef enum {
    BTN_EVT_SINGLE_CLICK,
    BTN_EVT_DOUBLE_CLICK,
    BTN_EVT_LONG_PRESS,
    BTN_EVT_PRESS_DOWN
} joystick_button_event_t;


QueueHandle_t joystick_button_get_queue(void);

esp_err_t joystick_button_init(void);