#include "lsmd6ds3.h"
#include "spi.h"
#include <string.h>

static spi_device_handle_t lsm_handle;

esp_err_t lsm6ds3_init(int cs_pin) {
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000, 
        .mode = 3,                 
        .spics_io_num = cs_pin,
        .queue_size = 1,
    };
    
    esp_err_t err = spi_bus_add_device(spi_bus_get_host(), &devcfg, &lsm_handle);
    if (err != ESP_OK) return err;

    uint8_t tx_data[2] = {0x10, 0x10}; 
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
    };
    return spi_device_transmit(lsm_handle, &t);
}

esp_err_t lsm6ds3_read_accel(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t tx_buf[7] = {0x28 | 0x80, 0, 0, 0, 0, 0, 0}; 
    uint8_t rx_buf[7] = {0};

    spi_transaction_t t = {
        .length = 8 * sizeof(tx_buf),   
        .tx_buffer = tx_buf,             
        .rx_buffer = rx_buf,
    };

    esp_err_t err = spi_device_transmit(lsm_handle, &t);
    if (err == ESP_OK) {
        *x = (int16_t)((rx_buf[2] << 8) | rx_buf[1]);
        *y = (int16_t)((rx_buf[4] << 8) | rx_buf[3]);
        *z = (int16_t)((rx_buf[6] << 8) | rx_buf[5]);
    }
    return err;
}