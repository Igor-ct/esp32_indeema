#pragma once
#include "driver/ledc.h"

#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2400
#define SERVO_MAX_DEGREE        180

esp_err_t servo_motor_init(void);
esp_err_t servo_motor_set_angle(float angle);