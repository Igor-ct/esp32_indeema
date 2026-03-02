#include "sys_monitor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "inttypes.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_timer.h"

void sys_monitor_print_chip_info(void) {
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;
    esp_chip_info(&chip_info);

    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET, chip_info.cores,
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

void task_cpu_load(void *pvParameters) {
    const load_cfg_t *cfg = (const load_cfg_t *)pvParameters;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(cfg->period_ms);
    volatile uint32_t x = 0;

    while (1) {
        int64_t start_us = esp_timer_get_time();
        while ((esp_timer_get_time() - start_us) < (int64_t)cfg->busy_ms * 1000LL) {
            x += 3;
            x ^= (x << 1);
        }
        printf("[LOAD] %s core=%d busy=%ums period=%ums\n",
               cfg->name, xPortGetCoreID(), (unsigned)cfg->busy_ms, (unsigned)cfg->period_ms);
        vTaskDelayUntil(&last_wake, period_ticks);
    }
}

void task_system_status(void *pvParameters) {
    static char list_buf[2048];
    static char runtime_buf[2048];

    while (1) {
        printf("\n================ SYSTEM STATUS ================\n");
        printf("Uptime: %lld ms\n", (long long)(esp_timer_get_time() / 1000));
        printf("Free heap: %u bytes\n", (unsigned)esp_get_free_heap_size());

        memset(list_buf, 0, sizeof(list_buf));
        vTaskList(list_buf);
        printf("\nTask          State  Prio  StackHW  Num\n");
        printf("-------------------------------------------\n%s\n", list_buf);

        memset(runtime_buf, 0, sizeof(runtime_buf));
        vTaskGetRunTimeStats(runtime_buf);
        printf("CPU usage per task:\nTask                AbsTime   %%Time\n");
        printf("-----------------------------------\n%s\n", runtime_buf);

        UBaseType_t n = uxTaskGetNumberOfTasks();
        TaskStatus_t *st = (TaskStatus_t *)malloc(n * sizeof(TaskStatus_t));
        if (st) {
            uint32_t total = 0;
            n = uxTaskGetSystemState(st, n, &total);
            printf("Status task running on core=%d\nCore info:\n", xPortGetCoreID());
            for (UBaseType_t i = 0; i < n; i++) {
                printf(" - %-16s stackHW=%u\n", st[i].pcTaskName, (unsigned)st[i].usStackHighWaterMark);
            }
            free(st);
        }
        printf("================================================\n\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}