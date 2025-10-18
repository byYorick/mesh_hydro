idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos
        nvs_flash
        esp_wifi
        esp_event
        esp_system
        driver
        # Common компоненты
        mesh_manager
        mesh_protocol
        node_config
        # NODE Climate компоненты
        climate_controller
        sht3x_driver
        ccs811_driver
        lux_sensor
        json
)
