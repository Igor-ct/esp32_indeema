#include "spi.h"
#include "esp_log.h"

static const char *TAG = "SPI";

#define SPI_MISO_PIN CONFIG_SPI_MISO_PIN
#define SPI_MOSI_PIN CONFIG_SPI_MOSI_PIN
#define SPI_SCLK_PIN CONFIG_SPI_SCLK_PIN

static const spi_host_device_t s_spi_host = SPI2_HOST;

static bool s_spi_initialized = false;

void spi_bus_init(void)
{
    if (s_spi_initialized) {
        ESP_LOGW(TAG, "SPI bus is already initialized");
        return;
    }

    spi_bus_config_t buscfg = {
        .miso_io_num = SPI_MISO_PIN,
        .mosi_io_num = SPI_MOSI_PIN,
        .sclk_io_num = SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092, 
    };

    esp_err_t ret = spi_bus_initialize(s_spi_host, &buscfg, SPI_DMA_CH_AUTO);
    
    if (ret == ESP_OK) {
        s_spi_initialized = true;
        ESP_LOGI(TAG, "SPI bus initialized successfully on host %d", s_spi_host);
    } else {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
    }
}

spi_host_device_t spi_bus_get_host(void)
{
    if (!s_spi_initialized) {
        ESP_LOGE(TAG, "Warning: Requested SPI host before initialization!");
    }
    return s_spi_host;
}