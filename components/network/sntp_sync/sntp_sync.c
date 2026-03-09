#include <time.h>
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"
#include "SNTP_sync.h"
#include <time.h>

static const char *TAG = "SNTP_SERVICE";

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized successfully!");

    
    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);

    
    setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
    tzset();

    
    localtime_r(&now, &timeinfo);

    
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    
    ESP_LOGI(TAG, "Current local time: %s", strftime_buf);
}

void sntp_service_init(void)
{
    ESP_LOGI(TAG, "Initializing SNTP Service...");

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
    config.sync_cb = time_sync_notification_cb;

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    config.smooth_sync = true;
#else
    config.smooth_sync = false;
#endif

    esp_netif_sntp_init(&config);
}

void sntp_net_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Wi-Fi Got IP. Starting SNTP...");
        esp_netif_sntp_start(); 
    }
}