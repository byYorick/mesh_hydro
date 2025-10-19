idf_component_register(
    SRCS "ph_ec_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos
        esp_wifi
        esp_system
        esp_timer
        ph_sensor
        ec_sensor
        pump_controller
        pid_controller
        mesh_manager
        mesh_protocol
        node_config
        json
)

