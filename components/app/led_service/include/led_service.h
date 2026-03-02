#pragma once

#include <stdbool.h>

typedef enum {
    WIFI_LED_OFF,
    WIFI_LED_STA_CONNECTING,
    WIFI_LED_STA_ERROR,
    WIFI_LED_IP_RECEIVED,
    WIFI_LED_ONLINE,
    WIFI_LED_AP_STARTED,
    WIFI_LED_AP_CLIENT
} wifi_led_state_t;

void led_service_start(void);

void led_service_toggle_mode(void);
bool led_service_is_circle_mode(void);

void led_service_set_wifi_state(wifi_led_state_t state);
