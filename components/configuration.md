# Configuration
This document describes the main compile-time configuration parameters used in the project.
Most of these options are defined through Kconfig and can be modified using the ESP-IDF configuration menu:

```idf.py menuconfig```

The configuration parameters control hardware setup, network behavior, peripheral drivers, and application services.
The settings are grouped into the following categories:
- Drivers - configuration of hardware drivers (LEDs, motors, sensors).
- UI - configuration of user input devices (joystick and button).
- Network - WiFi, MQTT, BLE, and time synchronization settings.
- HAL - low-level communication interfaces (UART, SPI, I2C).
- Application - configuration of application-level services.
- Legacy - deprecated configuration options kept for backward compatibility.


## MAIN Configuration

### DRIVERS

#### WS2812 LED Configuration

- `LED_WS2812_GPIO` - GPIO number for the WS2812 data line.
- `LED_WS2812_MAX_LEDS` - Number of LEDs in the strip.
- `LED_WS2812_RMT_RESOLUTION` - RMT peripheral resolution in Hz.

#### SG92R Servo Motors Configuration

-  `SG92R_PWM_PIN` -  GPIO number used for the PWM signal to control the servo angle.

#### 28BYJ-48 Stepper Motor Configuration

-  `STEPPER_IN1_PIN` - GPIO number for the first phase (IN1) of the ULN2003 driver.
-  `STEPPER_IN2_PIN` - GPIO number for the second phase (IN2) of the ULN2003 driver.
-  `STEPPER_IN3_PIN` - GPIO number for the third phase (IN3) of the ULN2003 driver.
-  `STEPPER_IN4_PIN` - GPIO number for the fourth  phase (IN4) of the ULN2003 driver.

### UI

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
- `JOYSTICK_ENABLE_WIFI_MODE` - Enable wifi mode.

### NETWORK

#### WIFI_AP Configuration

- `ESP_WIFI_SSID_AP` - WiFi SSID.
- `ESP_WIFI_PASSWORD_AP` - WiFi Password.
- `ESP_WIFI_CHANNEL` - WiFi Channel.
- `ESP_MAX_STA_CONN` - Maximal STA connections.
- `ESP_GTK_REKEYING_ENABLE` - Enable GTK Rekeying.
- `ESP_GTK_REKEY_INTERVAL` - GTK rekey interval.


#### WIFI_STA Configuration

- `ESP_WIFI_SSID_STA`  - WiFi SSID.
- `ESP_WIFI_PASSWORD_STA` - WiFi Password.
- `ESP_WIFI_SAE_MODE` - WPA3 SAE mode selection.
- `ESP_WIFI_PW_ID` - Password identifier.
- `ESP_MAXIMUM_RETRY` - Maximum retry.
- `ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD` - WiFi Scan auth mode threshold.

#### SNTP Configuration

- `SNTP_TIME_SERVER` - SNTP server name.
- `SNTP_TIME_SYNC_METHOD` - Time synchronization method.

#### MQTT Configuration

- `MQTT_BROKER_URI` - Broker uri.
- `MQTT_CLIENT_ID` - Unique identifier for the device on the broker.
- `MQTT_CMD_TOPIC` - Topic for receiving commands.
- `MQTT_STATUS_TOPIC` - Topic for publishing availability.

#### BLE Configuration

- `BLE_DEVICE_NAME` - The broadcast name visible during Bluetooth scanning.
- `BLE_MANUFACTURER_NAME` - Manufacturer name displayed in the DIS profile.
- `BLE_MODEL_NUMBER` - Device model identifier displayed in the DIS profile.

### HAL

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

### APP

#### Sensor service configuration

- `SENSOR_ENABLE_AHT20`  -  Enable AHT20 temperature and humidity sensor support.
- `SENSOR_ENABLE_BMP280` -  Enable BMP280 pressure sensor support.
- `SENSOR_ENABLE_LSM6DS3` - Enable LSM6DS3 IMU (accelerometer) support.

#### Telemetry service configuration

- `UPDATE_ENV_TELEMETRY` - Environment telemetry update interval in ms.
- `UPDATE_MOTION_TELEMETRY_TIME` - Motion telemetry update interval in ms.

### LEGACY

#### **[LEGACY]** Custom LED Configuration

- `MY_LED_ENABLE`          - Enable/disable LED logic (y/n).
- `MY_LED_GPIO`            - GPIO number for the LED.
- `MY_LED_ACTIVE_HIGH`     - Active high (1 = LED on when GPIO high) or active low (default: y).
- `MY_BLINK_PERIOD_MS`     - Blink period in milliseconds (optional).