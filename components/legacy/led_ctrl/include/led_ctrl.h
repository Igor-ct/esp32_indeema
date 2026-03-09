#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int gpio;
    bool active_high;
} led_ctrl_t;

int  led_ctrl_init(led_ctrl_t *led, int gpio, bool active_high);
void led_ctrl_set(led_ctrl_t *led, bool on);
void led_ctrl_toggle(led_ctrl_t *led);

#ifdef __cplusplus
}
#endif