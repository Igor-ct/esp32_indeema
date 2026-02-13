# esp32_indeema

Simple ESP-IDF training project

## What it does

- Prints basic chip, flash and heap information (based on `hello_world` example)
- Blinks an LED using a custom component `led_ctrl`
- LED parameters (GPIO, active level, blink period) are configurable via `menuconfig`
- Demonstrates FreeRTOS Queues by sending and receiving simulated sensor data (temperature).
- Simulates CPU load using two dedicated tasks pinned to different cores.
- Monitors and logs system status every 5 seconds, including free heap, active tasks, resource usage (CPU time), and core affinity.
- **[NEW]** Controls a WS2812 Addressable LED (RGB) via the RMT peripheral.
- **[NEW]** Reads analog inputs from an XY Joystick using the ADC to dynamically control the WS2812 LED color/position.
- **[NEW]** Uses a physical Button with multi-gesture recognition to control the system:
  - *Press down*: Toggles the WS2812 LED lock state.
  - *Single click*: Inverts the Joystick X/Y axes.
  - *Double click*: Switches the Joystick control mode (e.g., RGB circle mode).
  - *Long press*: Toggles the WS2812 LED power on/off.
  
## Task Implementation Details

The project implements the following FreeRTOS tasks to demonstrate multitasking capabilities:

1. Sender_Task & Receiver_Task:
   - `Sender_Task`: Generates simulated temperature data (randomized) and sends it to a queue.
   - `Receiver_Task`: Waits for data in the queue and logs the received temperature.
   
2. CPU Load Tasks:
   - `Load_A` (Core 0) & `Load_B` (Core 1): Perform arithmetic operations to simulate CPU usage. The load duration and period are configurable in code.

3. Sys_Status:
   - Runs every 5 seconds.
   - Prints a list of all tasks, their state (Blocked, Ready, Running), priority, and remaining stack size (High Water Mark).
   - Displays CPU usage percentage per task.

4. **[NEW]** Joystick Update Task:
   - `joystick_update_task`: Periodically reads ADC values from the joystick and updates the WS2812 LED based on the current mode and inversion settings.

## Configuration (menuconfig)

```bash
idf.py menuconfig
```

### `Custom LED Configuration`
  - `MY_LED_ENABLE`          — Enable/disable LED logic (y/n).
  - `MY_LED_GPIO`            — GPIO number for the LED.
  - `MY_LED_ACTIVE_HIGH`     — Active high (1 = LED on when GPIO high) or active low (default: y).
  - `MY_BLINK_PERIOD_MS`     — Blink period in milliseconds (optional).

### **[NEW]** WS2812 LED Configuration
- `LED_WS2812_GPIO` — GPIO number for the WS2812 data line.
- `LED_WS2812_MAX_LEDS` — Number of LEDs in the strip.
- `LED_WS2812_RMT_RESOLUTION` — RMT peripheral resolution in Hz.

### **[NEW]** Joystick Configuration
- `JOYSTICK_ADC_UNIT` — ADC Unit to use (1 or 2).
- `JOYSTICK_X_CHANNEL` / `JOYSTICK_Y_CHANNEL` — ADC channels for X and Y axes.
- `JOYSTICK_UPDATE_INTERVAL` — Read and update interval in milliseconds.
- `JOYSTICK_ENABLE_CIRCLE_MODE` — Enables RGB circle mode by default.

### **[NEW]** Button Configuration
- `BUTTON_GPIO` — GPIO number for the control button.
- `BUTTON_ACTIVE_LEVEL` — Active hardware level (0 = LOW, 1 = HIGH).
- `BUTTON_LONG_PRESS_TIME` — Duration to trigger a long press in ms.
- `BUTTON_SHORT_PRESS_TIME` — Max duration for a short press/click in ms.

### FreeRTOS Configuration (Required for System Status)

To enable task statistics (CPU usage and Stack monitoring), you must enable the following options in `menuconfig`:

1. Go to `Component config` → `FreeRTOS` → `Kernel`.
2. Enable:
   - `configUSE_TRACE_FACILITY`
   - `configGENERATE_RUN_TIME_STATS`
   - `configUSE_STATS_FORMATTING_FUNCTIONS`
   
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
    ├── led_ctrl/
    │   ├── CMakeLists.txt
    │   ├── include/
    │   │   └── led_ctrl.h      # public component API
    │   └── src/
    │       └── led_ctrl.c      # LED control implementation
    ├── led2/                   # [NEW] WS2812 Addressable LED control component
    ├── joystick/               # [NEW] ADC Joystick reading and LED mapping component
    └── button/                 # [NEW] Button event handler (uses esp-iot-solution)
```
