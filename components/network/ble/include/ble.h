#pragma once

#include <stdint.h>
#include "host/ble_uuid.h"
#include "led_service.h"
#include "ws2812.h"      

#define GATT_DEVICE_INFO_UUID               0x180A
#define GATT_MANUFACTURER_NAME_UUID         0x2A29
#define GATT_MODEL_NUMBER_UUID              0x2A24

#define GATT_BAS_UUID                       0x180F
#define GATT_BAS_BATTERY_LEVEL_UUID         0x2A19

#define GATT_CTS_UUID                       0x1805
#define GATT_CTS_CHR_TIME_UUID              0x2A2B

#define GATT_LED_CHR_STATE_UUID             0xDEAD
#define GATT_LED_CHR_COLOR_UUID             0xBEEF
#define GATT_LED_CHR_INIT_UUID              0xCAFE

extern uint8_t battery_level;

int gatt_svr_init(void);
void ble_setup_stack_and_security(void);
void ble_app_advertise(void);
led_cmd_t get_ble_bt_target_color(void);
bool get_ble_status_overriden_led(void);
void ble_update_telemetry(const char *json_data);