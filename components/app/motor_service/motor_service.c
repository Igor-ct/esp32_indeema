#include "motor_service.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "sg92r.h"
#include "stepper_28byj48.h"
#include "joystick_controller.h"

static const char *TAG = "MOTOR_SVC";

#define MAX_ANGLE 180.0f
#define STEPS_PER_180_DEG 2048 

static motor_mode_t current_mode = MOTOR_MODE_REMOTE;
static float target_angle = 0.0f;
static int16_t latest_accel_x = 0;

static QueueHandle_t motor_cmd_queue;

typedef struct {
    motor_mode_t mode;
    float target_angle;
} motor_cmd_t;

void motor_service_set_mode(motor_mode_t mode) {
    motor_cmd_t cmd = { .mode = mode, .target_angle = target_angle };
    xQueueSend(motor_cmd_queue, &cmd, 0);
}

void motor_service_set_angle(float angle_deg) {
    motor_cmd_t cmd = { .mode = MOTOR_MODE_REMOTE, .target_angle = angle_deg };
    xQueueSend(motor_cmd_queue, &cmd, 0);
}

void motor_service_push_accel_x(int16_t accel_x) {
    latest_accel_x = accel_x; 
}

static void motor_service_task(void *pvParameters) {
    motor_cmd_t incoming_cmd;
    int current_steps = 0; 

    while (1) {
        if (xQueueReceive(motor_cmd_queue, &incoming_cmd, 0) == pdTRUE) {
            current_mode = incoming_cmd.mode;
            if (current_mode == MOTOR_MODE_REMOTE) {
                target_angle = incoming_cmd.target_angle;
            }
            ESP_LOGI(TAG, "Mode updated: %d, Target Angle: %.1f", current_mode, target_angle);
        }

        if (current_mode == MOTOR_MODE_JOYSTICK) {
            joystick_pos_t pos;
            if (xQueuePeek(joystick_get_pos_queue(), &pos, 0) == pdTRUE) {
                target_angle = pos.x * MAX_ANGLE; 
            }
        } 
        else if (current_mode == MOTOR_MODE_ACCEL) {
            float normalized = (latest_accel_x + 16384.0f) / 32768.0f;
            if (normalized < 0.0f) normalized = 0.0f;
            if (normalized > 1.0f) normalized = 1.0f;
            target_angle = normalized * MAX_ANGLE;
        }

        if (target_angle < 0.0f) target_angle = 0.0f;
        if (target_angle > MAX_ANGLE) target_angle = MAX_ANGLE;

        int target_steps = (target_angle / MAX_ANGLE) * STEPS_PER_180_DEG;

        if (current_steps != target_steps) {
            bool clockwise = (target_steps > current_steps);
            
            stepper_28byj48_move(1, clockwise, 0); 
            
            if (clockwise) current_steps++;
            else current_steps--;

            if (current_steps % 16 == 0) {
                float current_angle = ((float)current_steps / STEPS_PER_180_DEG) * MAX_ANGLE;
                servo_motor_set_angle(current_angle);
                
                ESP_LOGI(TAG, "Moving... Step: %d, Angle: %.1f", current_steps, current_angle);
            }
            
            vTaskDelay(pdMS_TO_TICKS(20)); 
            
        } else {
            stepper_28byj48_stop();
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

void motor_service_init(void) {
    motor_cmd_queue = xQueueCreate(5, sizeof(motor_cmd_t));
    
    servo_motor_init();
    stepper_28byj48_init();

    xTaskCreate(motor_service_task, "motor_service", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Motor Service initialized");
}