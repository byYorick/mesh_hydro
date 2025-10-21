idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos
        esp_wifi
        nvs_flash
        driver
        mesh_manager
        mesh_protocol
        node_config
        mesh_config
        ec_sensor
        pump_controller
        pid_controller
        ec_manager
)

