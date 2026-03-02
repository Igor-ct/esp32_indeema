#include <stdio.h>
#include "mqtt_wrapper.h" 
#include "esp_netif.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include <inttypes.h>
#include <string.h>
#include "json_parser.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define my_address_uri    CONFIG_MQTT_BROKER_URI
#define MQTT_CLIENT_ID    CONFIG_MQTT_CLIENT_ID
#define MQTT_CMD_TOPIC    CONFIG_MQTT_CMD_TOPIC
#define MQTT_STATUS_TOPIC CONFIG_MQTT_STATUS_TOPIC

static const char *TAG = "mqtt";

static char message[64];
static bool is_mqtt_connected = false; 
static bool status_overriden_led = false;
static int mqtt_target_r = 0, mqtt_target_g = 0, mqtt_target_b = 0;

esp_mqtt_client_handle_t global_client = NULL; 
QueueHandle_t mqtt_cmd_queue = NULL;

static void set_status_overriden_led(bool status);

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        is_mqtt_connected = true; 
        
        msg_id = esp_mqtt_client_subscribe(client, MQTT_CMD_TOPIC, 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_publish(client, MQTT_STATUS_TOPIC, "online", 0, 1, 1 ); 
        ESP_LOGI(TAG, "sent publish  successful, msg_id=%d", msg_id);
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        set_status_overriden_led(false); 
        is_mqtt_connected = false; 
        break;
        
    case MQTT_EVENT_DATA: {
        if (strncmp(event->topic, MQTT_CMD_TOPIC, event->topic_len) != 0) {
            break; 
        }

        char *json_string = strndup(event->data, event->data_len);
        if (json_string == NULL) {
            ESP_LOGE(TAG, "Not enough memory to copy payload");
            break;
        }

        parsed_led_cmd_t parsed_cmd;
        
        if (json_parse_led_command(json_string, &parsed_cmd) == ESP_OK) {
            if (mqtt_cmd_queue != NULL) {
                xQueueSend(mqtt_cmd_queue, &parsed_cmd, 0);
            }
        } else {
            ESP_LOGE(TAG, "Failed to parse JSON or invalid command format.");
        }

        free(json_string); 
        break;
    }
        
    default:
        break;
    }
}

void mqtt_app_start(void)
{
    if (mqtt_cmd_queue == NULL) {
        mqtt_cmd_queue = xQueueCreate(10, sizeof(parsed_led_cmd_t));
    }

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = my_address_uri,
        },
        .credentials = {
            .client_id = MQTT_CLIENT_ID, 
        },
        .session.last_will = { 
            .topic = "esp-lection/status",
            .msg = "offline",
            .qos = 1,
            .retain = 1,
        },
    };

    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    
    global_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(global_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(global_client);
}

void task_heartbeat(void *pvParameters)
{
    while (1) {
        if (is_mqtt_connected && global_client != NULL) {
            esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, "online", 0, 1, 0); 
            ESP_LOGI("HB", "Heartbeat sent");
        } else {
            ESP_LOGD("HB", "Skipping heartbeat, no connection");
        }
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

void task_cmd_manager(void *pvParameters)
{
    parsed_led_cmd_t cmd;
    
    while (1) {
        if (mqtt_cmd_queue == NULL) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue; 
        }
        if (mqtt_cmd_queue != NULL && xQueueReceive(mqtt_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            
            if (cmd.has_color) {
                mqtt_target_r = cmd.r;
                mqtt_target_g = cmd.g;
                mqtt_target_b = cmd.b;
                set_status_overriden_led(true);
                
                snprintf(message, sizeof(message), "led set(%d, %d, %d)", mqtt_target_r, mqtt_target_g, mqtt_target_b);
                esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, message, 0, 1, 1 ); 
                ESP_LOGI("CMD_TASK", "Color applied: %d, %d, %d", cmd.r, cmd.g, cmd.b);
            }

            if (cmd.state == JSON_LED_STATE_OFF) {
                mqtt_target_r = 0; mqtt_target_g = 0; mqtt_target_b = 0;
                set_status_overriden_led(true);
                esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, "led: off", 0, 1, 1); 
                ESP_LOGI("CMD_TASK", "LED State: OFF");
            } 
            else if (cmd.state == JSON_LED_STATE_ON) {
                mqtt_target_r = 255; mqtt_target_g = 255; mqtt_target_b = 255;
                set_status_overriden_led(true);
                esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, "led: on", 0, 1, 1); 
                ESP_LOGI("CMD_TASK", "LED State: ON");
            }
            else if (cmd.state == JSON_LED_STATE_AUTO) {
                set_status_overriden_led(false); 
                esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, "led: auto", 0, 1, 1); 
                ESP_LOGI("CMD_TASK", "LED State: AUTO");
            }
        }
    }
}

led_cmd_t get_mqtt_target_color(void) {
    led_cmd_t color = { mqtt_target_r, mqtt_target_g, mqtt_target_b };
    return color;
}

static void set_status_overriden_led(bool status)
{
    status_overriden_led = status;
}

bool get_status_overriden_led(void)
{
    return status_overriden_led;
}

esp_mqtt_client_handle_t get_mqtt_client_handle(void) {
    return global_client;
}

esp_err_t mqtt_publish_message(const char *topic, const char *payload)
{
    if (global_client == NULL || !is_mqtt_connected) {
        ESP_LOGW(TAG, "Cannot publish to %s: MQTT is disconnected", topic);
        return ESP_FAIL;
    }

    int msg_id = esp_mqtt_client_publish(global_client, topic, payload, 0, 0, 0);
    
    if (msg_id >= 0) {
        ESP_LOGD(TAG, "Successfully queued publish, msg_id=%d", msg_id);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to publish message");
        return ESP_FAIL;
    }
}

bool get_mqtt_connected(void) {
    return is_mqtt_connected;
}