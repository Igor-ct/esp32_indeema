#include "my_I2C.h"
#include "esp_log.h"

static const char *TAG = "MY_I2C";

#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA_IO
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL_IO

static i2c_master_bus_handle_t s_i2c_bus_handle = NULL;

void i2c_bus_init(void)
{
    if (s_i2c_bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C bus is already initialized");
        return;
    }

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,  
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, 
    };

    esp_err_t err = i2c_new_master_bus(&i2c_bus_config, &s_i2c_bus_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "I2C bus initialized successfully");
    } else {
        ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(err));
    }
}

i2c_master_bus_handle_t i2c_bus_get_handle(void)
{
    if (s_i2c_bus_handle == NULL) {
        ESP_LOGE(TAG, "Warning: Requested I2C handle before initialization!");
    }
    return s_i2c_bus_handle;
}