#include "wifi_sta.h"
#include <string.h>
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "sntp_sync.h"
#include "esp_netif_sntp.h"
#include "nvs.h"
#include "http_server.h"
#include "led_service.h" 
#include "mqtt_wrapper.h"

#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID_STA
#define ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD_STA
#define ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_STA_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define H2E_IDENTIFIER ""
#elif CONFIG_ESP_STA_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EH2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_STA_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "wifi_station";
static int s_retry_num = 0;


static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_mqtt_client_handle_t mqtt_client = get_mqtt_client_handle();
        if (mqtt_client != NULL) {
            esp_mqtt_client_stop(mqtt_client);
            ESP_LOGW(TAG, "Wi-Fi lost. MQTT client stopped to save resources.");
        }

        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            led_service_set_wifi_state(WIFI_LED_STA_ERROR); 
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;

        led_service_set_wifi_state(WIFI_LED_IP_RECEIVED); 

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
         
        ESP_LOGI(TAG, "Starting SNTP synchronization...");
        esp_netif_sntp_start();
        
        start_webserver();
    }
}

esp_err_t wifi_init_sta(void)
{
    s_retry_num = 0;

    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
    }
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    static bool handlers_registered = false;
    if (!handlers_registered) {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));
        handlers_registered = true;
    }

    wifi_mode_t mode;
    esp_err_t err = esp_wifi_get_mode(&mode);
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "WiFi Driver not initialized. Initializing now...");
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = H2E_IDENTIFIER,
        },
    };
    
    led_service_set_wifi_state(WIFI_LED_STA_CONNECTING);
    
    nvs_handle_t my_handle;
    esp_err_t err_nvs = nvs_open("storage", NVS_READONLY, &my_handle);
    
    if (err_nvs == ESP_OK) {
        size_t len = sizeof(wifi_config.sta.ssid);
        nvs_get_str(my_handle, "ssid", (char *)wifi_config.sta.ssid, &len);
        
        len = sizeof(wifi_config.sta.password);
        nvs_get_str(my_handle, "pass", (char *)wifi_config.sta.password, &len);
        
        nvs_close(my_handle);
        ESP_LOGI(TAG, "Loaded SSID from memory: %s", wifi_config.sta.ssid);
    } else {
        strcpy((char *)wifi_config.sta.ssid, ESP_WIFI_SSID);
        strcpy((char *)wifi_config.sta.password, ESP_WIFI_PASS);
        ESP_LOGI(TAG, "No saved Wi-Fi found. Using default SSID: %s", wifi_config.sta.ssid);
    }
    esp_wifi_stop();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        

    esp_err_t ret = esp_wifi_start();
    if (ret != ESP_OK) return ret;    
    ESP_LOGI(TAG, "wifi_init_sta finished. Waiting for connection...");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s", wifi_config.sta.ssid);
        led_service_set_wifi_state(WIFI_LED_ONLINE); 
        mqtt_app_start();
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s", wifi_config.sta.ssid);
        led_service_set_wifi_state(WIFI_LED_STA_ERROR); 
        return ESP_FAIL;
    }
}