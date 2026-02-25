# esp32_indeema

Simple ESP-IDF training project

## What it does

- Prints basic chip, flash and heap information (based on `hello_world` example)
- Blinks an LED using a custom component `led_ctrl`
- LED parameters (GPIO, active level, blink period) are configurable via `menuconfig`
- **[NEW]** Demonstrates FreeRTOS Queues by sending and receiving simulated sensor data (temperature).
- **[NEW]** Simulates CPU load using two dedicated tasks pinned to different cores.
- **[NEW]** Monitors and logs system status every 5 seconds, including free heap, active tasks, resource usage (CPU time), and core affinity.

## Task Implementation Details

The project implements the following FreeRTOS tasks to demonstrate multitasking capabilities:

1. **Sender_Task & Receiver_Task**:
   - `Sender_Task`: Generates simulated temperature data (randomized) and sends it to a queue.
   - `Receiver_Task`: Waits for data in the queue and logs the received temperature.
   
2. **CPU Load Tasks**:
   - `Load_A` (Core 0) & `Load_B` (Core 1): Perform arithmetic operations to simulate CPU usage. The load duration and period are configurable in code.

3. **Sys_Status**:
   - Runs every 5 seconds.
   - Prints a list of all tasks, their state (Blocked, Ready, Running), priority, and remaining stack size (High Water Mark).
   - Displays CPU usage percentage per task.

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
    └── led_ctrl/
        ├── CMakeLists.txt
        ├── include/
        │   └── led_ctrl.h      # public component API
        └── src/
            └── led_ctrl.c      # LED control implementation
```
