# Application Layer

The Application layer contains high-level business logic and orchestrates all system services. It coordinates communication between hardware, network, and user-facing modules.

Currently the following services are implemented:
- **data_bridge** - Event routing, queues, and inter-service communication (currently a test/example).
- **led_service** - Manages LED logic, modes, and visual effects.
- **telemetry_service** - Collects, aggregates, and forwards sensor and system telemetry.
- **https_ota** - Handles secure firmware updates over HTTPS.
- **motor_service** - Controls stepper and servo motors, including coordinated movement.
- **sensor_service** - Polls sensors, gathers data, and interfaces with low-level drivers.
- **ble_service** - Initializes BLE, manages GATT server, and handles events.
- **wifi_service** - Manages Wi-Fi operation in STA/AP modes and maintains connectivity.

Each service is implemented as an independent component and communicates with other services through events, queues, or service APIs.

## data_bridge

The data_bridge service provides a simple inter-service communication mechanism using FreeRTOS queues. It acts as a test/example module for sending and receiving structured sensor data between tasks.
- **Initialization:** Creates a queue to hold DataPackage_t structures and starts sender and receiver tasks.
- **Sending:** The sender task periodically generates random temperature data and increments measurement IDs, then pushes them to the queue.
- **Receiving:** The receiver task waits for new data on the queue and processes it (currently prints to console).
- **Usage:** Other modules can use get_sensor_queue_handle() to access the queue and push or read data safely.

**Key functions:**
- `data_bridge_init()` - Initializes the queue and starts sender/receiver tasks.
- `get_sensor_queue_handle()` - Returns the handle to the sensor data queue for inter-task communication.

**Characteristics:**
- Queue-based communication ensures thread-safe data exchange.
- Independent tasks allow concurrent sending and receiving without blocking other system services.
- Extensible - Can be expanded to handle additional sensor types or forward data to other services like telemetry or MQTT.

## LED Service

The LED service provides high-level control over RGB LEDs (WS2812 strips). It handles user inputs from joystick buttons and position, manages Wi-Fi state indications, and executes remote or automatic color commands.

### Purpose
- Manage LED power and lock state.
- Toggle between RGB display modes (circle or zones).
- Visualize Wi-Fi connectivity state through LED colors/blinking.
- Receive remote LED commands via a queue.
- Convert joystick input into RGB output patterns.

### Initialization

- The service is initialized using:
```c
led_service_start();
```
- During initialization:
  - Creates `led_cmd_queue` to receive commands.
  - Starts `led_service_task` which continuously updates LED state based on joystick, remote commands, or Wi-Fi status.


### Event Handling

#### Joystick Button Events
- `BTN_EVT_LONG_PRESS` - toggles LED power on/off.
- `BTN_EVT_PRESS_DOWN` - locks/unlocks LED output.
- `BTN_EVT_DOUBLE_CLICK` - toggles RGB circle mode.
- `BTN_EVT_SINGLE_CLICK` - toggles joystick inversion for RGB mapping.

#### Joystick Position
- Maps joystick coordinates to RGB values.
- Supports both "circle" and "zone" display modes.
- Updates LED strip colors continuously in the service task.

#### Remote Commands
- Received via led_cmd_queue.
- Commands contain RGB values, mode (off/auto/remote), priority, and lock status.
- Higher-priority commands override lower-priority ones.
- LED is updated immediately according to the active remote command.

#### Wi-Fi State Indication
- Blinks or changes LED colors to reflect current Wi-Fi connectivity status:

`WIFI_LED_STA_CONNECTING`, `WIFI_LED_IP_RECEIVED`, `WIFI_LED_ONLINE`, etc.

- Toggles colors based on time for blinking effects.

### Service Task

`led_service_task` continuously executes:
- Checks and processes remote LED commands from queue.
- Reads joystick button events.
- Reads joystick position and maps to RGB.
- Updates LED strip using ws2812_set_rgb() or clears LEDs if powered off.
- Manages Wi-Fi state LED effects and RGB mode transitions.

