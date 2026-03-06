#include "ws2812.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "WS2812";
static led_strip_handle_t led_strip = NULL;

static const led_strip_config_t strip_config = {
    .strip_gpio_num = CONFIG_LED_WS2812_GPIO,
    .max_leds = CONFIG_LED_WS2812_MAX_LEDS,
    .led_model = LED_MODEL_WS2812,
    .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    .flags = { .invert_out = false, }
};

static const led_strip_rmt_config_t rmt_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = CONFIG_LED_WS2812_RMT_RESOLUTION,
    .mem_block_symbols = 64,
    .flags = { .with_dma = false, }
};

esp_err_t ws2812_init(void)
{
    if (led_strip) {
        ESP_LOGW(TAG, "LED already initialized");
        return ESP_OK;
    }

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_ERROR_CHECK(led_strip_clear(led_strip));
    ESP_LOGI(TAG, "LED initialized on GPIO %d", CONFIG_LED_WS2812_GPIO);

    return ESP_OK;
}

esp_err_t ws2812_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (!led_strip) return ESP_ERR_INVALID_STATE;

    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
    return led_strip_refresh(led_strip);
}

esp_err_t ws2812_clear(void)
{
    if (!led_strip) return ESP_ERR_INVALID_STATE;
    return led_strip_clear(led_strip);
}