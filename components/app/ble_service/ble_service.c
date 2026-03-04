#include <stdio.h>
#include "ble_service.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "ble.h"
#include "esp_log.h"

static const char *TAG = "BLE_SERVICE";

static void ble_host_task(void *pvParameters)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run(); 
    nimble_port_freertos_deinit();
}

void ble_start(void)
{
    ESP_LOGI(TAG, "Initializing NimBLE...");
    nimble_port_init();
    ESP_ERROR_CHECK(ble_gatt_svr_init());
    ble_setup_stack_and_security();
    nimble_port_freertos_init(ble_host_task);
    ESP_LOGI(TAG, "NimBLE successfully started!");
}
