#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"


typedef struct {
    int deviceID;
    int measurementID;
    float temperature;
} DataPackage_t;


QueueHandle_t get_sensor_queue_handle(void);

void data_bridge_init(void);
