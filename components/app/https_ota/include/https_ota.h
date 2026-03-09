#pragma once
#include <stdbool.h>
#include "esp_err.h"

void ota_service_init(void);

bool ota_service_start_update(const char *url);