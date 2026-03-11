# UI Layer

The UI layer handles all user input and interaction mechanisms. It abstracts the physical input devices and exposes events and queues to the application layer.

Currently the following input components are implemented:
- **Joystick**
- **Physical Buttons**

Each component runs as an independent task and communicates with the application layer through FreeRTOS queues. The UI layer provides normalized input data, discrete events, and state changes for seamless integration with LEDs, motors, and other services.

## Joystick

### Initialization
```c
esp_err_t joystick_init(void);
```

- Creates two FreeRTOS queues:
  - `joystick_pos_queue` - latest position (x, y) normalized [0.0, 1.0].
  - `joystick_zone_queue` - discrete zones (left, right, down, center).
- Configures ADC channels for X and Y axes.
- Starts joystick_task to continuously read values.

### Features
- Position Tracking
  - Normalized X/Y coordinates sent to `joystick_pos_queue`.
- Zone Detection
  - Detects movement zones based on thresholds:
    - `JOYSTICK_ZONE_LEFT`
    - `JOYSTICK_ZONE_RIGHT`
    - `JOYSTICK_ZONE_DOWN`
    - `JOYSTICK_ZONE_CENTER`
- Only pushes events to the zone queue on zone change.
- Axis Inversion
```c
void joystick_toggle_inversion(void);
```
- Toggles inversion of X/Y axes.
- Inverted coordinates: pos.x = 1 - pos.x, pos.y = 1 - pos.y.

### Queues
```c
QueueHandle_t joystick_get_pos_queue(void);
QueueHandle_t joystick_get_zone_queue(void);
```

Retrieve handles to access joystick positions or zones.

## Buttons

### Initialization
```c
esp_err_t joystick_button_init(void);
```
- Creates button_event_queue for sending button events.
- Configures GPIO pin from CONFIG_BUTTON_GPIO.
- Registers callbacks for:
  - `BUTTON_EVENT_LONG_PRESS` → `BTN_EVT_LONG_PRESS`
  - `BUTTON_EVENT_PRESS_DOWN` → `BTN_EVT_PRESS_DOWN`
  - `BUTTON_EVENT_DOUBLE_CLICK` → `BTN_EVT_DOUBLE_CLICK`

> Optional: BUTTON_EVENT_SINGLE_CLICK → BTN_EVT_SINGLE_CLICK (currently commented)

### Event Sending

Internal helper function:
```c
static void send_button_event(joystick_button_event_t evt);
```
- Sends the mapped button event to `button_event_queue`.

### Queue Access
```c
QueueHandle_t joystick_button_get_queue(void);
```
- Returns handle to receive button events.

### Default Events

- `BTN_EVT_SINGLE_CLICK`
- `BTN_EVT_DOUBLE_CLICK`
- `BTN_EVT_LONG_PRESS`
- `BTN_EVT_PRESS_DOWN`

>Notes
>Both joystick and buttons use FreeRTOS queues, ensuring non-blocking interaction with other system components (LED, ?motor, MQTT, etc.).
>The joystick task runs continuously, while button events are handled via callbacks.
>Event queues allow decoupling of UI logic from application logic.