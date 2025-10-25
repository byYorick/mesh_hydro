idf_component_register(
    SRCS "pump_events.c"
    INCLUDE_DIRS "."
    REQUIRES "mesh_manager" "cjson" "esp_timer"
)
