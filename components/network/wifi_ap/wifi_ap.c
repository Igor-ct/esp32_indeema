#include "wifi_ap.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "http_server.h"
#include "led_service.h" 


#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID_AP
#define ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD_AP
#define ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#if CONFIG_ESP_GTK_REKEYING_ENABLE
#define GTK_REKEY_INTERVAL CONFIG_ESP_GTK_REKEY_INTERVAL
#else
#define GTK_REKEY_INTERVAL 0
#endif

static const char *TAG = "wifi_AP";

static esp_netif_t *ap_netif = NULL;
extern bool is_wifi_driver_init;
static bool is_ap_webserver_started = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
        
        led_service_set_wifi_state(WIFI_LED_AP_CLIENT); 
        
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
                 
        led_service_set_wifi_state(WIFI_LED_AP_STARTED);
    }
}

esp_err_t wifi_init_softap(void)
{
    esp_err_t ret = ESP_OK;

    static bool ap_handlers_registered = false;
    if (!ap_handlers_registered) {
        ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
        if (ret != ESP_OK) return ret;
        ap_handlers_registered = true;
    }

    wifi_mode_t mode;
    if (esp_wifi_get_mode(&mode) == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "WiFi Driver not initialized. Initializing now...");
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    }
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = { .required = true },
            .gtk_rekey_interval = GTK_REKEY_INTERVAL,
        },
    };

    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_wifi_stop();

    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) return ret;

    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) return ret;

    led_service_set_wifi_state(WIFI_LED_AP_STARTED);

    ret = esp_wifi_start();
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s channel:%d", ESP_WIFI_SSID, ESP_WIFI_CHANNEL);
    if (!is_ap_webserver_started) {
        start_webserver(); 
        is_ap_webserver_started = true;
    }
    return ESP_OK;
}