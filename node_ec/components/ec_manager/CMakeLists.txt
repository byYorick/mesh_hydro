idf_component_register(
    SRCS "ec_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        ec_sensor
        pump_controller
        pid_controller
        adaptive_pid
        pump_events
        mesh_manager
        mesh_protocol
        node_config
        esp_wifi
)

