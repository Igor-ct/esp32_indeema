#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>           

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "ble.h"         
#include "esp_log.h"
#include "motor_service.h"

static const char *manuf_name = CONFIG_BLE_MANUFACTURER_NAME;
static const char *model_num  = CONFIG_BLE_MODEL_NUMBER;
const char *device_name = CONFIG_BLE_DEVICE_NAME; 
uint8_t own_addr_type;

uint8_t battery_level = 85;
static uint8_t ble_led_state = 0;
static uint8_t ble_led_color[3] = {255, 255, 255};
static uint8_t ble_led_hw_state = 1;
static int priority = 4;


static const ble_uuid128_t gatt_svr_svc_led_uuid =
    BLE_UUID128_INIT(0x00, 0x00, 0x23, 0x23, 0x12, 0x12, 0xef, 0xde, 
                     0x15, 0x23, 0x78, 0x5f, 0xef, 0x13, 0xd1, 0x23);

static const ble_uuid16_t dsc_user_desc_uuid = BLE_UUID16_INIT(0x2901);
static int gatt_svr_dsc_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_chr_access_battery(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_chr_access_cts(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_chr_access_led(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_chr_access_motor_raw(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static char ble_telemetry_json[256] = "{}"; 
static uint16_t telemetry_conn_handle = BLE_HS_CONN_HANDLE_NONE; 
static uint16_t telemetry_chr_val_handle; 

static const ble_uuid128_t gatt_svr_svc_telemetry_uuid =
    BLE_UUID128_INIT(0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 
                     0x99, 0xaa, 0xbb, 0xcc, 0x00, 0x00, 0x00, 0x00);

static const ble_uuid128_t gatt_svr_chr_telemetry_uuid =
    BLE_UUID128_INIT(0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 
                     0x99, 0xaa, 0xbb, 0xcc, 0x01, 0x00, 0x00, 0x00);

static const ble_uuid128_t gatt_svr_svc_motor_uuid =
    BLE_UUID128_INIT(0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 
                     0x99, 0xaa, 0xbb, 0xcc, 0x02, 0x00, 0x00, 0x00);

static const ble_uuid128_t gatt_svr_chr_motor_cmd_uuid =
    BLE_UUID128_INIT(0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 
                     0x99, 0xaa, 0xbb, 0xcc, 0x03, 0x00, 0x00, 0x00);

static int gatt_svr_chr_access_telemetry(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) {
            { .uuid = BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID), .access_cb = gatt_svr_chr_access_device_info, .flags = BLE_GATT_CHR_F_READ },
            { .uuid = BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID), .access_cb = gatt_svr_chr_access_device_info, .flags = BLE_GATT_CHR_F_READ },
            {0}
        }
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_BAS_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) {
            { .uuid = BLE_UUID16_DECLARE(GATT_BAS_BATTERY_LEVEL_UUID), .access_cb = gatt_svr_chr_access_battery, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY },
            {0}
        }
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_CTS_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) { 
            { .uuid = BLE_UUID16_DECLARE(GATT_CTS_CHR_TIME_UUID), .access_cb = gatt_svr_chr_access_cts, .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY }, 
            {0} 
        }
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (ble_uuid_t*)&gatt_svr_svc_led_uuid,
        .characteristics = (struct ble_gatt_chr_def[]) { 
            { 
                .uuid = BLE_UUID16_DECLARE(GATT_LED_CHR_STATE_UUID), 
                .access_cb = gatt_svr_chr_access_led, 
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = (ble_uuid_t*)&dsc_user_desc_uuid, 
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_dsc_access,
                        .arg = (void *)"LED Mode (0=OFF, 1=ON, 2=AUTO)"
                    },
                    {0}
                }
            }, 
            { 
                .uuid = BLE_UUID16_DECLARE(GATT_LED_CHR_COLOR_UUID), 
                .access_cb = gatt_svr_chr_access_led, 
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = (ble_uuid_t*)&dsc_user_desc_uuid, 
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_dsc_access,
                        .arg = (void *)"LED RGB Color (3 Bytes: R, G, B)"
                    },
                    {0}
                }
            }, 
            {0} 
        }
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (ble_uuid_t*)&gatt_svr_svc_telemetry_uuid,
        .characteristics = (struct ble_gatt_chr_def[]) { 
            { 
                .uuid = (ble_uuid_t*)&gatt_svr_chr_telemetry_uuid, 
                .access_cb = gatt_svr_chr_access_telemetry, 
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &telemetry_chr_val_handle, 
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = (ble_uuid_t*)&dsc_user_desc_uuid, 
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_dsc_access,
                        .arg = (void *)"Sensor Telemetry (JSON)"
                    },
                    {0}
                }
            }, 
            {0} 
        }
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (ble_uuid_t*)&gatt_svr_svc_motor_uuid,
        .characteristics = (struct ble_gatt_chr_def[]) { 
            { 
                .uuid = (ble_uuid_t*)&gatt_svr_chr_motor_cmd_uuid, 
                .access_cb = gatt_svr_chr_access_motor_raw, 
                .flags = BLE_GATT_CHR_F_WRITE,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = (ble_uuid_t*)&dsc_user_desc_uuid, 
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_dsc_access,
                        .arg = (void *)"Motor Control (B0: Mode, B1-4: Float Angle)"
                    },
                    {0}
                }
            }, 
            {0} 
        }
    },
    {0}   
};

