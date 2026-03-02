#include <stdio.h>
#include "telemetry_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"     

#include "mqtt_wrapper.h"
#include "ble.h"
#include "uart.h"
#include "aht20.h"
#include "bmp280.h"
#include "lsmd6ds3.h"

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

        if (get_mqtt_connected()) { 
                mqtt_publish_message("esp-lection/telemetry", telemetry_json);
        } 

        vTaskDelay(delay_ticks);
    }
}
