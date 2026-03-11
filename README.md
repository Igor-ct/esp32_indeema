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

## DETAILS

For detailed documentation on each module, please refer to the following Markdown files in the components folder:
- Application Layer – [app.md](components/app/app.md)
- Drivers Overview – [drivers.md](components/drivers/drivers.md) 
- HAL (Hardware Abstraction Layer) – [hal.md](components/hal/hal.md)
- System Infrastructure – [system.md](components/system/system.md)
- Network Layer – [network.md](components/network/network.md)
- System Utilities – [sys_utils.md](components/sys_utils/sys_utils.md)
- User Interface – [ui.md](components/ui/ui.md)

This section serves as a central reference to navigate through the full project documentation, detailing service APIs, architecture, hardware drivers, and system utilities.

## Build & Flash

1. Configure the project (optional)

   You can modify project configuration parameters using the ESP-IDF configuration menu:

   ```idf.py menuconfig```

   For a full description of all available configuration parameters, see the [Configuration Guide](components/configuration.md).

   After changing configuration options, save and exit the menu to apply the new settings.


2. Build the project
 
   ```idf.py build```

   This command compiles the firmware and generates the binary files.

3. Flash and monitor (real hardware)

   ```idf.py -p PORT flash monitor```

   Replace PORT with the serial port connected to the ESP32 device.

   Example:

   ```idf.py -p /dev/ttyUSB0 flash monitor```

4. Exit the serial monitor
Ctrl + ]

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
    ├── configuration.md        # Project configuration parameters
    ├── app/                    # Application layer (business logic)
    │   ├── data_bridge/        # Data exchange between services (event routing / queues / mediator)//in future now its just a test example 
    │   ├── led_service/        # High-level LED logic (modes, states, effects)
    │   ├── telemetry_service/  # Telemetry aggregation and sensor data processing
    │   ├── https_ota/          # Secure firmware update service over HTTPS (OTA updates)
    │   ├── motor_service/      # Motor control logic (stepper & servo coordination and control modes)
    │   ├── sensor_service/     # Sensor polling, data acquisition, and hardware driver orchestration
    │   ├── ble_service/        # BLE initialization and GATT server management
    │   ├── wifi_service/       # WiFi orchestration (manages STA/AP behavior)
    │   └── app.md              # Application layer architecture
    ├── drivers/                # Device drivers (hardware-specific logic)
    │   ├── aht20/              # AHT20 driver (temperature & humidity sensor)
    │   ├── bmp280/             # BMP280 driver (pressure & temperature sensor)
    │   ├── lsm6ds3/            # LSM6DS3 driver (accelerometer & gyroscope)
    │   ├── stepper_28byj48/    # 28BYJ-48 stepper motor driver (precise step control)
    │   ├── sg92r/              # SG92R servo motor driver (PWM-based angle control)
    │   ├── ws2812/             # WS2812 addressable LED driver
    │   └── drivers.md          # Driver overview
    ├── hal/                    # Hardware Abstraction Layer (low-level wrappers)
    │   ├── i2c/                # I2C wrapper (init, read/write, mutex, configuration)
    │   ├── spi/                # SPI wrapper
    │   └── uart/               # UART wrapper
    │   └── hal.md              # HAL overview
    ├── system/                 # Core device infrastructure (HAL, RTOS utilities, power/memory management)
    │   ├── platform_init/      # System initialization (NVS setup, hardware & UI bootstrap)
    │   └── system.md           # System overview
    ├── network/                # Networking layer
    │   ├── ble/                # NimBLE GATT server and custom BLE services
    │   ├── http_server/        # Embedded HTTP configuration server
    │   ├── mqtt_wrapper/       # MQTT client wrapper (abstraction over esp-mqtt)
    │   ├── sntp_sync/          # SNTP time synchronization
    │   ├── wifi_ap/            # WiFi Access Point implementation
    │   ├── wifi_sta/           # WiFi Station implementation with reconnect logic
    │   ├── https_ota/          # OTA
    │   └── network.md          # Network overview
    ├── sys_utils/              # System utilities
    │   ├── json_parser/        # JSON parsing wrapper (cJSON abstraction)
    │   ├── sys_monitor/        # System monitoring (heap, stack, CPU load, watchdog)
    │   └── sys_utils.md        # System utilits overview
    ├── ui/                     # User Interface
    │  ├── joystick_button/     # GPIO interrupt handler and multi-gesture recognizer (click, double-click, hold)
    │  ├── joystick_controller/ # ADC reader, software noise filter, and zone/position event dispatcher
    │  └── ui.md                # User Interface overview
    └── legacy/                 # LEGACY(NOT USED ANYMORE)
        └── led_ctrl/           # LEGACY(LED CONTROL DRIVER)
```
