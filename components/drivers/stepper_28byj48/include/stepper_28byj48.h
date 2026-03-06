#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

esp_err_t stepper_28byj48_init(void);

void stepper_28byj48_move(int steps, bool clockwise, uint32_t step_delay_ms);

void stepper_28byj48_stop(void);