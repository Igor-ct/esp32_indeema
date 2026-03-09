#include "bmp280.h"
#include "i2c.h"

#define BMP280_ADDR 0x76 

static i2c_master_dev_handle_t bmp280_dev_handle;

esp_err_t bmp280_init(void) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BMP280_ADDR,
        .scl_speed_hz = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(i2c_bus_get_handle(), &dev_cfg, &bmp280_dev_handle);
    if (err != ESP_OK) return err;

    uint8_t config_cmd[2] = {0xF4, 0x27}; 
    return i2c_master_transmit(bmp280_dev_handle, config_cmd, sizeof(config_cmd), -1);
}

esp_err_t bmp280_read_raw(int32_t *raw_temp, int32_t *raw_press) {
    uint8_t reg_addr = 0xF7; 
    uint8_t data[6];

    esp_err_t err = i2c_master_transmit_receive(bmp280_dev_handle, &reg_addr, 1, data, 6, -1);
    if (err != ESP_OK) return err;

    *raw_press = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    *raw_temp  = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    
    return ESP_OK;
}