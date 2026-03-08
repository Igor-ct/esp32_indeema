#include <stdio.h>
#include "telemetry_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"     
#include "freertos/semphr.h"

#include "mqtt_wrapper.h"
#include "ble.h"
#include "uart.h"
#include "sensor_service.h"
#include "motor_service.h"

#define Update_env_telemetry  CONFIG_UPDATE_ENV_TELEMETRY
#define Update_motion_telemetry CONFIG_UPDATE_MOTION_TELEMETRY_TIME

static sensor_data_t g_device_state = {0};
static SemaphoreHandle_t state_mutex = NULL;
static SemaphoreHandle_t sensor_mutex = NULL; 

static const char *TAG = "TELEMETRY";

static void sync_and_send_ble(void)
{
    if (state_mutex == NULL) return;

    char full_json[200]; 
    
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        snprintf(full_json, sizeof(full_json),
                 "{\"env\":{\"t\":%.2f,\"h\":%.2f,\"p\":%ld},\"acc\":[%d,%d,%d]}",
                 g_device_state.aht20.valid ? g_device_state.aht20.temperature : 0.0,
                 g_device_state.aht20.valid ? g_device_state.aht20.humidity : 0.0,
                 g_device_state.bmp280.valid ? (long)g_device_state.bmp280.pressure : 0,
                 g_device_state.accel.valid ? g_device_state.accel.x : 0,
                 g_device_state.accel.valid ? g_device_state.accel.y : 0,
                 g_device_state.accel.valid ? g_device_state.accel.z : 0);
        
        xSemaphoreGive(state_mutex); 
    }

    ble_update_telemetry(full_json);
}

static void telemetry_env_task(void *pvParameters)
{
    const TickType_t delay_ticks = pdMS_TO_TICKS(Update_env_telemetry);
    sensor_data_t local_data;

    while (1) {
        bool read_success = false;

        if (xSemaphoreTake(sensor_mutex, portMAX_DELAY) == pdTRUE) {
            read_success = sensor_service_read(&local_data);
            xSemaphoreGive(sensor_mutex);
        }

        if(read_success) {
            
            if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
                g_device_state.aht20 = local_data.aht20;
                g_device_state.bmp280 = local_data.bmp280;
                xSemaphoreGive(state_mutex);
            }

            sync_and_send_ble();

            char json[128];
            snprintf(json, sizeof(json), "{\"temp\":%.2f,\"hum\":%.2f,\"press\":%ld}",
                     local_data.aht20.temperature, local_data.aht20.humidity, local_data.bmp280.pressure);
            
            send_data("ENV", json);
            send_data("ENV", "\r\n");
            if(get_mqtt_connected()) mqtt_publish_message("esp-lection/env", json);
        }
        vTaskDelay(delay_ticks);
    }
}

static void telemetry_motion_task(void *pvParameters)
{
    const TickType_t delay_ticks = pdMS_TO_TICKS(Update_motion_telemetry);
    sensor_data_t local_data;

    while (1) {
        bool read_success = false;

        if (xSemaphoreTake(sensor_mutex, portMAX_DELAY) == pdTRUE) {
            read_success = sensor_service_read(&local_data);
            xSemaphoreGive(sensor_mutex);
        }

        if(read_success) {
            
            if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
                g_device_state.accel = local_data.accel;
                xSemaphoreGive(state_mutex);
            }

            sync_and_send_ble();

            char json[128];
            snprintf(json, sizeof(json), "{\"accel\":[%d,%d,%d]}",
                     local_data.accel.x, local_data.accel.y, local_data.accel.z);
            
            motor_service_push_accel_x(local_data.accel.x);
            send_data("MOTION", json);
            send_data("MOTION", "\r\n");
            if(get_mqtt_connected()) mqtt_publish_message("esp-lection/motion", json);
        }
        vTaskDelay(delay_ticks);
    }
}

void telemetry_start(void)
{
    state_mutex = xSemaphoreCreateMutex();
    sensor_mutex = xSemaphoreCreateMutex(); 
    
    xTaskCreate(telemetry_env_task, "telemetry_env", 4096, NULL, 4, NULL);
    xTaskCreate(telemetry_motion_task, "telemetry_motion", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "Initialized");
}