### API
```c
void led_service_start(void);
QueueHandle_t led_service_get_queue(void);
void led_send_remote_command(led_remote_mode_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t priority);
bool led_service_is_circle_mode(void);
void led_service_toggle_mode(void);
void led_service_set_wifi_state(wifi_led_state_t state);
```
- `led_service_start` - initializes the service and starts its task.
- `led_service_get_queue` - returns the queue handle for sending commands to the LED service.
- `led_send_remote_command` - sends a remote LED command with RGB values, mode, and priority.
- `led_service_is_circle_mode` - returns true if RGB circle mode is active.
- `led_service_toggle_mode` - switches between circle and zone RGB modes.
- `led_service_set_wifi_state` - updates LED display according to Wi-Fi connection state.

### Characteristics
- Queue-based communication ensures thread-safe command reception.
- Joystick integration allows dynamic user control over LED output.
- Remote command support enables external modules or MQTT to control LEDs.
- Wi-Fi state visualization provides immediate network feedback via LED colors.
- Extensible and modular - supports adding new modes, LED types, or effects without altering core logic.

## WiFi Service

The WiFi service manages wireless connectivity for the device. It handles both Station (STA) and Access Point (AP) modes, integrates joystick input for mode selection, and communicates Wi-Fi status to the LED service for visual feedback.

### Purpose
- Control Wi-Fi operation in STA, AP, or OFF modes.
- Switch Wi-Fi modes dynamically based on joystick input.
- Notify LED service about current Wi-Fi state.
- Initialize and maintain SNTP synchronization for time-dependent services.

### Initialization

- The service is initialized using:
```c
wifi_service_start();
```
- During initialization:
  - Default STA and AP network interfaces are created.
  - Wi-Fi driver is initialized with default configuration.

- SNTP service is started for time synchronization.

- `wifi_service_task` is started, continuously monitoring joystick input to switch Wi-Fi modes.

### Event Handling

#### Joystick Zone Events
- `JOYSTICK_ZONE_LEFT` - activates STA mode.
- `JOYSTICK_ZONE_RIGHT` - activates AP mode.
- `JOYSTICK_ZONE_DOWN` - disables Wi-Fi.

#### Wi-Fi Mode Switching
- Stops ongoing STA reconnections if switching from STA.
- Stops Wi-Fi if current mode is not OFF.
- Starts the appropriate Wi-Fi mode (wifi_init_sta or wifi_init_softap) or disables Wi-Fi.
- Updates LED service to reflect current Wi-Fi state using led_service_set_wifi_state().

### Service Task
- `wifi_service_task` continuously executes:
- Reads `joystick zone events` from `joystick_get_zone_queue()`.
- Switches Wi-Fi mode according to `joystick zone`.
- If joystick queue is empty, waits 100 ms before checking again.

### API
```c
void wifi_service_start(void);
```

- `wifi_service_start` - initializes Wi-Fi interfaces, starts SNTP, and begins the service task.

### Characteristics
- **Dynamic mode switching** - allows STA, AP, or OFF modes to be activated at runtime.
- **Joystick integration** - user input directly controls Wi-Fi operation.
- **LED feedback** - communicates Wi-Fi status to LED service for visual indication.
- **Extensible** - supports addition of new Wi-Fi modes or integration with other services.
- **Non-blocking** - operates continuously in a FreeRTOS task without blocking other services.

## Sensor Service

The Sensor Service manages all hardware sensors connected via I2C and SPI. It provides a unified interface to initialize, read, and validate sensor data for higher-level services like Telemetry or Motor Control.

### Purpose
- Initialize available sensors according to project configuration.
- Abstract low-level driver calls to provide a consistent API.
- Validate sensor data before use.
- Provide thread-safe access to sensor readings.

### Initialization
- The service is initialized using:
```c
sensor_service_init();
```
- During initialization:
  - I2C and SPI buses are configured.
- Each enabled sensor is initialized:
  - AHT20 - temperature and humidity sensor.
  - BMP280 - temperature and pressure sensor.
  - LSM6DS3 - accelerometer sensor.
- Logs success or failure for each sensor.

### Reading Data

`sensor_service_read(sensor_data_t *data)` reads the latest sensor values:
- Checks each sensor's initialization status.
- Reads sensor measurements and updates valid flags.
- Supports partial readings if some sensors fail.
- Returns true on a successful read attempt.

### API
```c
void sensor_service_init(void);
bool sensor_service_read(sensor_data_t *data);
```

- `sensor_service_init` - sets up buses and sensors.
- `sensor_service_read` - fills sensor_data_t structure with current sensor values.

