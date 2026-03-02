#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define JOYSTICK_ENABLE_WIFI_MODE   CONFIG_JOYSTICK_ENABLE_WIFI_MODE
#define JOYSTICK_ENABLE_CIRCLE_MODE CONFIG_JOYSTICK_ENABLE_CIRCLE_MODE
#define JOYSTICK_UPDATE_INTERVAL CONFIG_JOYSTICK_UPDATE_INTERVAL

typedef struct {
    float x; 
    float y; 
} joystick_pos_t;

typedef enum {
    JOYSTICK_ZONE_CENTER,
    JOYSTICK_ZONE_LEFT,
    JOYSTICK_ZONE_RIGHT,
    JOYSTICK_ZONE_DOWN
} joystick_zone_t;

QueueHandle_t joystick_get_pos_queue(void);
QueueHandle_t joystick_get_zone_queue(void);

esp_err_t joystick_init(void);
void joystick_toggle_inversion(void);