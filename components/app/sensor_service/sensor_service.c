#include "sensor_service.h"
#include "esp_log.h"

#include "i2c.h"
#include "spi.h"

#if CONFIG_SENSOR_ENABLE_AHT20
#include "aht20.h"
#endif

#if CONFIG_SENSOR_ENABLE_BMP280
#include "bmp280.h"
#endif

#if CONFIG_SENSOR_ENABLE_LSM6DS3
#include "lsmd6ds3.h"
#endif

static const char *TAG = "SENSOR_SERVICE";

static bool aht20_ok = false;
static bool bmp280_ok = false;
static bool lsm6ds3_ok = false;

void sensor_service_init(void)
{
    i2c_bus_init();
    spi_bus_init();

#if CONFIG_SENSOR_ENABLE_AHT20
    if (aht20_init() == ESP_OK) {
        aht20_ok = true;
        ESP_LOGI(TAG, "AHT20 initialized");
    } else {
        ESP_LOGE(TAG, "AHT20 init failed");
    }
#endif

#if CONFIG_SENSOR_ENABLE_BMP280
    if (bmp280_init() == ESP_OK) {
        bmp280_ok = true;
        ESP_LOGI(TAG, "BMP280 initialized");
    } else {
        ESP_LOGE(TAG, "BMP280 init failed");
    }
#endif

#if CONFIG_SENSOR_ENABLE_LSM6DS3
    if (lsm6ds3_init(10) == ESP_OK) {
        lsm6ds3_ok = true;
        ESP_LOGI(TAG, "LSM6DS3 initialized");
    } else {
        ESP_LOGE(TAG, "LSM6DS3 init failed");
    }
#endif

}

bool sensor_service_read(sensor_data_t *data)
{
    if (!data) return false;

#if CONFIG_SENSOR_ENABLE_AHT20
    if (aht20_ok)
    {
        if (aht20_read(&data->aht20.temperature,
                       &data->aht20.humidity) == ESP_OK)
        {
            data->aht20.valid = true;
        }
    }
#endif

#if CONFIG_SENSOR_ENABLE_BMP280
    if (bmp280_ok)
    {
        if (bmp280_read(&data->bmp280.temperature, &data->bmp280.pressure) == ESP_OK)
        {
            data->bmp280.valid = true;
        }
    }
#endif

#if CONFIG_SENSOR_ENABLE_LSM6DS3
    if (lsm6ds3_ok)
    {
        if (lsm6ds3_read_accel(
                &data->accel.x,
                &data->accel.y,
                &data->accel.z) == ESP_OK)
        {
            data->accel.valid = true;
        }
    }
#endif

    return true;
}