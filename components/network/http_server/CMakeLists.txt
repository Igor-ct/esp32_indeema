idf_component_register(SRCS "http_server.c"
                    REQUIRES esp_http_server nvs_flash esp_wifi
                    EMBED_TXTFILES "frontend/index.html" "frontend/index.css"
                    INCLUDE_DIRS "include")
