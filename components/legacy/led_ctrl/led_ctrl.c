#include "led_ctrl.h"
#include "driver/gpio.h"

int led_ctrl_init(led_ctrl_t *led, int gpio, bool active_high)
{
    if (!led) return -1;

    led->gpio = gpio;
    led->active_high = active_high;

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    if (gpio_config(&io_conf) != ESP_OK) return -2;

    led_ctrl_set(led, false);
    return 0;
}

void led_ctrl_set(led_ctrl_t *led, bool on)
{
    if (!led) return;

    int level = on ? 1 : 0;
    if (!led->active_high) level = !level;

    gpio_set_level((gpio_num_t)led->gpio, level);
}

void led_ctrl_toggle(led_ctrl_t *led)
{
    if (!led) return;
    int cur = gpio_get_level((gpio_num_t)led->gpio);
    gpio_set_level((gpio_num_t)led->gpio, !cur);
}