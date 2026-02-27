#include "my_AHT20.h"
#include "my_I2C.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AHT20_ADDR 0x38

static i2c_master_dev_handle_t aht20_dev_handle;

esp_err_t aht20_init(void) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AHT20_ADDR,
        .scl_speed_hz = 100000,
    };
    return i2c_master_bus_add_device(i2c_bus_get_handle(), &dev_cfg, &aht20_dev_handle);
}

esp_err_t aht20_read(float *temperature, float *humidity) {
    uint8_t cmd[3] = {0xAC, 0x33, 0x00}; 
    uint8_t data[6];

    esp_err_t err = i2c_master_transmit(aht20_dev_handle, cmd, sizeof(cmd), -1);
    if (err != ESP_OK) return err;

    vTaskDelay(pdMS_TO_TICKS(80)); 

    err = i2c_master_receive(aht20_dev_handle, data, sizeof(data), -1);
    if (err != ESP_OK) return err;

    if ((data[0] & 0x80) == 0) {
        uint32_t raw_hum = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
        uint32_t raw_temp = (((uint32_t)(data[3] & 0x0F)) << 16) | ((uint32_t)data[4] << 8) | data[5];

        *humidity = ((float)raw_hum / 1048576.0f) * 100.0f;
        *temperature = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;
        return ESP_OK;
    }
    return ESP_FAIL;
}