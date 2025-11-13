idf_component_register(
    SRCS "ph_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        ph_sensor
        pump_controller
        pid_controller
        adaptive_pid
        pump_events
        mesh_manager
        mesh_protocol
        mesh_config
        node_config
        esp_wifi
        json
        oled_display
)

