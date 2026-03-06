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

            parsed_led_cmd_t parsed_cmd;
            
            if (json_parse_led_command((const char*)data, &parsed_cmd) == ESP_OK) {
                
                if (parsed_cmd.has_color) {
                    uart_target_r = parsed_cmd.r;
                    uart_target_g = parsed_cmd.g;
                    uart_target_b = parsed_cmd.b;
                    
                    ESP_LOGI(RX_TASK_TAG, "UART JSON Command: Color set to %d, %d, %d", 
                             uart_target_r, uart_target_g, uart_target_b);
                }

                if (parsed_cmd.state == JSON_LED_STATE_OFF) {
                    led_send_remote_command(LED_REMOTE_OFF, 0, 0, 0, 5);
                    ESP_LOGI(RX_TASK_TAG, "UART JSON Command: LED OFF");
                } 
                else if (parsed_cmd.state == JSON_LED_STATE_ON) {
                    led_send_remote_command(LED_REMOTE_ON, uart_target_r, uart_target_g, uart_target_b, 5);
                    ESP_LOGI(RX_TASK_TAG, "UART JSON Command: LED ON");
                }
                else if (parsed_cmd.state == JSON_LED_STATE_AUTO) {
                    led_send_remote_command(LED_REMOTE_OFF, 0, 0, 0, 5);
                    ESP_LOGI(RX_TASK_TAG, "UART JSON Command: LED AUTO (Override disabled)");
                }
                
            } else {
                ESP_LOGW(RX_TASK_TAG, "Failed to parse JSON string or invalid format");
            }
        }
    }
    free(data);
}


