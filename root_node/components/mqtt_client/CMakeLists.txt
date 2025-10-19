idf_component_register(
    SRCS "mqtt_client_manager.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt
    PRIV_REQUIRES esp_wifi
)

