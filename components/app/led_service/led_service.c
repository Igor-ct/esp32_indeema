#include "led_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

#include "joystick_controller.h" 
#include "joystick_button.h"    
#include "ws2812.h"                
#include "uart.h"            
#include "ble.h"            
#include "mqtt_wrapper.h"            

static const char *TAG = "LED_SERVICE";

static bool rgb_circle_mode = JOYSTICK_ENABLE_CIRCLE_MODE;
static bool wifi_mode_enable = JOYSTICK_ENABLE_WIFI_MODE;

static bool led_power_on = true; 
static bool led_locked = false; 
static wifi_led_state_t current_wifi_state = WIFI_LED_OFF; 

void led_service_toggle_mode(void) {
    if (CONFIG_JOYSTICK_ENABLE_CIRCLE_MODE) {
        rgb_circle_mode = !rgb_circle_mode;
        ESP_LOGI(TAG, "RGB mode: %s", rgb_circle_mode ? "Circle" : "Zones");
    }
}

bool led_service_is_circle_mode(void) { return rgb_circle_mode; }

void led_service_set_wifi_state(wifi_led_state_t state) {
    current_wifi_state = state;
    ESP_LOGI(TAG, "WiFi LED State changed to: %d", state);
}

static void led_service_task(void *pvParameters) {
    const TickType_t delay_ticks = pdMS_TO_TICKS(JOYSTICK_UPDATE_INTERVAL);
    const TickType_t wifi_blink_ticks = pdMS_TO_TICKS(500); 
    uint32_t last_wifi_blink_time = 0;
    bool blink_toggle = false;
    joystick_button_event_t btn_evt;

    
    joystick_pos_t pos = {0, 0}; 

    while (1) {
        if (joystick_button_get_queue() != NULL) {
            while (xQueueReceive(joystick_button_get_queue(), &btn_evt, 0) == pdTRUE) {
                switch (btn_evt) {
                    case BTN_EVT_LONG_PRESS:   
                        led_power_on = !led_power_on;
                        ESP_LOGI(TAG, "LED Power: %s", led_power_on ? "ON" : "OFF");
                        if (!led_power_on) ws2812_clear(); 
                        break;
                    case BTN_EVT_PRESS_DOWN:   
                        led_locked = !led_locked; 
                        ESP_LOGI(TAG, "LED Lock: %s", led_locked ? "ON" : "OFF");
                        break;
                    case BTN_EVT_DOUBLE_CLICK: 
                        led_service_toggle_mode(); 
                        break;
                    case BTN_EVT_SINGLE_CLICK: 
                        joystick_toggle_inversion(); 
                        break;
                }
            }
        }

        if (joystick_get_pos_queue() != NULL) {
            xQueuePeek(joystick_get_pos_queue(), &pos, 0);
        }

        if (!led_power_on) {
            vTaskDelay(delay_ticks);
            continue;
        }
        
        uint8_t r = 0, g = 0, b = 0;

        if (!rgb_circle_mode && wifi_mode_enable) {
            if (get_uart_status_overriden_led()) { 
                led_cmd_t target = get_uart_target_color();
                r = target.r; g = target.g; b = target.b;
            }
            else if (get_ble_status_overriden_led()) {
                led_cmd_t target = get_ble_bt_target_color();
                r = target.r; g = target.g; b = target.b;
            } 
            else if (get_status_overriden_led()) {
                led_cmd_t target = get_mqtt_target_color();
                r = target.r; g = target.g; b = target.b;
            } 
            else {
                uint32_t now = xTaskGetTickCount();
                if ((now - last_wifi_blink_time) >= wifi_blink_ticks) {
                    blink_toggle = !blink_toggle;
                    last_wifi_blink_time = now;
                }

                switch (current_wifi_state) {
                    case WIFI_LED_STA_CONNECTING: r = 255; g = 255; break; 
                    case WIFI_LED_STA_ERROR:      r = 255; break;          
                    case WIFI_LED_IP_RECEIVED:    r = 0; b = 0; if(blink_toggle) g = 255; break; 
                    case WIFI_LED_ONLINE:         g = 255; break;          
                    case WIFI_LED_AP_STARTED:     if(blink_toggle) b = 255; break; 
                    case WIFI_LED_AP_CLIENT:      b = 255; break;          
                    default:                      r = 255; g = 255; b = 255; break; 
                }
            }
            ws2812_set_rgb(r, g, b); 
            vTaskDelay(delay_ticks);
            continue;
        } 
        
        if (!led_locked) {
            if (!rgb_circle_mode) {
                if (pos.x < 0.33f)      r = (uint8_t)(255 * pos.y);
                else if (pos.x < 0.66f) g = (uint8_t)(255 * pos.y);
                else                    b = (uint8_t)(255 * pos.y);
            } else {
                float dx = pos.x - 0.5f;
                float dy = pos.y - 0.5f;
                float dist = sqrtf(dx*dx + dy*dy);
                float angle = atan2f(dy, dx);
                float brightness = fminf(dist * 2.0f, 1.0f);

                r = (uint8_t)(fmaxf(0.0f, cosf(angle)) * brightness * 255);
                g = (uint8_t)(fmaxf(0.0f, cosf(angle - 2.094f)) * brightness * 255);
                b = (uint8_t)(fmaxf(0.0f, cosf(angle + 2.094f)) * brightness * 255);
            }
            
            ws2812_set_rgb(r, g, b); 
        }
        
        vTaskDelay(delay_ticks); 
    }
}

void led_service_start(void) {
    xTaskCreate(led_service_task, "led_service", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "LED Service started");
}