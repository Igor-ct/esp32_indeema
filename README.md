# esp32_indeema

Simple ESP-IDF training project

## What it does

- Event-Driven Architecture: Decouples hardware drivers from business logic using FreeRTOS Queues. Network modules and sensors no longer block or access hardware directly.
- Microservices:  Introduces dedicated service modules to isolate system functionality.
- System Information: Prints basic chip, flash, and heap information (based on `hello_world` example).
- FreeRTOS Queue Demonstration: Demonstrates FreeRTOS queues by sending and receiving simulated temperature sensor data.
- CPU Load Simulation: Simulates CPU load using two dedicated tasks pinned to different CPU cores.
- System Monitoring: Monitors and logs system status every 5 seconds, including free heap, active tasks, CPU usage time, and core affinity.
- WS2812 LED Control: Controls a WS2812 addressable RGB LED via the RMT peripheral.
- Joystick Input: Reads analog inputs from an XY joystick using the ADC to dynamically control WS2812 LED color and position.
- Button Gesture Control: Uses a physical button with multi-gesture recognition to control the system:
  - *Press down*: Toggles the WS2812 LED lock state.
  - *Single click*: Inverts the joystick X/Y axes.
  - *Double click*: Switches the joystick control mode (e.g., RGB circle mode).
  - *Long press*: Toggles the WS2812 LED power on/off.
- Wi-Fi Support: Full Wi-Fi functionality with AP and STA modes.
- Time Synchronization: Synchronizes system time using SNTP in STA mode.
- HTTP Configuration Interface: Hosts an HTTP server with a Wi-Fi configuration page.
- Wi-Fi Mode Switching: Allows Wi-Fi mode selection using the joystick (Left → AP, Right → STA).
- Wi-Fi Status Indication: Uses the WS2812 LED to indicate Wi-Fi connection status.
- MQTT Remote Control: Full integration with an MQTT broker for remote monitoring and LED control.
  - Supports RGB color setting and LED state toggling via JSON commands.
  - Implements an "Override" logic to prioritize remote commands over local joystick control.
  - Features a "Last Will and Testament" (LWT) to notify the system when the device goes offline unexpectedly.
-  Bluetooth Low Energy (BLE) Control: Hosts a custom GATT server using the NimBLE stack, allowing direct control of the WS2812 LED from a smartphone. Features standard profiles (Battery, Current Time) and a custom LED control service.
-  Control Priority System: Implements a hierarchical control mechanism that resolves command conflicts between multiple control interfaces by applying a defined priority order.
- Hardware Buses: Initializes dedicated I2C and SPI buses for external peripherals.
- Sensor Drivers: Reads environmental and motion data from AHT20, BMP280, and LSM6DS3 sensors.
- Telemetry Broadcast: Packages live sensor data into JSON and transmits it simultaneously via UART, MQTT, and BLE.
- UART Control: Accepts JSON commands over UART to control the WS2812 LED, fully integrated into the existing priority hierarchy.
- **[NEW]**Advanced Dual-Stream Telemetry: Polls environmental and motion sensors using separate high- and low-frequency tasks.
  - Sensor Polling: Reads environmental data from AHT20/BMP280 and motion data from LSM6DS3.
  - State Synchronization: Synchronizes shared sensor state using mutexes and broadcasts unified JSON telemetry over UART, MQTT, and BLE.
- **[NEW]**Hybrid Stepper & Servo Control: Provides synchronized control of stepper and servo motors.
  - Motor Control: Drives a 28BYJ-48 stepper motor and an SG92R servo motor.
  - Control Modes: Supports multiple control modes including Joystick (analog mapping), Accelerometer (IMU tilt control), and Remote commands.


## **[UPDATED]** Task Implementation Details

The project implements the following FreeRTOS tasks to demonstrate multitasking capabilities:
   
1. CPU Load Task (`task_cpu_load`):
   - Simulates CPU usage by performing arithmetic operations.
   - Load duration and execution period are configurable in code.

2. System Status Task (`sys_status`):
   - Runs every 5 seconds.
   - Lists all tasks with state (Blocked, Ready, Running), priority, and remaining stack size (High Water Mark).
   - Displays CPU usage percentage per task.

