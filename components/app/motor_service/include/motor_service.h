#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    MOTOR_MODE_REMOTE,   
    MOTOR_MODE_JOYSTICK, 
    MOTOR_MODE_ACCEL     
} motor_mode_t;

typedef enum {
    MOTOR_CMD_REMOTE_MODE, 
    MOTOR_CMD_PUSH_ACCEL
} motor_cmd_type_t;

typedef struct {
    motor_cmd_type_t type; 
    motor_mode_t mode;      
    float target_angle;     
    int accel_x;            
    uint8_t priority;       
    bool lock;              
} motor_cmd_t;

QueueHandle_t motor_service_get_queue(void);
void motor_service_init(void);

void motor_send_remote_command(motor_mode_t mode, float angle_deg, uint8_t priority);

void motor_service_push_accel_x(int16_t accel_x);