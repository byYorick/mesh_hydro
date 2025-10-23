idf_component_register(
    SRCS "mqtt_client_manager.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt mesh_config
    PRIV_REQUIRES esp_wifi esp_hw_support
)

