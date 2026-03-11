#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef enum {
    WIFI_LED_OFF,
    WIFI_LED_STA_CONNECTING,
    WIFI_LED_STA_ERROR,
    WIFI_LED_IP_RECEIVED,
    WIFI_LED_ONLINE,
    WIFI_LED_AP_STARTED,
    WIFI_LED_AP_CLIENT
} wifi_led_state_t;

typedef enum {
    LED_CMD_REMOTE_MODE,
    LED_CMD_SET_COLOR,
    LED_CMD_POWER,
    LED_CMD_LOCK
} led_cmd_type_t;

typedef enum {
    LED_REMOTE_AUTO = 0,
    LED_REMOTE_ON,
    LED_REMOTE_OFF
} led_remote_mode_t;

typedef struct {
    led_cmd_type_t type;
    led_remote_mode_t mode;  
    uint8_t r, g, b;        
    uint8_t priority;        
    bool lock;               
} led_cmd_t;

QueueHandle_t led_service_get_queue(void);

void led_service_start(void);

void led_service_toggle_mode(void);
bool led_service_is_circle_mode(void);

void led_service_set_wifi_state(wifi_led_state_t state);

void led_send_remote_command(led_remote_mode_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t priority);