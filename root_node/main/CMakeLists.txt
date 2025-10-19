idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos
        nvs_flash
        esp_wifi
        esp_event
        esp_system
        mesh_manager
        mesh_protocol
        mesh_config
        node_registry
        mqtt_client
        data_router
        climate_logic
        json
)

