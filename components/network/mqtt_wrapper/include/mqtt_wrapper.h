#pragma once
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt_client.h"
#include "led_service.h"

esp_mqtt_client_handle_t get_mqtt_client_handle(void);

void mqtt_app_start(void);

void task_heartbeat(void *pvParameters);

void task_cmd_manager(void *pvParameters);

esp_err_t mqtt_publish_message(const char *topic, const char *payload);

bool get_mqtt_connected(void);