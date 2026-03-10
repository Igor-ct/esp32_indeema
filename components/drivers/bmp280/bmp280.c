#include "bmp280.h"
#include "i2c.h"

#define BMP280_ADDR 0x77

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_t;

static i2c_master_dev_handle_t bmp280_dev_handle;
static bmp280_calib_t calib;
static int32_t t_fine;

static float bmp280_compensate_temp(int32_t raw_temp) {
    int32_t var1, var2, T;
    var1 = ((((raw_temp >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((raw_temp >> 4) - ((int32_t)calib.dig_T1)) * ((raw_temp >> 4) - ((int32_t)calib.dig_T1))) >> 12) * ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return (float)T / 100.0f; 
}

static float bmp280_compensate_press(int32_t raw_press) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    if (var1 == 0) return 0; 
    p = 1048576 - raw_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (float)p / 256.0f;
}

esp_err_t bmp280_init(void) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BMP280_ADDR,
        .scl_speed_hz = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(i2c_bus_get_handle(), &dev_cfg, &bmp280_dev_handle);
    if (err != ESP_OK) return err;

    uint8_t calib_reg = 0x88;
    err = i2c_master_transmit_receive(bmp280_dev_handle, &calib_reg, 1, (uint8_t*)&calib, 24, -1);
    if (err != ESP_OK) return err;

    uint8_t config_cmd[2] = {0xF4, 0x27}; 
    return i2c_master_transmit(bmp280_dev_handle, config_cmd, sizeof(config_cmd), -1);
}

static esp_err_t bmp280_read_raw(int32_t *raw_temp, int32_t *raw_press) {
    uint8_t reg_addr = 0xF7; 
    uint8_t data[6];

    esp_err_t err = i2c_master_transmit_receive(bmp280_dev_handle, &reg_addr, 1, data, 6, -1);
    if (err != ESP_OK) return err;

    *raw_press = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)data[2] >> 4);
    *raw_temp  = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)data[5] >> 4);
    
    return ESP_OK;
}

esp_err_t bmp280_read(float *temperature, float *pressure) {
    int32_t raw_temp, raw_press;
    
    esp_err_t err = bmp280_read_raw(&raw_temp, &raw_press);
    if (err != ESP_OK) return err;
    
    *temperature = bmp280_compensate_temp(raw_temp);
    *pressure = bmp280_compensate_press(raw_press);
    
    return ESP_OK;
}

