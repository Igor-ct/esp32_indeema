#pragma once

#include "ws2812.h"
#include <stdbool.h>

int sendData(const char* logName, const char* data);
void uart_component_init(void);
led_cmd_t get_uart_target_color(void);
bool get_uart_status_overriden_led(void);
void rx_task(void *arg);