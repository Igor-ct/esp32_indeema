## Hardware Bus & Sensor Topology

​The system implements a robust hardware layer using three distinct communication protocols to manage external peripherals and sensors:
- ​I2C Bus (Inter-Integrated Circuit): Configured as a master bus to communicate with environmental sensors. It supports multiple devices on the same data lines using unique hardware addresses.
- AHT20: Provides high-precision temperature and humidity readings.
- BMP280: Reads raw barometric pressure and temperature data.
- SPI Bus (Serial Peripheral Interface): Utilized for high-speed, full-duplex communication with motion sensors.
- LSM6DS3: A 6-axis inertial measurement unit (IMU) providing X, Y, and Z accelerometer data, selected via a dedicated Chip Select (CS) pin.
- UART Interface (Universal Asynchronous Receiver-Transmitter): Configured as an asynchronous serial bridge for direct, wired remote control and high-speed telemetry debugging