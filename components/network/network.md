# Network Layer
The Network layer provides communication interfaces that allow the device to interact with external systems.
It abstracts transport protocols and exposes services used by the application layer.

Currently the following protocols are implemented:
- **Wi-Fi (STA/AP)**
- **HTTP Server**
- **MQTT**
- **SNTP**
- **Bluetooth Low Energy (BLE)**

Each protocol is implemented as an independent component and communicates with the application layer through events, queues, or service APIs.

## Wi-Fi Service

The Wi-Fi service provides network connectivity for the device and supports two operating modes:

Station mode (STA) - connects the device to an existing Wi-Fi network.

Access Point mode (AP) - creates a Wi-Fi network that other devices can connect to.

The service is responsible for initializing the Wi-Fi driver, handling connection events, managing reconnection logic, and starting network-dependent services once connectivity is available.

### Station Mode (STA)

Station mode allows the device to connect to an external Wi-Fi access point.

#### Initialization

Station mode is initialized using:
```c
wifi_init_sta()
```
During initialization the service:
- initializes the Wi-Fi driver if it is not already started
- registers Wi-Fi and IP event handlers
- loads Wi-Fi credentials from NVS storage if they were previously saved
- falls back to default credentials from configuration if no saved credentials exist
- configures the Wi-Fi interface in STA mode
- starts the connection process

LED states are updated during the process to reflect connection status.

#### Connection Handling

The Wi-Fi event handler manages connection state changes:

**On start**

`WIFI_EVENT_STA_START`

The device attempts to connect to the configured access point.

**On disconnection**

`WIFI_EVENT_STA_DISCONNECTED`

The service:
- stops the MQTT client to free resources
- attempts reconnection up to a configured retry limit
- updates LED status to indicate connection loss or failure

Reconnection can be disabled using:
```c
wifi_sta_stop_reconnect()
```

**Successful Connection**

When an IP address is received:

`IP_EVENT_STA_GOT_IP`

The service:
- logs the assigned IP address
- resets the retry counter
- updates LED status
- starts network dependent services

This includes:
- starting SNTP time synchronization
- starting the HTTP server

After a successful connection the MQTT client is started or restarted if it was previously initialized.

### Access Point Mode (AP)

Access Point mode allows the device to create its own Wi-Fi network.

#### Initialization

AP mode is started using:
```c
wifi_init_softap()
```
During initialization the service:
- initializes the Wi-Fi driver if necessary
- registers Wi-Fi event handlers
- configures SSID, password, channel, and connection limits
- enables WPA2 or WPA3 authentication depending on configuration
- switches the Wi-Fi driver to AP mode
- starts the Wi-Fi interface

#### Client Connection Events

The service handles client connection events:

**Client connected**

`WIFI_EVENT_AP_STACONNECTED`

The system logs the device MAC address and updates the LED state.

**Client disconnected**

`WIFI_EVENT_AP_STADISCONNECTED`

The disconnection reason is logged and the LED state is updated.

#### Web Server

When the AP mode starts successfully, the system launches the HTTP web server if it is not already running.
This allows clients connected to the access point to interact with the device through the web interface.

## HTTP Server

The device includes a lightweight HTTP server used for basic device configuration through a web browser.

The server exposes a small web interface consisting of an HTML page and a CSS stylesheet embedded directly into the firmware binary. These files are served to clients when accessing the root endpoint.

**Endpoints**
| Endpoint   | Method | Description                                            |
|------------|--------|--------------------------------------------------------|
| /		   | GET    | Serves the main  HTML configuration page               |
| /index.css |	GET	  | Serves the CSS stylesheet used by the web interface    |
| /save      |	POST	  | Receives Wi-Fi credentials submitted from the web form |

### Wi-Fi Configuration

The configuration page allows the user to submit Wi-Fi SSID and password through a form.

When the form is submitted:
- The HTTP server receives the POST request.
- URL-encoded parameters are decoded.
- The credentials are stored in NVS persistent storage.
- The device sends a confirmation response.
- The device performs a software reboot to apply the new configuration.

