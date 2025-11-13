idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES
        freertos
        nvs_flash
        esp_wifi
        esp_event
        esp_system
        esp_timer
        esp_app_format
        mesh_manager
        mesh_protocol
        zone_config
        mesh_config
        node_config
        json
        oled_display
)


