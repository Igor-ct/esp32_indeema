# esp32_indeema

Simple ESP-IDF training project

## What it does

- Prints basic chip, flash and heap information (based on `hello_world` example)
- Blinks an LED using a custom component `led_ctrl`
- LED parameters (GPIO, active level, blink period) are configurable via `menuconfig`
- Demonstrates FreeRTOS Queues by sending and receiving simulated sensor data (temperature).
- Simulates CPU load using two dedicated tasks pinned to different cores.
- Monitors and logs system status every 5 seconds, including free heap, active tasks, resource usage (CPU time), and core affinity.
- Controls a WS2812 Addressable LED (RGB) via the RMT peripheral.
- Reads analog inputs from an XY Joystick using the ADC to dynamically control the WS2812 LED color/position.
- Uses a physical Button with multi-gesture recognition to control the system:
  - *Press down*: Toggles the WS2812 LED lock state.
  - *Single click*: Inverts the Joystick X/Y axes.
  - *Double click*: Switches the Joystick control mode (e.g., RGB circle mode).
  - *Long press*: Toggles the WS2812 LED power on/off.
- Full Wi-Fi support (AP + STA modes).
- SNTP time synchronization (in STA mode).
- HTTP server with Wi-Fi configuration page.
- Wi-Fi mode switching using the joystick (Left → AP, Right → STA).
- WS2812 LED Wi-Fi status strategy.
- **[NEW]** MQTT Remote Control: Full integration with an MQTT broker for remote monitoring and LED control.
  - **[NEW]** Supports RGB color setting and LED state toggling via JSON commands.
  - **[NEW]** Implements an "Override" logic to prioritize remote commands over local joystick control.
  - **[NEW]** Features a "Last Will and Testament" (LWT) to notify the system when the device goes offline unexpectedly.

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

4. Joystick Update Task:
   - `joystick_update_task`: Periodically reads ADC values from the joystick and updates the WS2812 LED based on the current mode and inversion settings.

5. **[NEW]** MQQT Heartbeat Task:
   - **[NEW]** `task_heartbeat`: Periodically (every 60s) publishes an "online" status and "ping" message to ensure the connection is alive.

6. **[NEW]** MQTT Commands Maneger:
   - **[NEW]** `task_cmd_manager`:Processes incoming commands from the mqtt_cmd_queue. It decodes JSON data, updates the target RGB values, and manages the LED override state.

## Wi-Fi Connectivity & System Integration

The project has been expanded with networking capabilities, enabling remote configuration and time synchronization.

### Core Networking Features

- Station (STA) Mode: Connects to existing Wi-Fi networks. It features an automatic retry mechanism and NVS (Non-Volatile Storage) integration to remember the last successful connection.
- Access Point (AP) Mode: Creates a local hotspot for initial setup or offline control.
- Dynamic Mode Switching: Transition between modes using physical joystick gestures:
- Joystick LEFT: Activate STA Mode.
- Joystick RIGHT: Activate AP Mode.
- Joystick UP: Turn Wi-Fi OFF (to save power/disable radio).
- Time Synchronization (SNTP): Once connected to the internet in STA mode, the system automatically synchronizes the internal RTC with global NTP servers.
- Embedded Web Server: A built-in HTTP server provides a web-based UI to:
- Update Wi-Fi credentials (SSID/Password) on the fly.
- Monitor system status and uptime.

## **[NEW]** MQTT Messaging & Remote Control
The system uses the MQTT protocol for low-latency communication with a central broker.

### Payload Schema (JSON)
The device listens for commands on the topic esp-lection/cmd. The expected JSON structure is:

```json
{
 "led": {
   "color": {
      "r": 255,
      "g": 0,
      "b": 0
   },
   "state": "on"
 }
}
```

- color: Sets the target RGB values for the WS2812 LED.
- state:
   - "on": Overrides the current Wi-Fi status LED and applies the MQTT color.
   - "off": Disables the override and returns the LED to showing the Wi-Fi connectivity status.

