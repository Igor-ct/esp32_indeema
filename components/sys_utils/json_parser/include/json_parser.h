#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"


typedef enum {
    JSON_LED_STATE_NONE = 0, 
    JSON_LED_STATE_ON,
    JSON_LED_STATE_OFF,
    JSON_LED_STATE_AUTO
} json_led_state_t;


typedef struct {
    bool has_color;      
    uint8_t r;
    uint8_t g;
    uint8_t b;
    json_led_state_t state; 
} parsed_led_cmd_t;


esp_err_t json_parse_led_command(const char *json_string, parsed_led_cmd_t *out_cmd);