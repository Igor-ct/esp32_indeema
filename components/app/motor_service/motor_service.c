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

static QueueHandle_t motor_cmd_queue = NULL;

QueueHandle_t motor_service_get_queue(void) {
    return motor_cmd_queue;
}

void motor_send_remote_command(motor_mode_t mode, float angle_deg, uint8_t priority) {
    if (motor_cmd_queue == NULL) return;
    
    motor_cmd_t cmd = {
        .type = MOTOR_CMD_REMOTE_MODE,
        .mode = mode,
        .target_angle = angle_deg,
        .priority = priority,
        .lock = (mode == MOTOR_MODE_REMOTE) ? true : false 
    };

    xQueueSend(motor_cmd_queue, &cmd, 0);
}

void motor_service_push_accel_x(int16_t accel_x) {
    if (motor_cmd_queue == NULL) return;
    motor_cmd_t cmd = { 
        .type = MOTOR_CMD_PUSH_ACCEL, 
        .accel_x = accel_x 
    };
    xQueueSend(motor_cmd_queue, &cmd, 0);
}

static void motor_service_task(void *pvParameters) {
    motor_cmd_t incoming_cmd;
    static motor_cmd_t active_remote_cmd = {0}; 
    
    int current_steps = 0; 
    int16_t latest_accel_x = 0;

    while (1) {
        
        while (xQueueReceive(motor_cmd_queue, &incoming_cmd, 0) == pdTRUE) {
            
            if (incoming_cmd.type == MOTOR_CMD_REMOTE_MODE) {
                if (incoming_cmd.priority >= active_remote_cmd.priority) {
                    active_remote_cmd = incoming_cmd;

                    if (incoming_cmd.mode != MOTOR_MODE_REMOTE) {
                        active_remote_cmd.priority = 0;
                        active_remote_cmd.lock = false;
                    }
                }
            } 
            else if (incoming_cmd.type == MOTOR_CMD_PUSH_ACCEL) {
                latest_accel_x = incoming_cmd.accel_x;
            }
        }

        float target_angle = 0.0f;

        if (active_remote_cmd.priority > 0 || active_remote_cmd.lock) {
            target_angle = active_remote_cmd.target_angle;
        } 
        else {
            if (active_remote_cmd.mode == MOTOR_MODE_JOYSTICK) {
                joystick_pos_t pos;
                if (xQueuePeek(joystick_get_pos_queue(), &pos, 0) == pdTRUE) {
                    target_angle = pos.x * MAX_ANGLE; 
                }
            } 
            else if (active_remote_cmd.mode == MOTOR_MODE_ACCEL) {
                float normalized = (latest_accel_x + 16384.0f) / 32768.0f;
                if (normalized < 0.0f) normalized = 0.0f;
                if (normalized > 1.0f) normalized = 1.0f;
                target_angle = normalized * MAX_ANGLE;
            }
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
            }
            
            vTaskDelay(pdMS_TO_TICKS(5)); 
            
        } else {
            stepper_28byj48_stop();
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

void motor_service_init(void) {
    motor_cmd_queue = xQueueCreate(10, sizeof(motor_cmd_t));
    
    servo_motor_init();
    stepper_28byj48_init();

    xTaskCreatePinnedToCore(motor_service_task, "motor_service", 4096, NULL, 3, NULL, 1);
    ESP_LOGI(TAG, "Motor Service initialized");
}