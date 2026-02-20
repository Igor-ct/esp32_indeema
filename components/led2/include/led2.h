#pragma once
#include <stdio.h>
#include "esp_err.h"
#include "led_strip.h"


typedef enum {
    WIFI_LED_OFF,
    WIFI_LED_STA_CONNECTING,
    WIFI_LED_STA_ERROR,
    WIFI_LED_IP_RECEIVED,
    WIFI_LED_ONLINE,
    WIFI_LED_AP_STARTED,
    WIFI_LED_AP_CLIENT,
} wifi_led_state_t;

void led2_set_wifi_state(wifi_led_state_t state);

esp_err_t led2_init(void);
esp_err_t led2_set_rgb(uint8_t r, uint8_t g, uint8_t b);
esp_err_t led2_deinit(void);
void led2_toggle_lock(void);

bool led2_is_initialized(void);
bool led2_is_lock(void);

void led2_set_block_wifi(void);
void led2_set_unblock_wifi(void);
wifi_led_state_t led2_get_wifi_state(void);

void led2_set_wifi_style(uint8_t r, uint8_t g, uint8_t b);