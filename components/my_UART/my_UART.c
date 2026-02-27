#include <stdio.h>
#include "my_UART.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "cJSON.h"


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
}

int sendData(const char* logName, const char* data)
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
        sendData(TX_TASK_TAG, "Hello world");
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

            cJSON *root = cJSON_Parse((char*)data);
        
            if (root != NULL) { 
                cJSON *led = cJSON_GetObjectItem(root, "led");
                
                if (cJSON_IsObject(led)) {
                    cJSON *color = cJSON_GetObjectItem(led, "color");
                    cJSON *state = cJSON_GetObjectItem(led, "state");

                    if (cJSON_IsObject(color)) {
                        cJSON *red = cJSON_GetObjectItem(color, "r");
                        cJSON *green = cJSON_GetObjectItem(color, "g");
                        cJSON *blue = cJSON_GetObjectItem(color, "b");

                        if (cJSON_IsNumber(red) && cJSON_IsNumber(green) && cJSON_IsNumber(blue)) {
                            uart_target_r = (uint8_t)red->valueint;
                            uart_target_g = (uint8_t)green->valueint;
                            uart_target_b = (uint8_t)blue->valueint;
                            uart_led_override = true;
                            
                            ESP_LOGI(RX_TASK_TAG, "UART JSON Command: Color set to %d, %d, %d", 
                                     uart_target_r, uart_target_g, uart_target_b);
                        }
                    }

                    if (cJSON_IsString(state)) {
                        if (strcmp(state->valuestring, "off") == 0) {
                            uart_target_r = 0; uart_target_g = 0; uart_target_b = 0;
                            uart_led_override = true;
                            ESP_LOGI(RX_TASK_TAG, "UART JSON Command: LED OFF");
                        } 
                        else if (strcmp(state->valuestring, "on") == 0) {
                            uart_target_r = 255; uart_target_g = 255; uart_target_b = 255; 
                            uart_led_override = true;
                            ESP_LOGI(RX_TASK_TAG, "UART JSON Command: LED ON");
                        }
                        else if (strcmp(state->valuestring, "auto") == 0) {
                            uart_led_override = false; 
                            ESP_LOGI(RX_TASK_TAG, "UART JSON Command: LED AUTO (Override disabled)");
                        }
                    }
                }
                
                cJSON_Delete(root); 
                
            } else {
                ESP_LOGW(RX_TASK_TAG, "Failed to parse JSON string");
            }
        }
    }
    free(data);
}


bool get_uart_status_overriden_led(void) {
    return uart_led_override;
}

led_cmd_t get_uart_target_color(void) {
    led_cmd_t color = { .r = uart_target_r, .g = uart_target_g, .b = uart_target_b };
    return color;
}