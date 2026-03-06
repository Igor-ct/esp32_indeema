#include <stdio.h>
#include "platform_init.h"
#include "nvs_flash.h"
#include "uart.h"
#include "joystick_controller.h"
#include "joystick_button.h"
#include "ws2812.h"
#include "esp_netif.h"
#include "esp_event.h"

void platform_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    uart_component_init(); 
    
    ws2812_init();
    joystick_init();
    joystick_button_init();
}