### Characteristics
- **Conditional compilation** - sensors can be enabled or disabled via project config.
- **Safe concurrency** - designed to be called from multiple tasks using external synchronization.
- **Extensible** - supports adding new sensors by extending the initialization and read logic.

## Telemetry Service

The Telemetry Service aggregates sensor data and pushes it to external interfaces such as MQTT, BLE, or UART. It provides both environmental and motion telemetry.

### Purpose
- Periodically read sensor data.
- Aggregate data into structured messages.
- Publish environmental telemetry: temperature, humidity, pressure.
- Publish motion telemetry: accelerometer readings.
- Synchronize BLE updates with the latest sensor values.
- Push data to MQTT topics if connected.

### Initialization
- The service is initialized using:
```c
telemetry_start();
```
- During initialization:
  - Two FreeRTOS tasks are created:
    - `telemetry_env_task` - handles environmental sensors.
    - `telemetry_motion_task` - handles accelerometer/motion sensors.
- Mutexes for sensor access (sensor_mutex) and shared device state (state_mutex) are created.
- Logs initialization status.

#### Environmental Telemetry

`telemetry_env_task` periodically:
- Reads AHT20 and BMP280 sensors under sensor_mutex.
- Updates shared device state under state_mutex.
- Sends telemetry over BLE using sync_and_send_ble().
- Publishes JSON-formatted environmental data to UART and MQTT (esp-lection/env topic).

#### Motion Telemetry

`telemetry_motion_task` periodically:
- Reads accelerometer (LSM6DS3) under sensor_mutex.
- Updates shared device state under state_mutex.
- Sends telemetry over BLE.
- Updates motor service with accelerometer Z-axis value (motor_service_push_accel_z).
- Publishes JSON-formatted motion data to UART and MQTT (esp-lection/motion topic).

### API
```c
void telemetry_start(void);
```
- `telemetry_start` - starts both telemetry tasks and initializes mutexes for safe shared access.

### Characteristics
- **Thread-safe** - protects shared state with FreeRTOS mutexes.
- **Flexible reporting** - environmental and motion telemetry are handled independently.
- **External interface integration** - supports BLE, MQTT, and UART outputs.
- **Periodic updates** - configurable update intervals via project configuration.

## Motor Service

The Motor Service coordinates stepper and servo motors, supporting remote commands, joystick control, and accelerometer-based control. It abstracts low-level motor driver details and provides a unified interface for higher-level applications.

### Purpose
- Control SG92R servo and 28BYJ-48 stepper motors.
- Accept commands from remote, joystick, or accelerometer input.
- Synchronize stepper and servo positions.
- Provide smooth and safe motor movements.

### Initialization
- The service is initialized using:
```c
motor_service_init();
```
- During initialization:
  - Command queue motor_cmd_queue is created.
  - Servo and stepper drivers are initialized.
- FreeRTOS task `motor_service_task` is started to process commands.

### Command Handling

Commands are sent via `motor_send_remote_command` or `motor_service_push_accel_z`:

**Remote Commands**
- `motor_mode_t mode` - defines control mode (REMOTE, JOYSTICK, ACCEL).
- `target_angle` - desired angle in degrees.
- `priority` - determines which command overrides lower-priority ones.

**Accelerometer Commands**
- Pushes latest Z-axis acceleration to adjust motor angle automatically.

### Task Operation

**motor_service_task** runs continuously:
- Reads commands from the queue and updates active command state.
- Computes target motor angle based on control mode:
  - **Remote** - use commanded angle.
  - **Joystick** - map joystick X position to angle.
  - **Accelerometer** - map Z-axis acceleration to angle.
- Converts target angle to stepper motor steps.
- Moves stepper one step at a time, updates servo position every 16 steps.
- Stops motor when target is reached.

### API
```c
void motor_service_init(void);
QueueHandle_t motor_service_get_queue(void);
void motor_send_remote_command(motor_mode_t mode, float angle_deg, uint8_t priority);
void motor_service_push_accel_z(float accel_z);
```
- `motor_service_init` - initializes stepper and servo, creates queue, starts motor task.
- `motor_service_get_queue` - returns handle to motor command queue.
- `motor_send_remote_command` - sends remote motor command with mode, angle, and priority.
- `motor_service_push_accel_z` - updates motor target angle based on accelerometer Z value.

