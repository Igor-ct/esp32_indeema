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
#include "motor_service.h"

#define my_address_uri    CONFIG_MQTT_BROKER_URI
#define MQTT_CLIENT_ID    CONFIG_MQTT_CLIENT_ID
#define MQTT_CMD_TOPIC    CONFIG_MQTT_CMD_TOPIC
#define MQTT_STATUS_TOPIC CONFIG_MQTT_STATUS_TOPIC

static int priority = 3;

static const char *TAG = "mqtt";

static char message[64];
static bool is_mqtt_connected = false; 
static int mqtt_target_r = 0, mqtt_target_g = 0, mqtt_target_b = 0;
static float mqtt_target_angle = 0.0f;

typedef enum { CMD_TYPE_LED, CMD_TYPE_MOTOR } cmd_type_t;
typedef struct {
    cmd_type_t type;
    parsed_led_cmd_t led;
    parsed_motor_cmd_t motor;
} mqtt_full_cmd_t;

esp_mqtt_client_handle_t global_client = NULL; 
QueueHandle_t mqtt_cmd_queue = NULL;


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

        mqtt_full_cmd_t cmd_led = {0}; 
        if (json_parse_led_command(json_string, &cmd_led.led) == ESP_OK) {
            cmd_led.type = CMD_TYPE_LED;
            if (mqtt_cmd_queue != NULL) {
                xQueueSend(mqtt_cmd_queue, &cmd_led, 0);
            }
        } 

        mqtt_full_cmd_t cmd_motor = {0}; 
        if (json_parse_motor_command(json_string, &cmd_motor.motor) == ESP_OK) {
            cmd_motor.type = CMD_TYPE_MOTOR;
            if (mqtt_cmd_queue != NULL) {
                xQueueSend(mqtt_cmd_queue, &cmd_motor, 0);
            }
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
        mqtt_cmd_queue = xQueueCreate(10, sizeof(mqtt_full_cmd_t));
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
    xTaskCreate(task_heartbeat, "mqtt_heartbeat", 3072, NULL, 5, NULL);
    xTaskCreate(task_cmd_manager, "mqtt_cmd_manager", 3072, NULL, 5, NULL);
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
    mqtt_full_cmd_t cmd; 
    
    while (1) {
        if (mqtt_cmd_queue == NULL) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue; 
        }
        if (xQueueReceive(mqtt_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            
            if (cmd.type == CMD_TYPE_LED) {
                if (cmd.led.has_color) {
                    mqtt_target_r = cmd.led.r;
                    mqtt_target_g = cmd.led.g;
                    mqtt_target_b = cmd.led.b;
                    
                    snprintf(message, sizeof(message), "led set(%d, %d, %d)", mqtt_target_r, mqtt_target_g, mqtt_target_b);
                    esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, message, 0, 1, 1); 
                    ESP_LOGI("CMD_TASK", "Color applied: %d, %d, %d", cmd.led.r, cmd.led.g, cmd.led.b);
                }

                if (cmd.led.state == JSON_LED_STATE_OFF) {
                    mqtt_target_r = 0; mqtt_target_g = 0; mqtt_target_b = 0;
                    led_send_remote_command(LED_REMOTE_OFF, 0, 0, 0, priority);
                    esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, "led: off", 0, 1, 1); 
                } 
                else if (cmd.led.state == JSON_LED_STATE_ON) {
                    led_send_remote_command(LED_REMOTE_ON, mqtt_target_r, mqtt_target_g, mqtt_target_b, priority);
                    esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, "led: on", 0, 1, 1); 
                }
                else if (cmd.led.state == JSON_LED_STATE_AUTO) {
                    led_send_remote_command(LED_REMOTE_AUTO, 0, 0, 0, priority);
                    esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, "led: auto", 0, 1, 1); 
                }
            }

            if (cmd.type == CMD_TYPE_MOTOR) {
                const char* mode_names[] = {"remote", "joystick", "accel"};
                
                if (cmd.motor.has_angle) {
                    mqtt_target_angle = cmd.motor.angle;
                    
                    if (!cmd.motor.has_mode) {
                        motor_send_remote_command(MOTOR_MODE_REMOTE, mqtt_target_angle, priority);
                    }

                    ESP_LOGI("CMD_TASK", "Motor angle set: %.1f", mqtt_target_angle);
                    snprintf(message, sizeof(message), "motor: angle %.1f", mqtt_target_angle);
                    esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, message, 0, 1, 1);
                }

                if (cmd.motor.has_mode) {
                    motor_send_remote_command((motor_mode_t)cmd.motor.mode, mqtt_target_angle, priority); 
                    
                    const char* m_name = (cmd.motor.mode >= 0 && cmd.motor.mode <= 2) ? mode_names[cmd.motor.mode] : "unknown";
                    
                    ESP_LOGI("CMD_TASK", "Motor mode set: %s (%d)", m_name, cmd.motor.mode);
                    snprintf(message, sizeof(message), "motor: mode %s", m_name);
                    esp_mqtt_client_publish(global_client, MQTT_STATUS_TOPIC, message, 0, 1, 1);
                }
            }
        }
    }
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

