#include <stdio.h>
#include <string.h>
#include "HTTP_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"

static const char *TAG = "WEB_SERVER";
static httpd_handle_t server = NULL;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

extern const uint8_t style_css_start[] asm("_binary_index_css_start");
extern const uint8_t style_css_end[]   asm("_binary_index_css_end");

static char hex_to_char(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static void url_decode(char *dst, const char *src) {
    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else if (*src == '%' && src[1] && src[2]) {
            *dst++ = (hex_to_char(src[1]) << 4) | hex_to_char(src[2]); 
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0'; 
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    const size_t index_html_size = (index_html_end - index_html_start);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_size);
    return ESP_OK;
}

static esp_err_t style_get_handler(httpd_req_t *req)
{
    const size_t style_css_size = (style_css_end - style_css_start);
    httpd_resp_set_type(req, "text/css"); 
    httpd_resp_send(req, (const char *)style_css_start, style_css_size);
    return ESP_OK;
}

static esp_err_t save_post_handler(httpd_req_t *req)
{
    char buf[200]; 
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    char ssid_raw[96] = {0};
    char pass_raw[128] = {0};

    char ssid[32] = {0};
    char pass[64] = {0};

    if (httpd_query_key_value(buf, "ssid", ssid_raw, sizeof(ssid_raw)) == ESP_OK &&
        httpd_query_key_value(buf, "pass", pass_raw, sizeof(pass_raw)) == ESP_OK) {

        url_decode(ssid, ssid_raw);
        url_decode(pass, pass_raw);

        ESP_LOGI(TAG, "Saving decoded SSID: '%s'", ssid);
        ESP_LOGI(TAG, "Saving decoded PASS: '%s'", pass);

        nvs_handle_t nvs;
        if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
            nvs_set_str(nvs, "ssid", ssid);
            nvs_set_str(nvs, "pass", pass);
            nvs_commit(nvs);
            nvs_close(nvs);
        }
        
        httpd_resp_send(req, "Saved. Rebooting...", HTTPD_RESP_USE_STRLEN);
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart(); 
    } else {
        httpd_resp_send_500(req);
    }
    return ESP_OK;
}

static const httpd_uri_t root = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
static const httpd_uri_t style_uri = { .uri = "/index.css", .method = HTTP_GET, .handler = style_get_handler };
static const httpd_uri_t save = { .uri = "/save", .method = HTTP_POST, .handler = save_post_handler };

void start_webserver(void)
{
    if (server) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &style_uri); 
        httpd_register_uri_handler(server, &save);
    }
}

void stop_webserver(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}