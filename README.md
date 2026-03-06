# esp32_indeema

Simple ESP-IDF training project

## What it does

- **[REFACTORED]** Event-Driven Architecture: Decouples hardware drivers from business logic using FreeRTOS Queues. Network modules and sensors no longer block or access hardware directly.
- **[REFACTORED]** Microservices: Introduces dedicated
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
- MQTT Remote Control: Full integration with an MQTT broker for remote monitoring and LED control.
  - Supports RGB color setting and LED state toggling via JSON commands.
  - Implements an "Override" logic to prioritize remote commands over local joystick control.
  - Features a "Last Will and Testament" (LWT) to notify the system when the device goes offline unexpectedly.
-  Bluetooth Low Energy (BLE) Control: Hosts a custom GATT server using the NimBLE stack, allowing direct control of the WS2812 LED from a smartphone. Features standard profiles (Battery, Current Time) and a custom LED control service.
-  Hierarchical Control Priority: Implements a strict control override logic where BLE commands take the highest priority, followed by MQTT remote commands, and finally Local Joystick/Wi-Fi status indications.
​- Hardware Buses: Initializes dedicated I2C and SPI buses for external peripherals.
​- Sensor Drivers: Reads environmental and motion data from AHT20, BMP280, and LSM6DS3 sensors.
​- Telemetry Broadcast: Packages live sensor data into JSON and transmits it simultaneously via UART, MQTT, and BLE.
- UART Control: Accepts JSON commands over UART to control the WS2812 LED, fully integrated into the existing priority hierarchy.

## Task Implementation Details

The project implements the following FreeRTOS tasks to demonstrate multitasking capabilities:

1. **Sender_Task & Receiver_Task:**
   - `Sender_Task`: Generates simulated temperature data (randomized) and sends it to a queue.
   - `Receiver_Task`: Waits for data in the queue and logs the received temperature.
   
2. **CPU Load Tasks:**
   - `Load_A` (Core 0) & `Load_B` (Core 1): Perform arithmetic operations to simulate CPU usage. The load duration and period are configurable in code.

3. **Sys_Status:**
   - Runs every 5 seconds.
   - Prints a list of all tasks, their state (Blocked, Ready, Running), priority, and remaining stack size (High Water Mark).
   - Displays CPU usage percentage per task.

4. **Joystick Update Task:**
   - `joystick_update_task`: Periodically reads ADC values from the joystick.

5. **MQTT Heartbeat Task:**
   - `task_heartbeat`: Periodically (every 60s) publishes an "online" status and "ping" message to ensure the connection is alive.

6. **MQTT Command Manager:**
   - `task_cmd_manager`: Processes incoming commands from the `mqtt_cmd_queue`. It decodes JSON data, updates the target RGB values, and manages the LED override state.

7. **BLE Host Task:**
   - `ble_host_task`: Runs the Apache NimBLE Bluetooth stack continuously in the background. It handles all BLE events, manages active connections, and processes read/write requests to the GATT server characteristics.

8. **uart_rx_task:**
   - Runs continuously.
   - Listens for incoming JSON-formatted commands over the UART interface.
   - Parses the received payload to extract RGB color values and power states.
   - Updates the UART-specific LED override state within the main priority hierarchy.

9. **telemetry_task:**
   - Runs every 5 seconds.
   - Polls physical data from the connected I2C (AHT20, BMP280) and SPI (LSM6DS3) sensors.
   - Constructs a unified JSON string containing all current environmental and motion measurements.
   - Broadcasts the formatted telemetry simultaneously over UART, MQTT, and a custom BLE characteristic.

10. **LED Core Service (`led_service`):**
    - `led_service_task`: Acts as the central visual rendering engine.
    - Asynchronously consumes data from the joystick queues (analog position and button gestures).
    - Evaluates the strict hierarchical control priority (BLE/UART/MQTT overrides vs. Local Wi-Fi status vs. Manual Joystick control).
    - Computes the final RGB values and complex visual effects (e.g., HSV-to-RGB color circle mathematics) before pushing the state to the WS2812 hardware driver.

11. **Wi-Fi Orchestration Service (`wifi_service`):**
    - `wifi_service_task`: Manages network state transitions safely and asynchronously.
    - Listens to the joystick zone queue for physical gestures (Left = STA mode, Right = AP mode, Down = Wi-Fi OFF).
    - Securely starts, stops, or switches underlying Wi-Fi operations and updates the global Wi-Fi LED status without blocking hardware polling or other critical system tasks.

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

