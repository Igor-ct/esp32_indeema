# System Initialization

This module initializes the hardware platform and essential components required for the firmware to operate correctly.

---

## Platform Initialization

```c
void platform_init(void);
```

**Purpose:** Initializes core system services, drivers, and peripherals at startup.

### Initialization Steps

1. Non-Volatile Storage (NVS)
```c
esp_err_t ret = nvs_flash_init();
if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
}
ESP_ERROR_CHECK(ret);
```
- Initializes the flash storage for persistent data.
- Automatically erases and reinitializes if necessary.

2. Networking Stack
```c
ESP_ERROR_CHECK(esp_netif_init());
ESP_ERROR_CHECK(esp_event_loop_create_default());
```
- Sets up ESP-IDF network interfaces.
- Creates a default event loop for system events.

3. UART Component
```c
uart_component_init();
```
- Initializes the UART driver for serial communication.

4. LED / WS2812 Control

```c
ws2812_init();
```
- Sets up the WS2812 LED strip driver.

5. Joystick and Buttons
```c
joystick_init();
joystick_button_init();
```
- Initializes the joystick input subsystem.
- Initializes joystick button handling.

> **Note**
> - `platform_init()` must be called exactly **once** at startup, before any application tasks or network services are created.
> - **Hardware Readiness**: UART, WS2812, and Joystick are ready for immediate use after this call.
> - **Abstraction Layer**: This function decouples business logic from low-level hardware dependencies.

**Example Usage:**
```c
#include "platform_init.h"

void app_main(void)
{
    platform_init(); // Initialize system

    // Application code can now safely use UART, LEDs, Joystick, etc.
}
```