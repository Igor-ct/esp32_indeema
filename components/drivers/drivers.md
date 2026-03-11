# Drivers

This document describes the low-level hardware drivers used in the system. Drivers provide direct access to sensors and actuators, abstracting communication protocols and timing details for safe usage by higher-level components.

Currently the following hardware peripherals are supported:
- **LSM6DS3** - 3-Axis Accelerometer & Gyroscope
- **BMP280** - Temperature & Pressure Sensor
- **AHT20** - Temperature & Humidity Sensor
- **WS2812** - RGB LED Strip
- **SG92R** - Servo Motor
- **28BYJ-48** - Stepper Motor

Each peripheral is implemented as an independent driver and provides initialization, read/write, or control functions. Drivers ensure safe and reliable access to hardware without requiring higher-level components to manage low-level communication details.

## LSM6DS3 (3-Axis Accelerometer & Gyroscope)

The LSM6DS3 driver provides access to acceleration and angular velocity data over SPI.
- Initialization: Configures SPI device settings, chip select pin, and sensor registers.
- Reading: Reads raw data from the accelerometer and converts it to physical units.
- **Usage:** Higher-level components can obtain x, y, z acceleration values without dealing with SPI transactions or sensor timing.

**Key functions:**
- `lsm6ds3_init(cs_pin)` - Initializes the SPI interface and configures the LSM6DS3 sensor.
- `lsm6ds3_read_accel(&x, &y, &z)` - Reads current acceleration values in g for each axis.

## BMP280 (Temperature & Pressure Sensor)

The BMP280 driver handles temperature and pressure measurements over I2C.
- **Initialization:** Adds the device to the I2C bus, reads calibration registers, and configures measurement settings.
- **Reading:** Reads raw sensor data and applies compensation algorithms to obtain temperature in °C and pressure in Pa.
- **Usage:** Higher-level modules can obtain environmental measurements without performing I2C communication manually.

**Key functions:**
- `bmp280_init()` - Initializes the I2C device and reads calibration data.
- `bmp280_read(&temperature, &pressure)` - Returns current temperature and pressure values.

## AHT20 (Temperature & Humidity Sensor)

The AHT20 driver provides access to temperature and humidity over I2C.
- **Initialization:** Adds the AHT20 device to the I2C bus.
- **Reading:** Sends measurement commands, waits for conversion, and returns humidity (%) and temperature (°C).
- **Usage:** Higher-level components can retrieve environmental data without managing I2C commands or timing delays.

**Key functions:**
- `aht20_init()` - Adds the sensor to the I2C bus and prepares it for measurements.
- `aht20_read(&temperature, &humidity)` - Reads current temperature and humidity from the sensor.

## 28BYJ-48 (Stepper Motor)

The 28BYJ-48 driver provides stepper motor control using GPIO outputs.
- **Initialization:** Configures motor control pins as outputs and resets them.
- **Movement:** Supports stepping in both clockwise and counterclockwise directions with configurable step delay.
- **Stopping:** Stops the motor by resetting all pins.
- **Usage:** Higher-level components can move the motor by specifying steps, direction, and speed without handling pin sequences manually.

**Key functions:**
- `stepper_28byj48_init()` - Initializes GPIO pins and prepares the motor.
- `stepper_28byj48_move(steps, clockwise, step_delay_ms)` - Moves motor by specified steps and direction.
- `stepper_28byj48_stop()` - Stops the motor and clears outputs.

## SG92R (Servo Motor)

The SG92R driver provides PWM-based control for a servo motor.
- **Initialization:** Configures LEDC PWM timer and channel for servo control.
- **Angle control:** Converts angle to PWM duty and updates the servo.
- **Usage:** Higher-level components can set the servo angle without dealing with timer or duty calculations.

**Key functions:**
- `servo_motor_init()` - Initializes PWM timer and channel for servo control.
- `servo_motor_set_angle(angle)` - Moves servo to a specified angle in degrees.

## WS2812 (RGB LED Strip)

The WS2812 driver provides control over an addressable RGB LED strip using the RMT peripheral.
- **Initialization:** Configures RMT and LED strip parameters.
- **Color control:** Sets individual LED colors and refreshes the strip.
- **Clearing:** Turns off all LEDs safely.
- **Usage:** Higher-level components can display colors without handling timing-sensitive RMT signals.

**Key functions:**
- `ws2812_init()` - Initializes the RMT device and LED strip.
- `ws2812_set_rgb(r, g, b)` - Sets the RGB color of the first LED.
- `ws2812_clear()` - Turns off all LEDs.

## Driver Layer Characteristics
- **Independent drivers** - Each peripheral operates independently and does not block access to others.
- **Safe concurrency** - Provides handles, tasks, or PWM/RMT abstractions to safely interact with hardware.
- **Initialization safety** - Prevents multiple initializations and warns if attempted.
- **Abstraction** - Higher layers do not need to know low-level bus transactions, GPIO sequences, or PWM calculations.
- **Extensible configuration** - Pins, addresses, PWM settings, step delays, and RMT parameters are configurable via macros.
- **Hardware-specific timing handled internally** - Drivers manage delays, step sequences, and PWM duty calculations internally for safe usage.