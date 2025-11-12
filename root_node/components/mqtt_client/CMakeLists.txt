idf_component_register(
    SRCS "mqtt_client_manager.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt mesh_config root_config zone_config mesh_protocol
    PRIV_REQUIRES esp_wifi esp_hw_support
)

