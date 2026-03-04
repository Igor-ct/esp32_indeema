#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "sys_monitor.h"
#include "data_bridge.h"

#include "platform_init.h"
#include "led_service.h"
#include "wifi_service.h"
#include "telemetry_service.h"
#include "ble_service.h"

static const char *TAG = "APP";

void app_main(void)
{
    printf("Hello world!\n");
    
    sys_monitor_print_chip_info();

    platform_init();

    led_service_start();
    wifi_service_start();
    telemetry_init();
    ble_start();

    //data_bridge_init();
    //xTaskCreate(task_system_status, "Sys_Status", 4096, NULL, 3, NULL);
}