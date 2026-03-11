idf_component_register(SRCS "https_ota.c"
                    REQUIRES esp_https_ota app_update
                    EMBED_TXTFILES "cert/ca_cert.pem"
                    INCLUDE_DIRS "include")
