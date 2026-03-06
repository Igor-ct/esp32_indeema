#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MOTOR_MODE_REMOTE,   
    MOTOR_MODE_JOYSTICK, 
    MOTOR_MODE_ACCEL     
} motor_mode_t;

void motor_service_init(void);

void motor_service_set_mode(motor_mode_t mode);

void motor_service_set_angle(float angle_deg);

void motor_service_push_accel_x(int16_t accel_x);