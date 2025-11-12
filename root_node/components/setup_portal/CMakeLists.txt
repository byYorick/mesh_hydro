idf_component_register(SRCS "setup_portal.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_server esp_wifi esp_netif nvs_flash json
                    PRIV_REQUIRES esp_event)
