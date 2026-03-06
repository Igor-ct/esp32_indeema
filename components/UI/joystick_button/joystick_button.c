#include "joystick_button.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

static QueueHandle_t button_event_queue = NULL;

static void send_button_event(joystick_button_event_t evt) {
    if (button_event_queue != NULL) {
        xQueueSend(button_event_queue, &evt, 0); 
    }
}

static void button_cb_off_led(void *btn, void *usr)       { send_button_event(BTN_EVT_LONG_PRESS);   ESP_LOGI(TAG, "Event: Long Press"); }
static void button_cb_lock_led(void *btn, void *usr)      { send_button_event(BTN_EVT_PRESS_DOWN);   ESP_LOGI(TAG, "Event: Press Down"); }
static void button_cb_change_joystick(void *btn, void *usr){ send_button_event(BTN_EVT_DOUBLE_CLICK); ESP_LOGI(TAG, "Event: Double Click"); }
static void button_cb_invert_joystick(void *btn, void *usr){ send_button_event(BTN_EVT_SINGLE_CLICK); ESP_LOGI(TAG, "Event: Single Click"); }

QueueHandle_t joystick_button_get_queue(void) {
    return button_event_queue;
}

esp_err_t joystick_button_init(void)
{
    button_event_queue = xQueueCreate(10, sizeof(joystick_button_event_t));
    if (button_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create button queue");
        return ESP_FAIL;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << CONFIG_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = CONFIG_BUTTON_ACTIVE_LEVEL == 0 ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = CONFIG_BUTTON_ACTIVE_LEVEL == 1 ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    const button_config_t btn_cfg = {
        .long_press_time  = CONFIG_BUTTON_LONG_PRESS_TIME,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME,
    };
    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = CONFIG_BUTTON_GPIO,
        .active_level = CONFIG_BUTTON_ACTIVE_LEVEL,
    };

    button_handle_t gpio_btn = NULL;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &gpio_btn);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Button create failed");
        return ret;
    }

    ESP_ERROR_CHECK(iot_button_register_cb(gpio_btn, BUTTON_EVENT_LONG_PRESS, NULL, button_cb_off_led, NULL));
    ESP_ERROR_CHECK(iot_button_register_cb(gpio_btn, BUTTON_EVENT_PRESS_DOWN, NULL, button_cb_lock_led, NULL));
    ESP_ERROR_CHECK(iot_button_register_cb(gpio_btn, BUTTON_EVENT_DOUBLE_CLICK, NULL, button_cb_change_joystick, NULL));
    ESP_ERROR_CHECK(iot_button_register_cb(gpio_btn, BUTTON_EVENT_SINGLE_CLICK, NULL, button_cb_invert_joystick, NULL));

    ESP_LOGI(TAG, "Button initialized on GPIO %d", CONFIG_BUTTON_GPIO);
    return ESP_OK;
}