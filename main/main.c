#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_random.h" 
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "sys_monitor.h"
#include "data_bridge.h"

#include "ws2812.h"
#include "joystick_button.h"
#include "joystick_controller.h"

#include "led_service.h"
#include "wifi_service.h"
#include "telemetry_service.h"

#include "wifi_sta.h"
#include "wifi_ap.h"
#include "sntp_sync.h"
#include "mqtt_wrapper.h"
#include "ble.h"

#include "uart.h"
#include "i2c.h"
#include "spi.h"

#include "aht20.h"
#include "bmp280.h"
#include "lsmd6ds3.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

static const char *TAG = "APP";


void ble_host_task(void *pvParameters)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run(); 
    nimble_port_freertos_deinit();
}


void app_main(void)
{
    printf("Hello world!\n");
    
    sys_monitor_print_chip_info();

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
    
    ws2812_init();
    joystick_init();
    joystick_button_init();

    led_service_start();
    wifi_service_start();

    //data_bridge_init();

    xTaskCreate(task_heartbeat, "mqtt_heartbeat", 3072, NULL, 5, NULL);
    xTaskCreate(task_cmd_manager, "mqtt_cmd_manager", 3072, NULL, 5, NULL);
    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, 5, NULL); 
    xTaskCreate(telemetry_update_task, "telemetry_task", 4096, NULL, 4, NULL);

    //xTaskCreate(task_system_status, "Sys_Status", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Initializing NimBLE...");
    nimble_port_init();
    ESP_ERROR_CHECK(gatt_svr_init());
    ble_setup_stack_and_security();
    nimble_port_freertos_init(ble_host_task);
    ESP_LOGI(TAG, "NimBLE successfully started!");
    
}