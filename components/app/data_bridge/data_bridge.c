#include "data_bridge.h"
#include <stdio.h>
#include "esp_random.h"
#include "freertos/task.h"

static QueueHandle_t xSensorQueue = NULL;

QueueHandle_t get_sensor_queue_handle(void) {
    return xSensorQueue;
}

static void task_sender(void *pvParameters) {
    DataPackage_t DataToSend = { .deviceID = 100, .measurementID = 0 };

    while(1) {
        DataToSend.temperature = 20 + (float)(esp_random() % 100) / 10.0f;
        DataToSend.measurementID++;
        
        if (xSensorQueue != NULL) {
            if (xQueueSend(xSensorQueue, &DataToSend, portMAX_DELAY) != pdPASS) {
                printf("[Sender] Failed to send!\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void task_receiver(void *pvParameters) {
    DataPackage_t receivedData;
    while(1) {
        if (xSensorQueue != NULL && xQueueReceive(xSensorQueue, &receivedData, portMAX_DELAY) == pdTRUE) {
            printf("[Receiver] GOT DATA! Device: %d | ID: %d | Temp: %.2f\n",
                   receivedData.deviceID, receivedData.measurementID, receivedData.temperature);
        }
    }
}

void data_bridge_init(void) {
    if (xSensorQueue == NULL) {
        xSensorQueue = xQueueCreate(5, sizeof(DataPackage_t));
    }
    
    if (xSensorQueue != NULL) {
        xTaskCreate(task_sender, "Sender_Task", 2048, NULL, 4, NULL);
        xTaskCreate(task_receiver, "Receiver_Task", 2048, NULL, 4, NULL);
    }
}