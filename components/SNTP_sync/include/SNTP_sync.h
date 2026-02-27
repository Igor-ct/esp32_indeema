#pragma once
#include "esp_log.h"
#include "esp_event.h"

void time_sync_notification_cb(struct timeval *tv);
void SNTP_service_init(void);
void sntp_net_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);


