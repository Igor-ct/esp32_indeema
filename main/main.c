/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "led_ctrl.h"

static const char *TAG = "APP";

static void print_chip_info(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;

    esp_chip_info(&chip_info);

    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);

    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed\n");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
}

void app_main(void)
{
    printf("Hello world!\n");
    print_chip_info();

#if CONFIG_MY_LED_ENABLE
    const int period_ms =
    #ifdef CONFIG_MY_BLINK_PERIOD_MS
        CONFIG_MY_BLINK_PERIOD_MS;
    #else
        500;
    #endif

    ESP_LOGI(TAG, "LED enabled: gpio=%d active_high=%d period=%dms",
             CONFIG_MY_LED_GPIO,
             CONFIG_MY_LED_ACTIVE_HIGH ? 1 : 0,
             period_ms);

    led_ctrl_t led;
    int rc = led_ctrl_init(&led, CONFIG_MY_LED_GPIO, CONFIG_MY_LED_ACTIVE_HIGH);
    if (rc != 0) {
        ESP_LOGE(TAG, "led_ctrl_init failed rc=%d (gpio=%d)", rc, CONFIG_MY_LED_GPIO);
        return;
    }

    while (1) {
        led_ctrl_toggle(&led);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
#else
    ESP_LOGW(TAG, "LED feature disabled in menuconfig (MY_LED_ENABLE=n). Idle loop.");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#endif
}
