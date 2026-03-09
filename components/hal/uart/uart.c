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
#include "https_ota.h"

static int priority = 5;

static uint8_t uart_target_r = 0;
static uint8_t uart_target_g = 0;
static uint8_t uart_target_b = 0;

static float uart_target_angle = 0.0f;

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
    // ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    
    uint8_t* rx_data = (uint8_t*) malloc(RX_BUF_SIZE);
    
    char line_buf[256]; 
    int line_idx = 0;
    
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, rx_data, RX_BUF_SIZE, 50 / portTICK_PERIOD_MS);
        
        if (rxBytes > 0) {
            for (int i = 0; i < rxBytes; i++) {
                char c = (char)rx_data[i];

                if (c == '\n') {
                    line_buf[line_idx] = '\0'; 
                    
                    if (line_idx > 0 && line_buf[line_idx - 1] == '\r') {
                        line_buf[line_idx - 1] = '\0';
                    }

                    if (strlen(line_buf) > 0) {
                        // ESP_LOGI(RX_TASK_TAG, "Received complete JSON: '%s'", line_buf);

                        parsed_led_cmd_t led_cmd;
                        bool parsed_led = (json_parse_led_command(line_buf, &led_cmd) == ESP_OK);
                        bool is_led_cmd = parsed_led && (led_cmd.has_color || led_cmd.state != JSON_LED_STATE_NONE);

                        if (is_led_cmd) {
                            if (led_cmd.has_color) {
                                uart_target_r = led_cmd.r;
                                uart_target_g = led_cmd.g;
                                uart_target_b = led_cmd.b;
                                ESP_LOGI(RX_TASK_TAG, "UART: Color set to %d,%d,%d", uart_target_r, uart_target_g, uart_target_b);
                            }

                            if (led_cmd.state == JSON_LED_STATE_OFF) {
                                led_send_remote_command(LED_REMOTE_OFF, 0, 0, 0, priority);
                            } else if (led_cmd.state == JSON_LED_STATE_ON) {
                                led_send_remote_command(LED_REMOTE_ON, uart_target_r, uart_target_g, uart_target_b, priority);
                            } else if (led_cmd.state == JSON_LED_STATE_AUTO) {
                                led_send_remote_command(LED_REMOTE_AUTO, 0, 0, 0, priority); 
                            }
                        }

                        parsed_motor_cmd_t motor_cmd;
                        bool parsed_motor = (json_parse_motor_command(line_buf, &motor_cmd) == ESP_OK);
                        bool is_motor_cmd = parsed_motor && (motor_cmd.has_angle || motor_cmd.has_mode);

                        if (is_motor_cmd) {
                            if (motor_cmd.has_angle) {
                                uart_target_angle = motor_cmd.angle;
                                ESP_LOGI(RX_TASK_TAG, "UART: Motor Angle %.1f", uart_target_angle);
                            }

                            if (motor_cmd.has_mode) {
                                motor_send_remote_command((motor_mode_t)motor_cmd.mode, uart_target_angle, priority);
                                ESP_LOGI(RX_TASK_TAG, "UART: Motor Mode %d", motor_cmd.mode);
                            }
                        }

                        parsed_ota_cmd_t ota_cmd;
                        bool parsed_ota = (json_parse_ota_command(line_buf, &ota_cmd) == ESP_OK);
                        bool is_ota_cmd = parsed_ota && ota_cmd.has_url;

                        if (is_ota_cmd) {
                            ESP_LOGI(RX_TASK_TAG, "UART: OTA command received. URL: %s", ota_cmd.url);
                            if (ota_service_start_update(ota_cmd.url)) {
                                send_data(RX_TASK_TAG, "ota: started\r\n");
                            } else {
                                send_data(RX_TASK_TAG, "ota: error (running or bad URL)\r\n");
                            }
                        }

                        if (!is_led_cmd && !is_motor_cmd && !is_ota_cmd) {
                            ESP_LOGD(RX_TASK_TAG, "Not a command or failed to parse: %s", line_buf);
                        }
                    }

                    line_idx = 0; 

                } else {
                    if (line_idx < sizeof(line_buf) - 1) {
                        line_buf[line_idx++] = c;
                    } else {
                        ESP_LOGW(RX_TASK_TAG, "Line too long, dropping buffer");
                        line_idx = 0; 
                    }
                }
            } 
        } 
    } 
    free(rx_data); 
}


