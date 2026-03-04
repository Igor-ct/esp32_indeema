#pragma once

#include <stdbool.h>

int send_data(const char* logName, const char* data);
void uart_component_init(void);
void rx_task(void *arg);