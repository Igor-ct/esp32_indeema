#include "led2.h"
#include "esp_log.h"
#include "led_strip.h"
#include "freertos/semphr.h"

static const char *TAG = "LED2";

static bool led_locked = false;
static led_strip_handle_t led_strip = NULL;
static wifi_led_state_t current_wifi_state = WIFI_LED_OFF;
static bool block_wifi_led = true;

static const led_strip_config_t strip_config = {
    .strip_gpio_num = CONFIG_LED_WS2812_GPIO,
    .max_leds = CONFIG_LED_WS2812_MAX_LEDS,
    .led_model = LED_MODEL_WS2812,
    .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    .flags = {
        .invert_out = false,
    }
};

static const led_strip_rmt_config_t rmt_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = CONFIG_LED_WS2812_RMT_RESOLUTION,
    .mem_block_symbols = 64,
    .flags = {
        .with_dma = false,
    }
};

esp_err_t led2_init(void)
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


void led2_toggle_lock(void)
{
    if (!led_strip) {
        ESP_LOGW(TAG, "Cannot lock: LED not initialized");
        return;
    }

    led_locked = !led_locked;

    ESP_LOGI(TAG, "LED lock: %s", led_locked ? "ON" : "OFF");
}

esp_err_t led2_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (!led_strip)
        return ESP_ERR_INVALID_STATE;

    if (led_locked || !block_wifi_led) return ESP_OK;

    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));

    ESP_ERROR_CHECK(led_strip_refresh(led_strip));

    return ESP_OK;
}

esp_err_t led2_deinit(void)
{
    if (!led_strip)
        return ESP_OK;

    led_strip_clear(led_strip);
    led_strip_del(led_strip);

    led_strip = NULL;
    led_locked = false;

    ESP_LOGI(TAG, "LED deinitialized");

    return ESP_OK;
}
bool led2_is_initialized(void)
{
    return led_strip != NULL;
}

bool led2_is_lock(void)
{
    return led_locked;
}

void led2_set_wifi_state(wifi_led_state_t state)
{
    current_wifi_state = state;
    ESP_LOGI(TAG, "WiFi LED State changed to: %d", state);
}

void led2_set_wifi_style(uint8_t r, uint8_t g, uint8_t b)
{
    if (!led_strip) return;
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);

}


wifi_led_state_t led2_get_wifi_state(void)
{
    return current_wifi_state;
}

void led2_set_block_wifi(void)
{
    block_wifi_led = true;
}

void led2_set_unblock_wifi(void)
{
    block_wifi_led = false;
}