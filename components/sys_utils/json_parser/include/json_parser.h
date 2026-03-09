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

typedef struct {
    float angle;
    int mode;       
    bool has_angle;
    bool has_mode;
} parsed_motor_cmd_t;

typedef struct {
    bool has_url;
    char url[256]; 
} parsed_ota_cmd_t;

esp_err_t json_parse_motor_command(const char *json_string, parsed_motor_cmd_t *out_cmd);

esp_err_t json_parse_led_command(const char *json_string, parsed_led_cmd_t *out_cmd);

esp_err_t json_parse_ota_command(const char *json_string, parsed_ota_cmd_t *out_cmd);