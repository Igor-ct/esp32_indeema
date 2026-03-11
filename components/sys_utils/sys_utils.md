# System Utilities

This module provides low-level system monitoring, JSON command parsing, and task utilities for the firmware.

---

## JSON Command Parsing

The `sys_utils` layer exposes functions to parse JSON commands received from network interfaces (BLE, MQTT, UART).

### LED Commands

```c
esp_err_t json_parse_led_command(const char *json_string, parsed_led_cmd_t *out_cmd);
```

**Purpose:** Parses an incoming LED command JSON object.

**Structure:**
```Json
{
  "led": {
    "state": "on|off|auto",
    "color": { "r": 255, "g": 0, "b": 0 }
  }
}
```

**Parsed Fields:**
- state → JSON_LED_STATE_ON | JSON_LED_STATE_OFF | JSON_LED_STATE_AUTO
- color → Optional RGB values

**Output:** Populates parsed_led_cmd_t structure.

### Motor Commands
esp_err_t json_parse_motor_command(const char *json_string, parsed_motor_cmd_t *out_cmd);

**Purpose:** Parses motor control JSON commands.

**Structure:**
```Json
{
  "motor": {
    "mode": "remote|joystick|accel",
    "angle": 45.0
  }
}
```

**Parsed Fields:**
- mode → mapped to integer mode codes
- angle → float value for target position

**Output:** Populates parsed_motor_cmd_t.

### OTA Commands

```c
esp_err_t json_parse_ota_command(const char *json_string, parsed_ota_cmd_t *out_cmd);
```

**Purpose:** Parses OTA update commands from JSON.

**Structure:**
```Json
{
  "ota": "https://example.com/firmware.bin"
}
```

**Parsed Fields:** URL string

**Output:** Populates parsed_ota_cmd_t.

## System Monitoring
Provides runtime monitoring and debugging tasks.

### Chip Information:
```c
void sys_monitor_print_chip_info(void);
```

**Prints:**
- Chip type and features (Wi-Fi, BT, BLE, Zigbee/Thread)
- Silicon revision
- Flash size (embedded/external)
- Minimum free heap

**Example Output:**

This is esp32s3 chip with 2 CPU core(s), WiFi/BLE, silicon revision v0.2, 16MB external flash
Minimum free heap size: 282484 bytes

### CPU Load Simulation Task
```c
void task_cpu_load(void *pvParameters);
```
- Simulates CPU load for testing.
- Parameters: load_cfg_t
   - name → Task name
   - period_ms → Loop period
   - busy_ms → Busy-wait duration
- Prints load statistics per task:

```[LOAD] Task1 core=0 busy=50ms period=100ms```

### System Status Task
```c
void task_system_status(void *pvParameters);
```
- Periodically prints runtime system status every 5s:
   - Uptime
   - Free heap
   - Task list with stack high-water marks
   - CPU usage per task
- Helps monitor health and resource usage of the firmware.

**Example Output:**
```
================ SYSTEM STATUS ================
Uptime: 1234567 ms
Free heap: 234567 bytes

Task          State  Prio  StackHW  Num
-------------------------------------------
IDLE          R      0     123      0
...

CPU usage per task:
Task                AbsTime   %Time
-----------------------------------
task_system_status  123456    15%
...
================================================
```


## Data Structures

### LED Command
```c
typedef struct {
    bool has_color;      
    uint8_t r, g, b;
    json_led_state_t state;
} parsed_led_cmd_t;
```

### Motor Command

```c
typedef struct {
    float angle;
    int mode;
    bool has_angle;
    bool has_mode;
} parsed_motor_cmd_t;
```

### OTA Command

```c
typedef struct {
    bool has_url;
    char url[256];
} parsed_ota_cmd_t;
```

## Summary

The **sys_utils** module centralizes:
- Parsing of JSON commands for LED, motor, and OTA.
- Runtime system monitoring and logging.
- CPU load simulation for testing and validation.
- It allows the application and network layers to operate independently while providing safe, structured data handling.