3. Joystick Update Task (`joystick_update_task`):
   - Periodically reads analog values from the XY joystick using the ADC.
   - Updates joystick queues for other services/tasks.

4. MQTT Heartbeat Task (`task_heartbeat`):
   - Runs every 60 seconds.
   - Publishes "online" status and a "ping" message to maintain MQTT connection.

5. MQTT Command Manager (`task_cmd_manager`):
   - Processes incoming JSON commands from the `mqtt_cmd_queue`.
   - Updates target RGB values and manages LED override states.

6. BLE Host Task (`ble_host_task`):
   - Runs the NimBLE Bluetooth stack continuously.
   - Handles BLE events, manages connections, and processes GATT read/write requests.

7. UART Receive Task (`uart_rx_task`):
   - Continuously listens for JSON-formatted commands over UART.
   - Parses RGB and power state values from incoming messages.
   - Updates UART-specific LED override state in the global control hierarchy.

8. Telemetry Task (`telemetry_task`):
   - Runs every 5 seconds.
   - Polls I2C (AHT20, BMP280) and SPI (LSM6DS3) sensors.
   - Constructs a unified JSON telemetry snapshot.
   - Broadcasts telemetry simultaneously via UART, MQTT, and BLE.

9. LED Core Service (`led_service_task`):
   - Central visual rendering engine for WS2812 RGB LED.
   - Consumes joystick input (analog positions and button gestures).
   - Applies hierarchical control priority (BLE/UART/MQTT overrides > Wi-Fi status > manual joystick).
   - Computes final RGB values and visual effects before updating the WS2812 hardware.

10. Wi-Fi Orchestration Service (`wifi_service_task`):
    - Manages Wi-Fi state transitions safely and asynchronously.
    - Listens to joystick gestures (Left = STA, Right = AP, Down = Wi-Fi OFF).
    - Starts, stops, or switches Wi-Fi modes and updates global Wi-Fi LED status without blocking other tasks.

11. Environmental Telemetry Task (`telemetry_env_task`):
    - Runs periodically based on configuration.
    - Polls AHT20 (temperature, humidity) and BMP280 (pressure) sensors.
    - Updates the global device state safely using a mutex.
    - Broadcasts JSON telemetry via UART, MQTT, and triggers BLE updates.

12. Motion Telemetry Task (`telemetry_motion_task`):
    - Executes at configured motion update intervals.
    - Reads high-frequency accelerometer data (X, Y, Z axes).
    - Pushes X-axis values to motor control for tilt-based interactions.
    - Publishes motion telemetry via UART and MQTT, maintaining state consistency with mutex protection.

13. **[NEW]** Motor Control Service (`motor_service_task`):
    - Multimodal Operation: Supports Remote, Joystick, and Accelerometer control modes.
    - Dynamic Input Processing:
      - Maps joystick ADC values to target motor angles.
      - Normalizes accelerometer X-axis data for tilt-based control.
    - Hybrid Hardware Synchronization: Drives 28BYJ-48 stepper and SG92R servo motors synchronously.
    - Smooth Motion Logic: Implements non-blocking step-by-step movement with integrated delays (5 ms) to ensure stability.
    - State Feedback: Synchronizes servo visual state with stepper physical position.

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