### Characteristics
- **Thread-safe command queue** - ensures reliable handling of multiple input sources.
- **Stepper-servo synchronization** - keeps both motors coordinated.
- **Flexible control modes** - **remote**, **joystick**, or **accel**.
- **Extensible** - new motors or control modes can be added with minimal changes.

## BLE Service

The BLE Service provides a Bluetooth Low Energy interface for telemetry, control commands, and device interaction. It leverages NimBLE as the BLE stack.

### Purpose
- Enable BLE connectivity for external apps or devices.
- Expose telemetry data and control endpoints via GATT services.
- Manage BLE host and stack initialization.

### Initialization
- The service is started using:
```c
ble_start();
```
- During initialization:
  - NimBLE stack is initialized (`nimble_port_init`).
  - GATT server is configured (`ble_gatt_svr_init`).
  - Security and BLE stack settings are applied (`ble_setup_stack_and_security`).
  - FreeRTOS host task ble_host_task runs the BLE event loop.

### Task Operation

`ble_host_task` runs continuously:
- Executes `nimble_port_run()` to handle BLE events.
- Stops and cleans up BLE stack on deinitialization (`nimble_port_freertos_deinit`).

### Characteristics
- **Independent BLE task** - runs separately from application logic.
- **Extensible GATT services** - supports telemetry, motor, or LED control.
- **NimBLE-based** - lightweight and FreeRTOS-compatible.
- **Safe initialization and teardown** - ensures BLE stack is properly started and stopped.

## HTTPS OTA Service

The HTTPS OTA Service handles secure firmware updates over the network. It verifies firmware versions, downloads updates, and reboots the device when the update is successful.

### Purpose
- Download and apply new firmware over HTTPS.
- Verify that the new firmware version is higher than the current one.
- Provide safe update process with logging and rollback on failure.
- Prevent multiple concurrent OTA updates.

### Initialization 
- The service is invoked using:
```c
ota_service_start_update(const char *url);
```
- During initialization:
  - Checks if another OTA is already in progress.
  - Uses a default URL if none is provided (`CONFIG_FIRMWARE_UPGRADE_URL`).
  - Creates a FreeRTOS task ota_task to perform the OTA in the background.

### Task Operation

`ota_task` executes the OTA process:
- Configures HTTPS client with server certificate.
- Begins OTA session (`esp_https_ota_begin`).
- Reads running firmware version and compares with new firmware (`is_new_version_greater`).
- Aborts if the new version is not strictly higher.
- Downloads and writes firmware in chunks (`esp_https_ota_perform`).
- Verifies complete data receipt and finalizes update (`esp_https_ota_finish`).
- On success, delays 2 seconds and restarts the device.
- On failure, aborts and frees resources.

### API
```c
bool ota_service_start_update(const char *url);
```

- `ota_service_start_update` - starts OTA update from the specified URL. Returns false if OTA is already running or URL is invalid.

### Characteristics
- **Version safety** - prevents downgrades or re-flashing the same version.
- **Secure** - HTTPS with certificate verification.
- **Non-blocking** - runs in a dedicated FreeRTOS task.
- **Safe concurrency** - ensures only one OTA runs at a time.
- **Extensible** - can integrate with MQTT commands or remote triggers.

## Application Layer Summary 
- **Modular Design** - Each service is independent, encapsulated, and communicates through queues or APIs.
- **Thread-Safe** - Queues and mutexes ensure safe access across tasks (LED, telemetry, motor, sensors).
- **Extensible & Scalable** - Adding new sensors, motors, or LED effects is possible without changing existing logic.
- **Integrated Communication** - Telemetry, motor commands, LED control, Wi-Fi state, and OTA updates are coordinated across services.
- **Hardware Abstraction** - Low-level drivers are hidden behind services, simplifying high-level control.
- **Network Feedback** - Wi-Fi status and OTA progress can be visualized via LEDs or monitored remotely through BLE/MQTT.
- **Safety Measures** - OTA verifies versions, motor commands are prioritized, and sensors are validated before use.
- **Continuous Operation** - Each service runs in its own FreeRTOS task, ensuring smooth multitasking and responsiveness.
- **Unified API Layer** - Each service exposes a small, clear set of functions for other services to use, reducing coupling and complexity.