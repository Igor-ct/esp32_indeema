#include "joystick_controller.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define JOYSTICK_X_CHANNEL       CONFIG_JOYSTICK_X_CHANNEL
#define JOYSTICK_Y_CHANNEL       CONFIG_JOYSTICK_Y_CHANNEL
#define JOYSTICK_ADC_UNIT        (CONFIG_JOYSTICK_ADC_UNIT == 1 ? ADC_UNIT_1 : ADC_UNIT_2)

static const char *TAG = "JOYSTICK";
static adc_oneshot_unit_handle_t adc_handle;
static bool joystick_inverted = false;

QueueHandle_t joystick_pos_queue = NULL;
QueueHandle_t joystick_zone_queue = NULL;

static void joystick_task(void *arg) {
    joystick_pos_t pos;
    joystick_zone_t current_zone = JOYSTICK_ZONE_CENTER;

    while(1) {
        int raw_x = 0, raw_y = 0;
        
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, JOYSTICK_X_CHANNEL, &raw_x));
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, JOYSTICK_Y_CHANNEL, &raw_y));
        pos.x = raw_x / 4095.0f;
        pos.y = raw_y / 4095.0f;
        
        if (joystick_inverted) {
            pos.x = 1.0f - pos.x;
            pos.y = 1.0f - pos.y;
        }

        xQueueOverwrite(joystick_pos_queue, &pos);

        joystick_zone_t new_zone = JOYSTICK_ZONE_CENTER;
        if (pos.x < 0.10f)      new_zone = JOYSTICK_ZONE_LEFT;
        else if (pos.x > 0.90f) new_zone = JOYSTICK_ZONE_RIGHT;
        else if (pos.y < 0.10f) new_zone = JOYSTICK_ZONE_DOWN;

        if (new_zone != current_zone) {
            current_zone = new_zone;
            xQueueSend(joystick_zone_queue, &new_zone, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(JOYSTICK_UPDATE_INTERVAL)); 
    }
}

esp_err_t joystick_init(void)
{
    joystick_pos_queue = xQueueCreate(1, sizeof(joystick_pos_t));
    joystick_zone_queue = xQueueCreate(5, sizeof(joystick_zone_t));

    if (joystick_pos_queue == NULL || joystick_zone_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS queues!");
        return ESP_FAIL;
    }

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = JOYSTICK_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, JOYSTICK_X_CHANNEL, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, JOYSTICK_Y_CHANNEL, &chan_cfg));

    xTaskCreate(joystick_task, "joystick_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Joystick initialized (X:%d Y:%d)", JOYSTICK_X_CHANNEL, JOYSTICK_Y_CHANNEL);
    return ESP_OK;
}

void joystick_toggle_inversion(void)
{
    joystick_inverted = !joystick_inverted;
    ESP_LOGI(TAG, "Inversion: %s", joystick_inverted ? "ON" : "OFF");
}

QueueHandle_t joystick_get_pos_queue(void) {
    return joystick_pos_queue;
}

QueueHandle_t joystick_get_zone_queue(void) {
    return joystick_zone_queue;
}