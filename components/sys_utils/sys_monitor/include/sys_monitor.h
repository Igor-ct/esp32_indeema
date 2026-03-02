#pragma once

#include <stdint.h>

typedef struct {
    const char *name;
    uint32_t period_ms;
    uint32_t busy_ms;
} load_cfg_t;

void sys_monitor_print_chip_info(void);

void task_system_status(void *pvParameters);

void task_cpu_load(void *pvParameters);

