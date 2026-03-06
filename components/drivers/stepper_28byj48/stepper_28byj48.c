#include <stdio.h>
#include "stepper_28byj48.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "STEPPER_28BYJ48";

static const gpio_num_t pins[4] = {
    CONFIG_STEPPER_IN1_PIN,
    CONFIG_STEPPER_IN2_PIN,
    CONFIG_STEPPER_IN3_PIN,
    CONFIG_STEPPER_IN4_PIN
};

static const uint8_t step_sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

esp_err_t stepper_28byj48_init(void) {
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(pins[i]);
        gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(pins[i], 0);
    }
    ESP_LOGI(TAG, "Initialized on pins: %d, %d, %d, %d", pins[0], pins[1], pins[2], pins[3]);
    return ESP_OK;
}

static void set_step(uint8_t step) {
    for (int i = 0; i < 4; i++) {
        gpio_set_level(pins[i], step_sequence[step][i]);
    }
}

void stepper_28byj48_stop(void) {
    for (int i = 0; i < 4; i++) {
        gpio_set_level(pins[i], 0);
    }
}

void stepper_28byj48_move(int steps, bool clockwise, uint32_t step_delay_ms) {
    static int current_step = 0;

    for (int i = 0; i < steps; i++) {
        if (clockwise) {
            current_step = (current_step + 1) % 8;
        } else {
            current_step = (current_step - 1 + 8) % 8;
        }
        
        set_step(current_step);
        
        vTaskDelay(pdMS_TO_TICKS(step_delay_ms)); 
    }
}