## MQTT Messaging & Remote Control
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

## Bluetooth Low Energy (BLE) Control
The system utilizes the Apache NimBLE stack to expose a GATT server for direct smartphone control without requiring an active Wi-Fi connection.
### Supported Standard Services
- Device Information Service (DIS): Exposes Manufacturer Name and Model Number.
- Battery Service (BAS): Simulates and reports the current battery level.
- Current Time Service (CTS): Reports the current synchronized RTC time to the connected client.

### Custom LED Control Service

The device exposes a custom service with readable User Descriptions (0x2901 descriptors) for seamless integration with mobile apps like LightBlue or nRF Connect.
- LED Power State: Allows overriding the LED state locally.
   - Payload: 01 (Enable BLE Override), 00 (Release control back to MQTT/Joystick).
- LED RGB Color: Direct HEX color injection.
   - Payload: 3 Bytes (e.g., FF0000 for Red, 00FF00 for Green).
- LED Hardware Init: Controls the physical initialization state of the RMT peripheral for power saving.
   - Payload: 01 (led2_init()), 00 (led2_deinit()).

## Hardware Bus & Sensor Topology

​The system implements a robust hardware layer using three distinct communication protocols to manage external peripherals and sensors:
- ​I2C Bus (Inter-Integrated Circuit): Configured as a master bus to communicate with environmental sensors. It supports multiple devices on the same data lines using unique hardware addresses.
- AHT20: Provides high-precision temperature and humidity readings.
- BMP280: Reads raw barometric pressure and temperature data.
- SPI Bus (Serial Peripheral Interface): Utilized for high-speed, full-duplex communication with motion sensors.
- LSM6DS3: A 6-axis inertial measurement unit (IMU) providing X, Y, and Z accelerometer data, selected via a dedicated Chip Select (CS) pin.
- UART Interface (Universal Asynchronous Receiver-Transmitter): Configured as an asynchronous serial bridge for direct, wired remote control and high-speed telemetry debugging

## Configuration (menuconfig)

```bash
idf.py menuconfig
```
### MAIN Configuration

#### WS2812 LED Configuration
- `LED_WS2812_GPIO` - GPIO number for the WS2812 data line.
- `LED_WS2812_MAX_LEDS` - Number of LEDs in the strip.
- `LED_WS2812_RMT_RESOLUTION` - RMT peripheral resolution in Hz.

#### Joystick Configuration
- `JOYSTICK_ADC_UNIT` - ADC Unit to use (1 or 2).
- `JOYSTICK_X_CHANNEL` / `JOYSTICK_Y_CHANNEL` - ADC channels for X and Y axes.
- `JOYSTICK_UPDATE_INTERVAL` - Read and update interval in milliseconds.
- `JOYSTICK_ENABLE_CIRCLE_MODE` - Enables RGB circle mode by default.

#### Button Configuration
- `BUTTON_GPIO` - GPIO number for the control button.
- `BUTTON_ACTIVE_LEVEL` - Active hardware level (0 = LOW, 1 = HIGH).
- `BUTTON_LONG_PRESS_TIME` - Duration to trigger a long press in ms.
- `BUTTON_SHORT_PRESS_TIME` - Max duration for a short press/click in ms.
- `JOYSTICK_ENABLE_WIFI_MODE` - Enable wifi mode

#### WIFI_AP Configuration
- `ESP_WIFI_SSID_AP` - WiFi SSID
- `ESP_WIFI_PASSWORD_AP` - WiFi Password
- `ESP_WIFI_CHANNEL` - WiFi Channel
- `ESP_MAX_STA_CONN` - Maximal STA connections
- `ESP_GTK_REKEYING_ENABLE` - Enable GTK Rekeying
- `ESP_GTK_REKEY_INTERVAL` - GTK rekey interval

#### WIFI_STA Configuration
- `ESP_WIFI_SSID_STA`  - WiFi SSID
- `ESP_WIFI_PASSWORD_STA` - WiFi Password
- `ESP_WIFI_SAE_MODE` - WPA3 SAE mode selectionWPA3 SAE mode selection
- `ESP_WIFI_PW_ID` - PASSWORD IDENTIFIER
- `ESP_MAXIMUM_RETRY` - Maximum retry
- `ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD` - WiFi Scan auth mode threshold

#### SNTP Configuration
- `SNTP_TIME_SERVER` - SNTP server name
- `SNTP_TIME_SYNC_METHOD` - Time synchronization method

