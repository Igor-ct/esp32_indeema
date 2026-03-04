#include "wifi_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "joystick_controller.h" 
#include "led_service.h"         
#include "wifi_sta.h"            
#include "wifi_ap.h"             
#include "sntp_sync.h"

static const char *TAG = "WIFI_SERVICE";

typedef enum { WIFI_STATE_OFF, WIFI_STATE_STA, WIFI_STATE_AP } my_wifi_mode_t;
static my_wifi_mode_t current_wifi_mode = WIFI_STATE_OFF;

static bool wifi_mode_enable = JOYSTICK_ENABLE_WIFI_MODE;

static void switch_wifi_mode(my_wifi_mode_t new_mode) {
    if (current_wifi_mode == new_mode) return; 
    ESP_LOGI(TAG, "Switching WiFi mode...");

    if (current_wifi_mode != WIFI_STATE_OFF) {
        esp_wifi_stop();
        esp_wifi_deinit();
    }

    if (new_mode == WIFI_STATE_STA)      wifi_init_sta(); 
    else if (new_mode == WIFI_STATE_AP)  wifi_init_softap();
    else {
        led_service_set_wifi_state(WIFI_LED_OFF);
        ESP_LOGI(TAG, "WiFi Disabled");
    }
    current_wifi_mode = new_mode;
}

static void wifi_service_task(void *pvParameters) {
    joystick_zone_t zone;

    while (1) {
        if (joystick_get_zone_queue() != NULL) {
            
            if (xQueueReceive(joystick_get_zone_queue(), &zone, portMAX_DELAY) == pdTRUE) {
                
                if (!led_service_is_circle_mode() && wifi_mode_enable) {
                    switch (zone) {
                        case JOYSTICK_ZONE_LEFT:  switch_wifi_mode(WIFI_STATE_STA); break;
                        case JOYSTICK_ZONE_RIGHT: switch_wifi_mode(WIFI_STATE_AP);  break;
                        case JOYSTICK_ZONE_DOWN:  switch_wifi_mode(WIFI_STATE_OFF); break;
                        default: break; 
                    }
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void wifi_service_start(void) {

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    sntp_service_init();

    xTaskCreate(wifi_service_task, "wifi_service", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "WiFi Service started");
}