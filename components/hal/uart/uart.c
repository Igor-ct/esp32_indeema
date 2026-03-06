#include <stdio.h>
#include "uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "json_parser.h"
#include "led_service.h"
#include "motor_service.h"

static bool uart_led_override = false;
static uint8_t uart_target_r = 0;
static uint8_t uart_target_g = 0;
static uint8_t uart_target_b = 0;


static const int RX_BUF_SIZE = 1024;

#define TXD_PIN CONFIG_MY_TXD_PIN
#define RXD_PIN CONFIG_MY_RXD_PIN
#define UART_BAUD_RATE CONFIG_UART_BAUD_RATE

void uart_component_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, 5, NULL); 
}

int send_data(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        send_data(TX_TASK_TAG, "Hello world");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1);
    
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        
        if (rxBytes > 0) {
            data[rxBytes] = 0; 
            ESP_LOGI(RX_TASK_TAG, "Received JSON: '%s'", data);

            parsed_led_cmd_t led_cmd;
            bool is_led_cmd = (json_parse_led_command((const char*)data, &led_cmd) == ESP_OK);

            if (is_led_cmd) {
                if (led_cmd.has_color) {
                    uart_target_r = led_cmd.r;
                    uart_target_g = led_cmd.g;
                    uart_target_b = led_cmd.b;
                    ESP_LOGI(RX_TASK_TAG, "UART: Color set to %d,%d,%d", uart_target_r, uart_target_g, uart_target_b);
                }

                if (led_cmd.state == JSON_LED_STATE_OFF) {
                    led_send_remote_command(LED_REMOTE_OFF, 0, 0, 0, 5);
                } else if (led_cmd.state == JSON_LED_STATE_ON) {
                    led_send_remote_command(LED_REMOTE_ON, uart_target_r, uart_target_g, uart_target_b, 5);
                } else if (led_cmd.state == JSON_LED_STATE_AUTO) {
                    led_send_remote_command(LED_REMOTE_OFF, 0, 0, 0, 5);
                }
            }

            parsed_motor_cmd_t motor_cmd;
            bool is_motor_cmd = (json_parse_motor_command((const char*)data, &motor_cmd) == ESP_OK);

            if (is_motor_cmd) {
                if (motor_cmd.has_mode) {
                    motor_service_set_mode((motor_mode_t)motor_cmd.mode);
                    ESP_LOGI(RX_TASK_TAG, "UART: Motor Mode %d", motor_cmd.mode);
                }
                if (motor_cmd.has_angle) {
                    motor_service_set_angle(motor_cmd.angle);
                    ESP_LOGI(RX_TASK_TAG, "UART: Motor Angle %.1f", motor_cmd.angle);
                }
            }

            if (!is_led_cmd && !is_motor_cmd) {
                ESP_LOGW(RX_TASK_TAG, "Failed to parse JSON string or unknown command");
            }
        }
    }
    free(data); 
}