#### MQTT Configuration
- `MQTT_BROKER_URI` - broker uri
- `MQTT_CLIENT_ID` - Unique identifier for the device on the broker.
- `MQTT_CMD_TOPIC` - Topic for receiving commands.
- `MQTT_STATUS_TOPIC` - Topic for publishing availability.

#### BLE Configuration
- `BLE_DEVICE_NAME` - The broadcast name visible during Bluetooth scanning.
- `BLE_MANUFACTURER_NAME` - Manufacturer name displayed in the DIS profile.
- `BLE_MODEL_NUMBER` - Device model identifier displayed in the DIS profile.

#### UART Configuration
- `UART_BAUD_RATE` - Data transmission speed in bits per second.
- `MY_TXD_PIN` - Transmit Data pin for sending outgoing serial communication.
- `MY_RXD_PIN` - Receive Data pin for reading incoming serial communication.

#### SPI Configuration
- `SPI_MOSI_PIN` - Master Out Slave In pin for transmitting data to the connected peripheral.
- `SPI_MISO_PIN` - Master In Slave Out pin for receiving data back from the connected peripheral.
- `SPI_SCLK_PIN` - Serial Clock pin for synchronizing the data transmission.


#### I2C configuration
- `I2C_MASTER_SDA_IO` - Serial Data Line pin for sending and receiving data payloads.
- `I2C_MASTER_SCL_IO` - Serial Clock Line pin for synchronizing the data transfer between devices.

#### **[LEGACY]** Custom LED Configuration
  - `MY_LED_ENABLE`          - Enable/disable LED logic (y/n).
  - `MY_LED_GPIO`            - GPIO number for the LED.
  - `MY_LED_ACTIVE_HIGH`     - Active high (1 = LED on when GPIO high) or active low (default: y).
  - `MY_BLINK_PERIOD_MS`     - Blink period in milliseconds (optional).

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
├── sdkconfig.defaults
├── main/
│   ├── main.c                  # Entry point. System initialization and service startup
│   └── Kconfig.projbuild      # Shortcut for kconfig
└── components/
    ├── app/                    # Application layer (business logic)
    │   ├── data_bridge/        # Data exchange between services (event routing / queues / mediator)
    │   ├── led_service/        # High-level LED logic (modes, states, effects)
    │   ├── telemetry_service/  # Telemetry aggregation and sensor data processing
    │   ├── sensor_service/     # Sensor polling, data acquisition, and hardware driver orchestration
    │   ├── ble_service/        # BLE initialization and GATT server management
    │   └── wifi_service/       # WiFi orchestration (manages STA/AP behavior)
    ├── drivers/                # Device drivers (hardware-specific logic)
    │   ├── aht20/              # AHT20 driver (temperature & humidity sensor)
    │   ├── bmp280/             # BMP280 driver (pressure & temperature sensor)
    │   ├── lsm6ds3/            # LSM6DS3 driver (accelerometer & gyroscope)
    │   └── ws2812/             # WS2812 addressable LED driver
    ├── hal/                    # Hardware Abstraction Layer (low-level wrappers)
    │   ├── i2c/                # I2C wrapper (init, read/write, mutex, configuration)
    │   ├── spi/                # SPI wrapper
    │   └── uart/               # UART wrapper
    ├── system/                 # Core device infrastructure (HAL, RTOS utilities, power/memory management)
    │   └── platform_init/      # System initialization (NVS setup, hardware & UI bootstrap) 
    ├── network/                # Networking layer
    │   ├── ble/                # NimBLE GATT server and custom BLE services
    │   ├── http_server/        # Embedded HTTP configuration server
    │   ├── mqtt_wrapper/       # MQTT client wrapper (abstraction over esp-mqtt)
    │   ├── sntp_sync/          # SNTP time synchronization
    │   ├── wifi_ap/            # WiFi Access Point implementation
    │   └── wifi_sta/           # WiFi Station implementation with reconnect logic
    ├── sys_utils/              # System utilities
    │   ├── json_parser/        # JSON parsing wrapper (cJSON abstraction)
    │   └── sys_monitor/        # System monitoring (heap, stack, CPU load, watchdog)
    ├── UI/                     # User Interface
    │  ├── joystick_button/     # GPIO interrupt handler and multi-gesture recognizer (click, double-click, hold)
    │  └── joystick_controller/ # ADC reader, software noise filter, and zone/position event dispatcher
    └── legacy/                 # LEGACY(NOT USED ANYMORE)
        └── led_ctrl/           # LEGACY(LED CONTROL DRIVER)
```
