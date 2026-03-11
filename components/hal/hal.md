# HAL Layer

The Hardware Abstraction Layer (HAL) provides unified access to physical hardware peripherals. It abstracts low-level driver details and exposes standard interfaces for the application and network layers.

Currently the following hardware interfaces are implemented:
- **I2C** - master bus initialization and handle access
- **SPI** - master bus initialization and host access
- **UART** - serial communication initialization, sending, and receiving

Each interface is implemented as an independent component and provides handles, queues, or tasks to safely interact with hardware. The HAL layer ensures reliable peripheral access, allowing higher-level components to operate without dealing with low-level driver intricacies.

## I2C (Inter-Integrated Circuit)

The I2C component provides access to I2C-compatible peripherals.
- **Initialization:** Configures SDA and SCL pins, clock source, and optional internal pull-ups.
- **Handle access:** Exposes a single bus handle `(i2c_master_bus_handle_t)` that can be used by other modules to communicate with connected devices.
- **Usage:** Higher-level components interact with sensors or actuators over I2C without needing to manage low-level bus settings.

**Key functions:**
- `i2c_bus_init()` - Initializes the I2C master bus.
- `i2c_bus_get_handle()` - Returns the I2C bus handle for peripheral communication.

## SPI (Serial Peripheral Interface)

The SPI component manages SPI communication with external devices.
- **Initialization:** Configures MOSI, MISO, SCLK pins and optional DMA channels.
- **Host access:** Provides the SPI host device (spi_host_device_t) for transactions.
- **Usage:** Other modules can send and receive data over SPI without handling low-level configuration.

**Key functions:**
- `spi_bus_init()` - Initializes the SPI bus.
- `spi_bus_get_host()` - Returns the SPI host device for SPI transactions.

## UART (Universal Asynchronous Receiver-Transmitter)

The UART component enables serial communication with other devices.
- **Initialization:** Sets up TX/RX pins, baud rate, data bits, parity, stop bits, and flow control.
- **Sending:** Allows higher-level components to transmit strings or binary data.
- **Receiving:** Uses a dedicated FreeRTOS task for reading incoming data, parsing commands, and forwarding them through queues.
- **Usage:** Higher-level modules communicate with external controllers, microcontrollers, or peripherals without directly handling low-level UART operations.

**Key functions:**
- `uart_component_init()` - Configures UART peripheral and starts the RX task.
- `send_data(logName, data)` - Sends a string or command over UART.
- `rx_task()` - FreeRTOS task that continuously reads incoming UART data and processes complete messages.

## HAL Layer Characteristics

- **Independent components** - Each interface operates separately without blocking other hardware access.
- **Safe concurrency** - Provides handles, queues, or tasks to ensure thread-safe peripheral interaction.
- **Initialization safety** - Warns or prevents multiple initializations of the same hardware interface.
- **Extensible configuration** - Pin assignments, baud rates, and other parameters can be configured via macros or project settings.
- **Abstraction** - Higher layers can operate without detailed knowledge of low-level drivers, improving modularity and maintainability.