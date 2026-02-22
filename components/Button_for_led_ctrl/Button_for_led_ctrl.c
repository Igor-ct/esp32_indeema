#include "Button_for_led_ctrl.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led2.h"
#include "Joy_stick_control_led.h"

static const char *TAG = "BUTTON";

static const button_config_t btn_cfg = {
    .long_press_time  = CONFIG_BUTTON_LONG_PRESS_TIME,
    .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME,
};

static void button_cb_off_led(void *btn_handle, void *usr_data)
{
    ESP_LOGI(TAG, "Long press -> Toggle LED power");

    if (!led2_is_initialized()) {
        led2_init();
        led2_set_rgb(0, 0, 0);
    } else {
        led2_deinit();
    }
}

static void button_cb_lock_led(void *btn_handle, void *usr_data)
{
    ESP_LOGI(TAG, "Press down -> Toggle LED lock");

    if (led2_is_initialized()) {
        led2_toggle_lock();
    }
}

static void button_cb_change_joystick(void *btn_handle, void *usr_data)
{
    ESP_LOGI(TAG, "Double click -> Change joystick mode");
    joystick_mode_switch();
}

static void button_cb_invert_joystick(void *btn_handle, void *usr_data)
{
    ESP_LOGI(TAG, "Single click -> Invert joystick");
    joystick_toggle_inversion();   
}


esp_err_t button_for_led_ctrl_init(void)
{
    esp_err_t ret;

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << CONFIG_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = CONFIG_BUTTON_ACTIVE_LEVEL == 0 ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = CONFIG_BUTTON_ACTIVE_LEVEL == 1 ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = CONFIG_BUTTON_GPIO,
        .active_level = CONFIG_BUTTON_ACTIVE_LEVEL,
    };

    button_handle_t gpio_btn = NULL;

    ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &gpio_btn);
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