## **[UPDATED]** MQTT Messaging & Remote Control

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
 },
 "motor" : {
  "angle" : 180,
  "mode" : "remote"
 }

}
```

#### led:

- color: Sets the target RGB values for the WS2812 LED.
- state:
   - "on": Overrides the current Wi-Fi status LED and applies the MQTT, UART or BLE color.
   - "off": Sets the LED color to black and locks further changes until the state is switched to "on" by a higher-priority command.
   - "auto": Disables the override and returns the LED to showing the Wi-Fi connectivity status.

#### Motor:

- angle: Target servo angle in degrees.
- mode:
   - "remote": Motor is controlled via MQTT commands, UART or BLE.
   - "joystick": Motor follows the local analog joystick input.
   - "accel": Motor responds to tilt using accelerometer data.

### Status 

Status Topic: esp-lection/status

- Publishes online when connected.
- Publishes offline (LWT) if the device loses connection.
- Sends a ping message every minute to verify the link.

## Telemetry

The system continuously collects and broadcasts sensor data via UART, MQTT, and BLE.

### Environment

- Sensors:
   - AHT20: Temperature (t) and Humidity (h)
   - BMP280: Pressure (p)
- Frequency: Configured via Update_env_telemetry.
- Processing:
  - Sensor readings are written to the global device state under state_mutex for thread safety.
  - JSON packet format: ```{"temp":temperature,"hum":humidity,"press":pressure}```
- Transmission:
  - Sent over UART (send_data("ENV", json)).
  - Published to MQTT topic esp-lection/env if connected.
  - Synchronized to BLE clients via sync_and_send_ble().

### Motion

- Sensors:
   - LSM6DS3 accelerometer: X, Y, Z axes (accel).
- Frequency: Configured via Update_motion_telemetry.
- Processing:
   - Accelerometer data is written to global state under state_mutex.
   - JSON packet format: ```{"accel":[x,y,z]}```
- Integration:
   - X-axis values are pushed to motor_service_push_accel_x() for tilt-based motor control.
- Transmission:
  - Sent over UART (send_data("MOTION", json)).
  - Published to MQTT topic esp-lection/motion if connected.
  - Synchronized to BLE clients via ```sync_and_send_ble()```.

### BLE Synchronization

- The function sync_and_send_ble() combines ENV and MOTION data into a single JSON packet:```{"env":{"t":...,"h":...,"p":...},"acc":[x,y,z]}```
- Ensures BLE clients receive a consistent snapshot of all sensor data, even though ENV and MOTION are polled at different rates.

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

#### DRIVERS

##### WS2812 LED Configuration

- `LED_WS2812_GPIO` - GPIO number for the WS2812 data line.
- `LED_WS2812_MAX_LEDS` - Number of LEDs in the strip.
- `LED_WS2812_RMT_RESOLUTION` - RMT peripheral resolution in Hz.

##### SG92R Servo Motors Configuration

-  `SG92R_PWM_PIN` -  GPIO number used for the PWM signal to control the servo angle.

##### 28BYJ-48 Stepper Motor Configuration

-  `STEPPER_IN1_PIN` - GPIO number for the first phase (IN1) of the ULN2003 driver.
-  `STEPPER_IN2_PIN` - GPIO number for the second phase (IN2) of the ULN2003 driver.
-  `STEPPER_IN3_PIN` - GPIO number for the third phase (IN3) of the ULN2003 driver.
-  `STEPPER_IN4_PIN` - GPIO number for the fourth  phase (IN4) of the ULN2003 driver.

#### UI

##### Joystick Configuration

- `JOYSTICK_ADC_UNIT` - ADC Unit to use (1 or 2).
- `JOYSTICK_X_CHANNEL` / `JOYSTICK_Y_CHANNEL` - ADC channels for X and Y axes.
- `JOYSTICK_UPDATE_INTERVAL` - Read and update interval in milliseconds.
- `JOYSTICK_ENABLE_CIRCLE_MODE` - Enables RGB circle mode by default.

##### Button Configuration

- `BUTTON_GPIO` - GPIO number for the control button.
- `BUTTON_ACTIVE_LEVEL` - Active hardware level (0 = LOW, 1 = HIGH).
- `BUTTON_LONG_PRESS_TIME` - Duration to trigger a long press in ms.
- `BUTTON_SHORT_PRESS_TIME` - Max duration for a short press/click in ms.
- `JOYSTICK_ENABLE_WIFI_MODE` - Enable wifi mode.

#### NETWORK

##### WIFI_AP Configuration

- `ESP_WIFI_SSID_AP` - WiFi SSID.
- `ESP_WIFI_PASSWORD_AP` - WiFi Password.
- `ESP_WIFI_CHANNEL` - WiFi Channel.
- `ESP_MAX_STA_CONN` - Maximal STA connections.
- `ESP_GTK_REKEYING_ENABLE` - Enable GTK Rekeying.
- `ESP_GTK_REKEY_INTERVAL` - GTK rekey interval.

##### WIFI_STA Configuration

- `ESP_WIFI_SSID_STA`  - WiFi SSID.
- `ESP_WIFI_PASSWORD_STA` - WiFi Password.
- `ESP_WIFI_SAE_MODE` - WPA3 SAE mode selection.
- `ESP_WIFI_PW_ID` - Password identifier.
- `ESP_MAXIMUM_RETRY` - Maximum retry.
- `ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD` - WiFi Scan auth mode threshold.

##### SNTP Configuration

- `SNTP_TIME_SERVER` - SNTP server name.
- `SNTP_TIME_SYNC_METHOD` - Time synchronization method.

##### MQTT Configuration

- `MQTT_BROKER_URI` - Broker uri.
- `MQTT_CLIENT_ID` - Unique identifier for the device on the broker.
- `MQTT_CMD_TOPIC` - Topic for receiving commands.
- `MQTT_STATUS_TOPIC` - Topic for publishing availability.

##### BLE Configuration

- `BLE_DEVICE_NAME` - The broadcast name visible during Bluetooth scanning.
- `BLE_MANUFACTURER_NAME` - Manufacturer name displayed in the DIS profile.
- `BLE_MODEL_NUMBER` - Device model identifier displayed in the DIS profile.

#### HAL

##### UART Configuration

- `UART_BAUD_RATE` - Data transmission speed in bits per second.
- `MY_TXD_PIN` - Transmit Data pin for sending outgoing serial communication.
- `MY_RXD_PIN` - Receive Data pin for reading incoming serial communication.

##### SPI Configuration

- `SPI_MOSI_PIN` - Master Out Slave In pin for transmitting data to the connected peripheral.
- `SPI_MISO_PIN` - Master In Slave Out pin for receiving data back from the connected peripheral.
- `SPI_SCLK_PIN` - Serial Clock pin for synchronizing the data transmission.


##### I2C configuration

- `I2C_MASTER_SDA_IO` - Serial Data Line pin for sending and receiving data payloads.
- `I2C_MASTER_SCL_IO` - Serial Clock Line pin for synchronizing the data transfer between devices.

#### APP

##### Sensor service configuration

- `SENSOR_ENABLE_AHT20`  -  Enable AHT20 temperature and humidity sensor support.
- `SENSOR_ENABLE_BMP280` -  Enable BMP280 pressure sensor support.
- `SENSOR_ENABLE_LSM6DS3` - Enable LSM6DS3 IMU (accelerometer) support.

##### Telemetry service configuration

- `UPDATE_ENV_TELEMETRY` - Environment telemetry update interval in ms.
- `UPDATE_MOTION_TELEMETRY_TIME` - Motion telemetry update interval in ms.

#### LEGACY

##### **[LEGACY]** Custom LED Configuration

- `MY_LED_ENABLE`          - Enable/disable LED logic (y/n).
- `MY_LED_GPIO`            - GPIO number for the LED.
- `MY_LED_ACTIVE_HIGH`     - Active high (1 = LED on when GPIO high) or active low (default: y).
- `MY_BLINK_PERIOD_MS`     - Blink period in milliseconds (optional).

## Build & Flash

1. Build the project
- ```idf.py build```

2. Flash + monitor (real hardware)
- ```idf.py -p PORT flash monitor```
- Exit monitor: Ctrl + ]

## Project structure
```
esp32_indeema/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── main/
│   ├── main.c                  # Entry point. System initialization and service startup.
│   └── Kconfig.projbuild       # Shortcut for kconfig
└── components/
    ├── app/                    # Application layer (business logic)
    │   ├── data_bridge/        # Data exchange between services (event routing / queues / mediator)
    │   ├── led_service/        # High-level LED logic (modes, states, effects)
    │   ├── telemetry_service/  # Telemetry aggregation and sensor data processing
    │   ├── motor_service/      # Motor control logic (stepper & servo coordination and control modes)
    │   ├── sensor_service/     # Sensor polling, data acquisition, and hardware driver orchestration
    │   ├── ble_service/        # BLE initialization and GATT server management
    │   └── wifi_service/       # WiFi orchestration (manages STA/AP behavior)
    ├── drivers/                # Device drivers (hardware-specific logic)
    │   ├── aht20/              # AHT20 driver (temperature & humidity sensor)
    │   ├── bmp280/             # BMP280 driver (pressure & temperature sensor)
    │   ├── lsm6ds3/            # LSM6DS3 driver (accelerometer & gyroscope)
    │   ├── stepper_28byj48/    # 28BYJ-48 stepper motor driver (precise step control)
    │   ├── sg92r               # SG92R servo motor driver (PWM-based angle control)
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
