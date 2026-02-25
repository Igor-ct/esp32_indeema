#include "Joy_stick_control_led.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led2.h"
#include <math.h>
#include "wifi_sta.h"
#include "wifi_ap.h"
#include "esp_wifi.h"       
#include "my_mqtt.h"
#include "BLE_BT.h"

static const char *TAG = "JOYSTICK";

static adc_oneshot_unit_handle_t adc_handle;
static bool rgb_circle_mode = CONFIG_JOYSTICK_ENABLE_CIRCLE_MODE;
static bool wifi_mode_enable = CONFIG_JOYSTICK_ENABLE_WIFI_MODE;
static bool joystick_inverted = false;

typedef enum {
    WIFI_STATE_OFF,
    WIFI_STATE_STA,
    WIFI_STATE_AP
} my_wifi_mode_t;

static my_wifi_mode_t current_wifi_mode = WIFI_STATE_OFF;

static void switch_wifi_mode(my_wifi_mode_t new_mode) {
    if (current_wifi_mode == new_mode) return; 

    ESP_LOGI(TAG, "Switching WiFi mode...");

    if (current_wifi_mode != WIFI_STATE_OFF) {
        esp_wifi_stop();
        esp_wifi_deinit();
    }

    if (new_mode == WIFI_STATE_STA) {
        wifi_init_sta(); 
        current_wifi_mode = WIFI_STATE_STA;
    } else if (new_mode == WIFI_STATE_AP) {
        wifi_init_softap();
        current_wifi_mode = WIFI_STATE_AP;
    } else {
        current_wifi_mode = WIFI_STATE_OFF;
        led2_set_wifi_state(WIFI_LED_OFF);
        ESP_LOGI(TAG, "WiFi Disabled");
    }
}

esp_err_t joystick_led_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = CONFIG_JOYSTICK_ADC_UNIT == 1 ? ADC_UNIT_1 : ADC_UNIT_2,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel( adc_handle, CONFIG_JOYSTICK_X_CHANNEL, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel( adc_handle, CONFIG_JOYSTICK_Y_CHANNEL, &chan_cfg));

    ESP_LOGI(TAG, "Joystick initialized (X:%d Y:%d)", CONFIG_JOYSTICK_X_CHANNEL, CONFIG_JOYSTICK_Y_CHANNEL);

    return ESP_OK;
}


esp_err_t joystick_read(joystick_pos_t *pos)
{
    int raw_x = 0;
    int raw_y = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, CONFIG_JOYSTICK_X_CHANNEL, &raw_x));
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle,  CONFIG_JOYSTICK_Y_CHANNEL, &raw_y));

    float norm_x = raw_x / 4095.0f;
    float norm_y = raw_y / 4095.0f;

    if (joystick_inverted) {
        norm_x = 1.0f - norm_x;
        norm_y = 1.0f - norm_y;
    }

    pos->x = norm_x;
    pos->y = norm_y;

    return ESP_OK;
}

void joystick_update_task(void *pvParameters)
{
    const TickType_t delay_ticks = pdMS_TO_TICKS(CONFIG_JOYSTICK_UPDATE_INTERVAL);
    const TickType_t wifi_blink_ticks = pdMS_TO_TICKS(500); 
    uint32_t last_wifi_blink_time = 0;
    bool blink_toggle = false;

    joystick_pos_t pos;

    while (1) {
        joystick_read(&pos);

        float x = pos.x;
        float y = pos.y;
        uint8_t r = 0, g = 0, b = 0;

        if (!rgb_circle_mode && wifi_mode_enable) {
            if (x < 0.10f)      switch_wifi_mode(WIFI_STATE_STA);
            else if (x > 0.90f) switch_wifi_mode(WIFI_STATE_AP);
            else if (y < 0.10f) switch_wifi_mode(WIFI_STATE_OFF);

            
            if (get_ble_status_overriden_led()) {
                led_cmd_t target = get_ble_bt_target_color();
                r = target.r; 
                g = target.g; 
                b = target.b;
            } 
            else if (get_status_overriden_led()) {
                led_cmd_t target = get_mqtt_target_color();
                r = target.r; 
                g = target.g; 
                b = target.b;
            } 
            else {
                uint32_t now = xTaskGetTickCount();
                if ((now - last_wifi_blink_time) >= wifi_blink_ticks) {
                    blink_toggle = !blink_toggle;
                    last_wifi_blink_time = now;
                }

                wifi_led_state_t state = led2_get_wifi_state(); 

                switch (state) {
                    case WIFI_LED_STA_CONNECTING: r = 255; g = 255; break; 
                    case WIFI_LED_STA_ERROR:      r = 255; break;          
                    case WIFI_LED_IP_RECEIVED:    r = 0; b = 0; if(blink_toggle) g = 255; break; 
                    case WIFI_LED_ONLINE:         g = 255; break;          
                    case WIFI_LED_AP_STARTED:     if(blink_toggle) b = 255; break; 
                    case WIFI_LED_AP_CLIENT:      b = 255; break;          
                    default:                      r = 255; g = 255; b = 255; break; 
                }
            }
            
            led2_set_wifi_style(r, g, b); 
            vTaskDelay(delay_ticks);
            continue;
        } 
        
        if (!rgb_circle_mode) {
            if (!led2_is_lock()) {
                if (x < 0.33f)      r = (uint8_t)(255 * y);
                else if (x < 0.66f) g = (uint8_t)(255 * y);
                else                b = (uint8_t)(255 * y);
            }
        } 
        else {
            if (!led2_is_lock()) {
                float dx = x - 0.5f;
                float dy = y - 0.5f;
                float dist = sqrtf(dx*dx + dy*dy);
                float angle = atan2f(dy, dx);
                float brightness = fminf(dist * 2.0f, 1.0f);

                r = (uint8_t)(fmaxf(0.0f, cosf(angle)) * brightness * 255);
                g = (uint8_t)(fmaxf(0.0f, cosf(angle - 2.094f)) * brightness * 255);
                b = (uint8_t)(fmaxf(0.0f, cosf(angle + 2.094f)) * brightness * 255);
            }
        }

        led2_set_rgb(r, g, b); 
        vTaskDelay(delay_ticks);
    }
}

void joystick_mode_switch(void)
{
    if(CONFIG_JOYSTICK_ENABLE_CIRCLE_MODE)
    {
    rgb_circle_mode = !rgb_circle_mode;
    ESP_LOGI(TAG, "RGB mode: %s", rgb_circle_mode ? "Circle" : "Zones");
    }
}

void joystick_toggle_inversion(void)
{
    joystick_inverted = !joystick_inverted;
    ESP_LOGI(TAG, "Inversion: %s", joystick_inverted ? "ON" : "OFF");
}


