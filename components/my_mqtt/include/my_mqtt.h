#pragma once
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt_client.h"
#include "led2.h"



extern QueueHandle_t mqtt_cmd_queue;

esp_mqtt_client_handle_t get_mqtt_client_handle(void);

void mqtt_app_start(void);

bool get_status_overriden_led(void);

led_cmd_t get_mqtt_target_color(void);

void task_heartbeat(void *pvParameters);

void task_cmd_manager(void *pvParameters);