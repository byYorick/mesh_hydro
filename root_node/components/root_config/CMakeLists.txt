idf_component_register(
    SRCS "root_config.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_system
    PRIV_REQUIRES mesh_config esp_timer
)