static int gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid = ble_uuid_u16(ctxt->chr->uuid);
    int rc;

    if (uuid == GATT_MODEL_NUMBER_UUID) {
        rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    if (uuid == GATT_MANUFACTURER_NAME_UUID) {
        rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_svr_chr_access_battery(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    return os_mbuf_append(ctxt->om, &battery_level, sizeof(battery_level)) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int gatt_svr_chr_access_cts(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    uint8_t cts_data[10];
    uint16_t year = timeinfo.tm_year + 1900;

    cts_data[0] = year & 0xff;
    cts_data[1] = year >> 8;
    cts_data[2] = timeinfo.tm_mon + 1;
    cts_data[3] = timeinfo.tm_mday;
    cts_data[4] = timeinfo.tm_hour;
    cts_data[5] = timeinfo.tm_min;
    cts_data[6] = timeinfo.tm_sec;
    cts_data[7] = timeinfo.tm_wday == 0 ? 7 : timeinfo.tm_wday; 
    cts_data[8] = 0; 
    cts_data[9] = 1; 

    return os_mbuf_append(ctxt->om, cts_data, sizeof(cts_data)) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int gatt_svr_chr_access_led(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        
        if (uuid == GATT_LED_CHR_STATE_UUID) {
            ble_led_state = ctxt->om->om_data[0];

            if (ble_led_state == 0) { 
                led_send_remote_command(LED_REMOTE_OFF, 0, 0, 0, priority);
                ESP_LOGI("BLE", "Command: LED OFF");
            } 
            else if (ble_led_state == 1) {
                led_send_remote_command(LED_REMOTE_ON, ble_led_color[0], ble_led_color[1], ble_led_color[2], priority);
                ESP_LOGI("BLE", "Command: LED ON (R:%d G:%d B:%d)", ble_led_color[0], ble_led_color[1], ble_led_color[2]);
            }
            else if (ble_led_state == 2) {
                led_send_remote_command(LED_REMOTE_OFF, 0, 0, 0, priority);
                ESP_LOGI("BLE", "Command: LED AUTO (Override disabled)");
            }
            return 0;
        }

        if (uuid == GATT_LED_CHR_COLOR_UUID) {
            if (OS_MBUF_PKTLEN(ctxt->om) == 3) {
                memcpy(ble_led_color, ctxt->om->om_data, 3);
                
                if (ble_led_state == 1) {
                    led_send_remote_command(LED_REMOTE_ON, ble_led_color[0], ble_led_color[1], ble_led_color[2], priority);
                }
                
                ESP_LOGI("BLE", "Color updated: %d,%d,%d", ble_led_color[0], ble_led_color[1], ble_led_color[2]);
                return 0;
            }
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        if (uuid == GATT_LED_CHR_INIT_UUID) {
            ble_led_hw_state = ctxt->om->om_data[0];
            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        if (uuid == GATT_LED_CHR_STATE_UUID) {
            return os_mbuf_append(ctxt->om, &ble_led_state, 1) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        if (uuid == GATT_LED_CHR_COLOR_UUID) {
            return os_mbuf_append(ctxt->om, ble_led_color, 3) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        if (uuid == GATT_LED_CHR_INIT_UUID) {
            return os_mbuf_append(ctxt->om, &ble_led_hw_state, 1) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

int ble_gatt_svr_init(void)
{
    int rc;
    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) return rc;

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) return rc;

    return 0;
}

static int gap_event_handler(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                printf("Connection established! (conn_handle: %d)\n", event->connect.conn_handle);
                telemetry_conn_handle = event->connect.conn_handle; 
            } else {
                printf("Connection failed (status: %d), resuming advertising...\n", event->connect.status);
                ble_app_advertise();
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            printf("Disconnected (reason: %d), resuming advertising...\n", event->disconnect.reason);
            telemetry_conn_handle = BLE_HS_CONN_HANDLE_NONE; 
            ble_app_advertise();
            break;
    }
    return 0;
}

void ble_app_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)ble_svc_gap_device_name();
    fields.name_len = strlen((char *)fields.name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) return;

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; 
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; 

    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, gap_event_handler, NULL);
    if (rc == 0) {
        printf(" Advertising successfully started!\n");
    }
}

static void ble_app_on_sync(void)
{
    int rc = ble_svc_gap_device_name_set(device_name);
    assert(rc == 0);

    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    ble_app_advertise();
}

void ble_setup_stack_and_security(void)
{
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO; 
    ble_hs_cfg.sm_bonding = 1; 
    ble_hs_cfg.sm_mitm = 0; 
    ble_hs_cfg.sm_sc = 1; 
    ble_hs_cfg.sm_our_key_dist = 1;
    ble_hs_cfg.sm_their_key_dist = 1;
}


static int gatt_svr_dsc_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const char *desc = (const char *)arg;
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC) {
        return os_mbuf_append(ctxt->om, desc, strlen(desc)) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_svr_chr_access_telemetry(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        int rc = os_mbuf_append(ctxt->om, ble_telemetry_json, strlen(ble_telemetry_json));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

void ble_update_telemetry(const char *json_data)
{
    strncpy(ble_telemetry_json, json_data, sizeof(ble_telemetry_json) - 1);
    ble_telemetry_json[sizeof(ble_telemetry_json) - 1] = '\0';

    if (telemetry_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        struct os_mbuf *om = ble_hs_mbuf_from_flat(ble_telemetry_json, strlen(ble_telemetry_json));
        if (om) {
            ble_gatts_notify_custom(telemetry_conn_handle, telemetry_chr_val_handle, om);
            ESP_LOGD("BLE", "Telemetry notification sent");
        }
    }
}



static int gatt_svr_chr_access_motor_raw(uint16_t conn_handle, uint16_t attr_handle, 
                                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        
        if (len < 5) {
            ESP_LOGE("BLE", "Invalid data length: %d", len);
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        uint8_t raw_data[5];
        ble_hs_mbuf_to_flat(ctxt->om, raw_data, 5, &len);

        motor_mode_t mode = (motor_mode_t)raw_data[0];

        float angle;
        memcpy(&angle, &raw_data[1], sizeof(float));

        motor_send_remote_command(mode, angle, priority);

        ESP_LOGI("BLE", "Raw Command: Mode %d, Angle %.1f", mode, angle);
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}