### Status 
Status Topic: esp-lection/status

- Publishes online when connected.
- Publishes offline (LWT) if the device loses connection.
- Sends a ping message every minute to verify the link.

## Configuration (menuconfig)

```bash
idf.py menuconfig
```

### Custom LED Configuration
  - `MY_LED_ENABLE`          — Enable/disable LED logic (y/n).
  - `MY_LED_GPIO`            — GPIO number for the LED.
  - `MY_LED_ACTIVE_HIGH`     — Active high (1 = LED on when GPIO high) or active low (default: y).
  - `MY_BLINK_PERIOD_MS`     — Blink period in milliseconds (optional).

### WS2812 LED Configuration
- `LED_WS2812_GPIO` — GPIO number for the WS2812 data line.
- `LED_WS2812_MAX_LEDS` — Number of LEDs in the strip.
- `LED_WS2812_RMT_RESOLUTION` — RMT peripheral resolution in Hz.

### Joystick Configuration
- `JOYSTICK_ADC_UNIT` — ADC Unit to use (1 or 2).
- `JOYSTICK_X_CHANNEL` / `JOYSTICK_Y_CHANNEL` — ADC channels for X and Y axes.
- `JOYSTICK_UPDATE_INTERVAL` — Read and update interval in milliseconds.
- `JOYSTICK_ENABLE_CIRCLE_MODE` — Enables RGB circle mode by default.

### Button Configuration
- `BUTTON_GPIO` — GPIO number for the control button.
- `BUTTON_ACTIVE_LEVEL` — Active hardware level (0 = LOW, 1 = HIGH).
- `BUTTON_LONG_PRESS_TIME` — Duration to trigger a long press in ms.
- `BUTTON_SHORT_PRESS_TIME` — Max duration for a short press/click in ms.
- `JOYSTICK_ENABLE_WIFI_MODE` - Enable wifi mode

### WIFI_AP Configuration
- `ESP_WIFI_SSID_AP` - WiFi SSID
- `ESP_WIFI_PASSWORD_AP` - WiFi Password
- `ESP_WIFI_CHANNEL` - WiFi Channel
- `ESP_MAX_STA_CONN` - Maximal STA connections
- `ESP_GTK_REKEYING_ENABLE` - Enable GTK Rekeying
- `ESP_GTK_REKEY_INTERVAL` - GTK rekey interval

### WIFI_STA Configuration
- `ESP_WIFI_SSID_STA`  - WiFi SSID
- `ESP_WIFI_PASSWORD_STA` - WiFi Password
- `ESP_WIFI_SAE_MODE` - WPA3 SAE mode selectionWPA3 SAE mode selection
- `ESP_WIFI_PW_ID` - PASSWORD IDENTIFIER
- `ESP_MAXIMUM_RETRY` - Maximum retry
- `ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD` - WiFi Scan auth mode threshold

### SNTP Configuration
- `SNTP_TIME_SERVER` - SNTP server name
- `SNTP_TIME_SYNC_METHOD` - Time synchronization method

### **[NEW]** MQTT Configuration
- `MQTT_BROKER_URI` - broker uri
- `MQTT_CLIENT_ID` — Unique identifier for the device on the broker.
- `MQTT_CMD_TOPIC` — Topic for receiving commands.
- `MQTT_STATUS_TOPIC` — Topic for publishing availability.

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
    ├── led2/                   # WS2812 Addressable LED control component
    ├── SNtP_sync/              # SNTP time synchronization module
    ├── HTTP_server/            # Embedded HTTP server for configuration
    ├── wifi_ap/                # Wi-Fi Access Point mode implementation
    ├── wifi_sta/               # Wi-Fi Station mode with reconnect logic
    ├── joystick/               # ADC Joystick reading and LED mapping component
    ├── button/                 # Button event handler (uses esp-iot-solution)
    └── my_mqtt                 # [NEW] MQTT client, JSON parser, and command manager
```
