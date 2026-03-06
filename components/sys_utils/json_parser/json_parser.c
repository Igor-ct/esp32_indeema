#include "json_parser.h"
#include "cJSON.h"
#include <string.h>

esp_err_t json_parse_led_command(const char *json_string, parsed_led_cmd_t *out_cmd) {
    if (json_string == NULL || out_cmd == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    out_cmd->has_color = false;
    out_cmd->state = JSON_LED_STATE_NONE;
    out_cmd->r = 0; out_cmd->g = 0; out_cmd->b = 0;

    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        return ESP_FAIL;
    }

    cJSON *led = cJSON_GetObjectItem(root, "led");
    if (cJSON_IsObject(led)) {
        
        cJSON *color = cJSON_GetObjectItem(led, "color");
        if (cJSON_IsObject(color)) {
            cJSON *red = cJSON_GetObjectItem(color, "r");
            cJSON *green = cJSON_GetObjectItem(color, "g");
            cJSON *blue = cJSON_GetObjectItem(color, "b");

            if (cJSON_IsNumber(red) && cJSON_IsNumber(green) && cJSON_IsNumber(blue)) {
                out_cmd->r = (uint8_t)red->valueint;
                out_cmd->g = (uint8_t)green->valueint;
                out_cmd->b = (uint8_t)blue->valueint;
                out_cmd->has_color = true;
            }
        }

        cJSON *state = cJSON_GetObjectItem(led, "state");
        if (cJSON_IsString(state) && (state->valuestring != NULL)) {
            if (strcmp(state->valuestring, "on") == 0) {
                out_cmd->state = JSON_LED_STATE_ON;
            } else if (strcmp(state->valuestring, "off") == 0) {
                out_cmd->state = JSON_LED_STATE_OFF;
            } else if (strcmp(state->valuestring, "auto") == 0) {
                out_cmd->state = JSON_LED_STATE_AUTO;
            }
        }
    }

    cJSON_Delete(root); 
    return ESP_OK;
}

esp_err_t json_parse_motor_command(const char *json_string, parsed_motor_cmd_t *out_cmd) {
    if (json_string == NULL || out_cmd == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    out_cmd->has_angle = false;
    out_cmd->has_mode = false;
    out_cmd->angle = 0.0f;
    out_cmd->mode = 0;

    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        return ESP_FAIL;
    }

    cJSON *motor = cJSON_GetObjectItem(root, "motor");
    if (cJSON_IsObject(motor)) {
        
        cJSON *angle = cJSON_GetObjectItem(motor, "angle");
        if (cJSON_IsNumber(angle)) {
            out_cmd->angle = (float)angle->valuedouble;
            out_cmd->has_angle = true;
        }

        cJSON *mode = cJSON_GetObjectItem(motor, "mode");
        if (cJSON_IsNumber(mode)) {
            out_cmd->mode = mode->valueint;
            out_cmd->has_mode = true;
        }
    }

    cJSON_Delete(root);
    return ESP_OK;
}