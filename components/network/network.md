# Network Layer
The Network layer provides communication interfaces that allow the device to interact with external systems.
It abstracts transport protocols and exposes services used by the application layer.

Currently the following protocols are implemented:
- Wi-Fi (STA/AP)
- HTTP Server
- MQTT
- SNTP
- Bluetooth Low Energy (BLE)

Each protocol is implemented as an independent component and communicates with the application layer through events, queues, or service APIs.

## Bluetooth Low Energy (BLE)
The BLE subsystem is implemented using the Apache NimBLE stack provided by ESP-IDF.

The device operates as a GATT server, exposing several services that allow a smartphone or other BLE client to:
- read system information
- receive telemetry updates
- control the RGB LED
- control motor position and mode

The BLE module also handles:
- advertising
- connection management
- telemetry notifications
- command forwarding to application services

### BLE Architecture

The BLE component performs the following responsibilities:
- Initialize the NimBLE host stack.
- Configure security and pairing parameters.
- Register GATT services and characteristics.
- Start BLE advertising.
- Process incoming read/write requests.
- Forward commands to application services.

**Typical communication flow:**
```
BLE Client
     ↓
GATT Characteristic Write
     ↓
BLE Handler
     ↓
Application Service (LED / Motor / Telemetry)
```

### Advertising
The device starts advertising automatically after the BLE stack is synchronized.

#### Advertising configuration

| Parameter       | Value                             |
|-----------------|-----------------------------------|
| Device Name     | defined by CONFIG_BLE_DEVICE_NAME |
| Discovery Mode  | General discoverable              |
| Connection Mode | Undirected connectable            |

**Advertising payload contains:**
- device name
- general discovery flags

Advertising restarts automatically after a disconnection.

### BLE Security

Security configuration is defined during stack initialization.

| Parameter          | Value           |
|--------------------|-----------------|
| Bonding            | Enabled         |
| MITM protection	 | Disabled        |
| Secure Connections | Enabled         |
| IO Capability      | No input/output |

This configuration allows secure pairing while keeping the connection flow simple for mobile applications.

### GATT Services Overview

The BLE server exposes several standard and custom services.

#### Standard Services

##### Device Information Service (DIS)

Provides device metadata.

**Characteristic**
| Characteristic	    | Description                                   |
|-----------------------|-----------------------------------------------|
| Manufacturer Name	    | Value defined by CONFIG_BLE_MANUFACTURER_NAME |
| Model Number	Value   | defined by CONFIG_BLE_MODEL_NUMBER            |

##### Battery Service (BAS) 

Provides simulated battery level information.

**Characteristic**

|Characteristic | Description                                 |
|---------------|---------------------------------------------|
| Battery Level | 8-bit value representing battery percentage |

**Supported operations:**
- Read
- Notify

##### Current Time Service (CTS)

Provides synchronized device time.

**Characteristic**
| Characteristic | Description                                |
|----------------|--------------------------------------------|
| Current Time   | Encoded timestamp containing date and time |

**Supported operations:**
- Read
- Notify

The timestamp is generated from the system RTC, which may be synchronized via SNTP.

#### Custom Services

##### LED Control Service

This service allows remote control of the WS2812 LED.

**Characteristic**

Controls the LED operating mode.

| Value |	Mode   |
|-------|----------|
| 0	    | LED OFF  |
| 1	    | LED ON   |
| 2	    | LED AUTO | 

When a value is written, the command is forwarded to the LED service:

```
led_send_remote_command(...)
```

The BLE interface participates in the control priority system used by the firmware.

##### LED RGB Color

Sets the LED color.

**Payload format:**

```
3 bytes: [R, G, B]
```

**Examples:**

```
FF 00 00 → Red
00 FF 00 → Green
00 00 FF → Blue
```

If the LED is currently enabled, the new color is immediately applied.

##### Telemetry Service

This service provides live sensor telemetry to connected BLE clients.

**Characteristic**
| Characteristic | Description                      |
|----------------|----------------------------------|
| Telemetry JSON | Combined sensor telemetry packet |

**Supported operations:**
- Read
- Notify

**Default telemetry format:** See [Telemetry Format](../components/app/app.md)

The application layer periodically updates the telemetry buffer using:

```ble_update_telemetry(json_data)```

If a client is connected, the BLE module sends a notification automatically.

##### Motor Control Service

This custom service allows remote control of the motor subsystem.

**Characteristic**
| Characteristic | Description              |
|----------------|--------------------------|
| Motor Command  | Raw motor control packet |

**Supported operations:**
- Write

**Payload format**
```
Byte 0    → Motor mode
Bytes 1–4 → Target angle (float)
```

**Example:**
```
[mode][angle]
```

After receiving a command, the BLE module forwards it to the motor service:

```motor_send_remote_command(mode, angle, priority)```

This integrates BLE commands into the system's control priority hierarchy.

##### OTA Update Service (BLE)

This custom BLE service allows initiating an OTA firmware update by writing a URL string to a dedicated characteristic.

**Characteristic:**

| Characteristic | Description                    | 
|----------------|--------------------------------|
| OTA URL        | URL of the firmware to update  |            

**Supported Operations:**
- Write

**Usage:**

- Write the full OTA URL (max 255 bytes) to the OTA characteristic.
- The BLE handler validates the URL length and forwards it to the OTA service.
- If valid, `ota_service_start_update(url)` is called to start the update process.

**Example:**

```
https://example.com/firmware.bin
```

This characteristic integrates OTA initiation into the BLE control flow alongside other application services.

### Telemetry Notifications

The BLE module supports real-time telemetry notifications.

When telemetry data is updated:
1. JSON telemetry is written into an internal buffer.
2. If a BLE client is connected, a notification is sent.
3. The client receives updated sensor data immediately.

This mechanism allows low-latency monitoring without polling.

### BLE Connection Handling

The GAP event handler manages connection events.

**Characteristic**
| Event	     | Description                                                  |
|------------|--------------------------------------------------------------|
| Connect    | Stores connection handle and enables telemetry notifications |
| Disconnect | Clears connection state and restarts advertising             |

This ensures the device automatically becomes discoverable after a disconnection.

### Integration With Application Layer

The BLE module communicates with the application services using function calls.

| Service	        | Function                    |
|-------------------|-----------------------------|
| LED Service	    | led_send_remote_command()   |
| Motor Service     | motor_send_remote_command() |
| Telemetry Service | ble_update_telemetry()      |

This design keeps the network layer responsible only for communication, while business logic remains in the application layer.