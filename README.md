# esp32_indeema

Simple ESP-IDF training project

## What it does

- Prints basic chip, flash and heap information (based on `hello_world` example)
- Blinks an LED using a custom component `led_ctrl`
- LED parameters (GPIO, active level, blink period) are configurable via `menuconfig`

## Configuration (menuconfig)

```bash
idf.py menuconfig
```

- Menu path:
- Component config → Custom LED Configuration
- Available options:

- MY_LED_ENABLE          — Enable/disable LED logic (y/n)
- MY_LED_GPIO            — GPIO number for the LED
- MY_LED_ACTIVE_HIGH     — Active high (1 = LED on when GPIO high) or active low -  (default: y)
- MY_BLINK_PERIOD_MS     — Blink period in milliseconds (optional)

## Build & Flash
- Build the project
 ```idf.py build```
- Flash + monitor (real hardware)
```idf.py -p PORT flash monitor```
- Exit monitor: Ctrl + ]
- Run in QEMU
- ```idf.py qemu monitor```
- Exit monitor: Ctrl + ]

## Project structure
```
esp32_indeema/
├── CMakeLists.txt
├── Kconfig.projbuild
├── sdkconfig.defaults
├── main/
│   └── main.c                  # entry point, chip info + LED usage
└── components/
    └── led_ctrl/
        ├── CMakeLists.txt
        ├── include/
        │   └── led_ctrl.h      # public component API
        └── src/
            └── led_ctrl.c      # LED control implementation
```