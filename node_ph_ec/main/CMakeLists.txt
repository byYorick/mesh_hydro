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
        mesh_manager
        mesh_protocol
        mesh_config        # Централизованная конфигурация
        node_config
        ph_sensor
        ec_sensor
        pump_controller
        ph_ec_manager
        json
)
