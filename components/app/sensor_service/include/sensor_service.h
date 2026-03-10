#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool valid;
    float temperature;
    float humidity;
} aht20_data_t;

typedef struct
{
    bool valid;
    float temperature;
    float pressure; 
} bmp280_data_t;

typedef struct
{
    bool valid;
    float x; 
    float y; 
    float z; 
} accel_data_t;

typedef struct
{
    aht20_data_t aht20;
    bmp280_data_t bmp280;
    accel_data_t accel;

} sensor_data_t;

void sensor_service_init(void);

bool sensor_service_read(sensor_data_t *data);