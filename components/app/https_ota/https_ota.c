#include "https_ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_https_ota.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "OTA_SVC";

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[]   asm("_binary_ca_cert_pem_end");

static bool ota_is_running = false;

static bool is_new_version_greater(const char *new_ver, const char *cur_ver) {
    int n_maj = 0, n_min = 0, n_pat = 0;
    int c_maj = 0, c_min = 0, c_pat = 0;

    if (sscanf(new_ver, "v%d.%d.%d", &n_maj, &n_min, &n_pat) != 3) {
        sscanf(new_ver, "%d.%d.%d", &n_maj, &n_min, &n_pat);
    }

    if (sscanf(cur_ver, "v%d.%d.%d", &c_maj, &c_min, &c_pat) != 3) {
        sscanf(cur_ver, "%d.%d.%d", &c_maj, &c_min, &c_pat);
    }

    if (n_maj > c_maj) return true;
    if (n_maj == c_maj && n_min > c_min) return true;
    if (n_maj == c_maj && n_min == c_min && n_pat > c_pat) return true;
    
    return false; 
}

static void ota_task(void *pvParameter) {
    char *url = (char *)pvParameter;
    ESP_LOGI(TAG, "Starting OTA from: %s", url);

    esp_http_client_config_t http_config = {
        .url = url,
        .cert_pem = (char *)server_cert_pem_start,
        .keep_alive_enable = true,
#ifdef CONFIG_SKIP_COMMON_NAME_CHECK
        .skip_cert_common_name_check = true,
#endif
#ifdef CONFIG_OTA_RECV_TIMEOUT
        .timeout_ms = CONFIG_OTA_RECV_TIMEOUT,
#endif
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        goto ota_end;
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_get_img_desc failed");
        goto ota_abort;
    }

    const esp_app_desc_t *running_app_desc = esp_app_get_description();

    ESP_LOGI(TAG, "Running firmware version: %s", running_app_desc->version);
    ESP_LOGI(TAG, "Available firmware version: %s", app_desc.version);

    if (!is_new_version_greater(app_desc.version, running_app_desc->version)) {
        ESP_LOGW(TAG, "New version is not strictly greater than current. Aborting OTA.");
        goto ota_abort;
    }

    ESP_LOGI(TAG, "Version is valid. Proceeding with download...");

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        ESP_LOGE(TAG, "Complete data was not received.");
        goto ota_abort;
    }

    esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
        ESP_LOGI(TAG, "OTA Upgrade successful. Rebooting in 2 seconds...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA Upgrade failed. Error: %s", esp_err_to_name(ota_finish_err));
    }

    goto ota_end;

ota_abort:
    esp_https_ota_abort(https_ota_handle);
ota_end:
    free(url);
    ota_is_running = false;
    vTaskDelete(NULL);
}

bool ota_service_start_update(const char *url) {
    if (ota_is_running) {
        ESP_LOGW(TAG, "OTA is already in progress!");
        return false;
    }
    
    const char *final_url = url;
    
    if (final_url == NULL || strlen(final_url) == 0) {
#ifdef CONFIG_FIRMWARE_UPGRADE_URL
        final_url = CONFIG_FIRMWARE_UPGRADE_URL;
#else
        ESP_LOGE(TAG, "No URL provided and CONFIG_FIRMWARE_UPGRADE_URL is not set");
        return false;
#endif
    }

    char *url_copy = strdup(final_url);
    if (url_copy == NULL) return false;

    ota_is_running = true;
    
    xTaskCreate(&ota_task, "ota_task", 8192, (void *)url_copy, 5, NULL);
    return true;
}