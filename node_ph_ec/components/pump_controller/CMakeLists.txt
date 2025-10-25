idf_component_register(
    SRCS "pump_controller.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer freertos pump_events mesh_manager
)

