
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
#include "led2.h"
#include "Button_for_led_ctrl.h"
#include "Joy_stick_control_led.h"
#include "wifi_sta.h"
#include "wifi_ap.h"
#include "SNTP_sync.h"
#include "my_mqtt.h"
#include "BLE_BT.h"
#include "my_UART.h"
#include "my_I2C.h"
#include "my_SPI.h"
#include "my_AHT20.h"
#include "my_BMP280.h"
#include "my_LSM6DS3.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "esp_random.h" 
#include "freertos/queue.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"


static const char *TAG = "APP";

typedef struct {
    int deviceID;
    int measurementID;
    float temperature;
} DataPackage_t;

QueueHandle_t xSensorQueue;

void task_sender(void *pvParameters)
{
    DataPackage_t DataToSend;
    DataToSend.deviceID = 100;
    DataToSend.measurementID = 0;

    while(1)
    {
        DataToSend.temperature = 20 + (float)(esp_random() % 100)/ 10;
        DataToSend.measurementID++;
        printf("[Sender] Sending measuremen #%d (Temp: %.2f)...\n",
        DataToSend.measurementID, DataToSend.temperature);
        
        if (xQueueSend(xSensorQueue, &DataToSend, portMAX_DELAY) != pdPASS)
        {
            printf("[Sender] Failde to send!\n");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
void task_receiver(void *pvParameters)
{
    DataPackage_t receivedData;
    while(1)
    {
      if  (xQueueReceive(xSensorQueue, &receivedData, portMAX_DELAY) == pdTRUE)
      {
        printf("[Receiver] GOT DATA! Device: %d | ID: %d | Temd: %.2f\n",
        receivedData.deviceID, receivedData.measurementID, receivedData.temperature);
      }
    }
}

typedef struct {
    const char *name;
    uint32_t period_ms;
    uint32_t busy_ms;
} load_cfg_t;


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

static void task_cpu_load(void *pvParameters)
{
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
               cfg->name, xPortGetCoreID(),
               (unsigned)cfg->busy_ms, (unsigned)cfg->period_ms);

        vTaskDelayUntil(&last_wake, period_ticks);
    }
}


static void task_system_status(void *pvParameters)
{
    (void)pvParameters;

    static char list_buf[2048];
    static char runtime_buf[2048];

    while (1) {
        printf("\n================ SYSTEM STATUS ================\n");
        printf("Uptime: %lld ms\n", (long long)(esp_timer_get_time() / 1000));
        printf("Free heap: %u bytes\n", (unsigned)esp_get_free_heap_size());

        memset(list_buf, 0, sizeof(list_buf));
        vTaskList(list_buf);
        printf("\nTask          State  Prio  StackHW  Num\n");
        printf("-------------------------------------------\n");
        printf("%s\n", list_buf);

        memset(runtime_buf, 0, sizeof(runtime_buf));
        vTaskGetRunTimeStats(runtime_buf);
        printf("CPU usage per task:\n");
        printf("Task               AbsTime   %%Time\n");
        printf("-----------------------------------\n");
        printf("%s\n", runtime_buf);

        UBaseType_t n = uxTaskGetNumberOfTasks();
        TaskStatus_t *st = (TaskStatus_t *)malloc(n * sizeof(TaskStatus_t));
        if (st) {
            uint32_t total = 0;
            n = uxTaskGetSystemState(st, n, &total);

            printf("Status task running on core=%d\n", xPortGetCoreID());
            printf("Core info:\n");
            for (UBaseType_t i = 0; i < n; i++) {
               printf(" - %-16s stackHW=%u\n",
                st[i].pcTaskName,
                (unsigned)st[i].usStackHighWaterMark);
            }
            free(st);
        }

        printf("================================================\n\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void ble_host_task(void *pvParameters)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run(); 
    nimble_port_freertos_deinit();
}

void telemetry_update_task(void *pvParameters)
{
    const TickType_t delay_ticks = pdMS_TO_TICKS(5000); 

    while (1) {
        float temp_aht = 0.0f, hum_aht = 0.0f;
        int32_t raw_temp = 0, raw_press = 0;
        int16_t accel_x = 0, accel_y = 0, accel_z = 0;

        aht20_read(&temp_aht, &hum_aht);
        bmp280_read_raw(&raw_temp, &raw_press);
        lsm6ds3_read_accel(&accel_x, &accel_y, &accel_z);

        char telemetry_json[256];
        snprintf(telemetry_json, sizeof(telemetry_json),
                 "{\"temp_c\":%.2f, \"hum_percent\":%.2f, \"press_raw\":%ld, \"accel\":[%d,%d,%d]}",
                 temp_aht, hum_aht, raw_press, accel_x, accel_y, accel_z);

        ESP_LOGI("TELEMETRY", "Generated: %s", telemetry_json);

        sendData("TELEMETRY", telemetry_json);
        sendData("TELEMETRY", "\r\n");

        mqtt_publish_message("esp-lection/telemetry", telemetry_json);

        ble_update_telemetry(telemetry_json);

        vTaskDelay(delay_ticks);
    }
}

void app_main(void)
{


    printf("Hello world!\n");
    print_chip_info();    
    /*xSensorQueue = xQueueCreate(5, sizeof(DataPackage_t));
    if (xSensorQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }

    xTaskCreate(task_sender,   "Sender_Task",   2048, NULL, 4, NULL);
    xTaskCreate(task_receiver, "Receiver_Task", 4096, NULL, 4, NULL);

    static load_cfg_t loadA = { .name="LOAD_A", .period_ms=1000, .busy_ms=300 };
    static load_cfg_t loadB = { .name="LOAD_B", .period_ms=500,  .busy_ms=150 };

    xTaskCreatePinnedToCore(task_cpu_load, "Load_A", 3072, &loadA, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_cpu_load, "Load_B", 3072, &loadB, 5, NULL, 1);

    xTaskCreate(task_system_status, "Sys_Status", 4096, NULL, 3, NULL);
    */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    SNTP_service_init();
    
    uart_component_init(); 
    i2c_bus_init();
    spi_bus_init();

    if (aht20_init() == ESP_OK) ESP_LOGI(TAG, "AHT20 Initialized");
    if (bmp280_init() == ESP_OK) ESP_LOGI(TAG, "BMP280 Initialized");
    if (lsm6ds3_init(10) == ESP_OK) ESP_LOGI(TAG, "LSM6DS3 Initialized");
    
    led2_init();
    joystick_led_init();
    button_for_led_ctrl_init();


    mqtt_cmd_queue = xQueueCreate(10, sizeof(led_cmd_t));
    xTaskCreate(task_heartbeat, "mqtt_heartbeat", 3072, NULL, 5, NULL);
    xTaskCreate(task_cmd_manager, "mqtt_cmd_manager", 3072, NULL, 5, NULL);
    
    xTaskCreate(joystick_update_task, "joystick update", 4096, NULL, 5, NULL);

    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, 5, NULL); 
    xTaskCreate(telemetry_update_task, "telemetry_task", 4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "Initializing NimBLE...");
    nimble_port_init();
    ESP_ERROR_CHECK(gatt_svr_init());
    ble_setup_stack_and_security();
    nimble_port_freertos_init(ble_host_task);
    ESP_LOGI(TAG, "NimBLE successfully started!");
}