**Stored parameters:**
- `ssid`
- `pass`

These values are later used by the Wi-Fi subsystem during STA initialization.

### Static Content

The web interface files are compiled into the firmware binary and accessed using linker symbols:
- `index_html`
- `index.css`

This avoids the need for a filesystem and keeps the HTTP interface lightweight.

### Lifecycle

The web server is started by the network subsystem using:

`start_webserver()`

and can be stopped using:

`stop_webserver()`

The server typically runs when:
- the device operates in AP configuration mode
- the device has network connectivity in STA mode

## SNTP Time Synchronization

This component handles system time synchronization using SNTP.

### Initialization

The SNTP service is configured and initialized by:
```c
sntp_service_init()
```
**During initialization:**
- the SNTP configuration structure is created using the configured time server
- a synchronization callback is registered
- the SNTP service is initialized

Optional **smooth time synchronization** can be enabled through configuration.

### Network Event Handling

The component listens for network events using:
```c
sntp_net_event_handler(...)
```
When the device receives an IP address (IP_EVENT_STA_GOT_IP):
- a log message is printed
- the SNTP synchronization process is started

This ensures that time synchronization begins only after network connectivity is available.

### Time Synchronization Callback

When the system time is successfully synchronized, the following callback is executed:
```c
time_sync_notification_cb(...)
```
The callback performs the following steps:
- Logs that time synchronization has completed.
- Reads the current system time.
- Applies the configured timezone.
- Converts the timestamp to local time.
- Prints the current local time to the system log.
- The timezone used by the firmware is configured as:

   `EET-2EEST,M3.5.0/3,M10.5.0/4`

This corresponds to Eastern European Time with daylight saving adjustments.

## MQTT Service

The MQTT service provides a mechanism for the device to exchange messages with a broker. It handles commands for LEDs, motor control, and OTA updates, and manages the connection state.

### Purpose

Connect to the configured MQTT broker.
- Subscribe to a command topic.
- Publish status updates.
- Parse incoming commands and forward them to the relevant services.
- Manage connection state and retries.

### Initialization

The service is initialized using:
```c
mqtt_app_start();
```

During initialization:
- MQTT client is created and configured with broker URI and client ID.
- Last-will message is set for offline detection.
- A command queue is created to store incoming commands.
- Event handler for MQTT events is registered.
- Two tasks are started:
  - task_heartbeat - periodically publishes a heartbeat message.
  - task_cmd_manager - processes commands from the queue.

### Event Handling
MQTT Connection Events
- Connected: Subscribes to the command topic and publishes an "online" status.
- Disconnected: Updates connection status.

**Command Reception**

When a message arrives on the command topic:
- It is parsed as JSON.
- Commands are categorized as:
  - LED commands - color, state (on/off/auto)
  - Motor commands - angle and mode
  - OTA commands - firmware update URL

Parsed commands are pushed to mqtt_cmd_queue for processing.

### Command Processing

`task_cmd_manager` handles commands:

**LED Commands**
- Sets RGB target values.
- Sends LED commands to the LED service.
- Publishes LED status updates on the status topic.

**Motor Commands**
- Updates target angle.
- Sends motor commands with specified mode or angle.
- Publishes motor status updates on the status topic.

**OTA Commands**
- Starts OTA update if a valid URL is provided.
- Publishes OTA status updates on the status topic.

### Heartbeat Task

`task_heartbeat` periodically sends a status message "online" to the status topic every 60 seconds if the client is connected.

### API
```c
esp_mqtt_client_handle_t get_mqtt_client_handle(void);
bool get_mqtt_connected(void);
void set_mqtt_connected(bool status);
esp_err_t mqtt_publish_message(const char *topic, const char *payload);
```
- `get_mqtt_client_handle` - returns the MQTT client handle.
- `get_mqtt_connected` - returns the current connection state.
- `set_mqtt_connected` - manually sets the connection state.
- `mqtt_publish_message` - publishes a message to the specified topic if connected.

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

**Default telemetry format:** See 

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
Bytes 1-4 → Target angle (float)
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