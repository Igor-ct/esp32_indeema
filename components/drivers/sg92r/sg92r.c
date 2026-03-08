#include "sg92r.h"
#include "esp_log.h"
#include "driver/ledc.h"

static const char *TAG = "SERVO_DRV";

#define SERVO_LEDC_TIMER       LEDC_TIMER_0
#define SERVO_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_CHANNEL     LEDC_CHANNEL_0
#define SERVO_LEDC_RESOLUTION  LEDC_TIMER_14_BIT 

esp_err_t servo_motor_init(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = SERVO_LEDC_MODE,
        .timer_num       = SERVO_LEDC_TIMER,
        .duty_resolution = SERVO_LEDC_RESOLUTION,
        .freq_hz         = 50, 
        .clk_cfg         = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&timer_cfg);
    if (err != ESP_OK) return err;

    ledc_channel_config_t channel_cfg = {
        .gpio_num       = CONFIG_SG92R_PWM_PIN, 
        .speed_mode     = SERVO_LEDC_MODE,
        .channel        = SERVO_LEDC_CHANNEL,
        .timer_sel      = SERVO_LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    err = ledc_channel_config(&channel_cfg);
    
    ESP_LOGI(TAG, "Servo initialized on pin %d", CONFIG_SG92R_PWM_PIN);
    return err;
}

esp_err_t servo_motor_set_angle(float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > SERVO_MAX_DEGREE) angle = SERVO_MAX_DEGREE;

    uint32_t pulse_us = SERVO_MIN_PULSEWIDTH_US + 
        (uint32_t)((SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) * (angle / SERVO_MAX_DEGREE));

    uint32_t duty = (pulse_us * (1 << 14)) / 20000;

    esp_err_t err = ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
    if (err == ESP_OK) {
        err = ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
    }
    return err;
}