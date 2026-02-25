#pragma once
#include "esp_err.h"
#include "esp_log.h"

esp_err_t joystick_led_init(void);

typedef struct {
    float x; 
    float y; 
} joystick_pos_t;


esp_err_t joystick_read(joystick_pos_t *pos);

void joystick_update_task(void *pvParameters);

void joystick_mode_switch(void);

void joystick_toggle_inversion(void);