idf_component_register(
    SRCS "pump_events.c"
    INCLUDE_DIRS "."
    REQUIRES mesh_manager json esp_timer pump_controller pid_controller
)
