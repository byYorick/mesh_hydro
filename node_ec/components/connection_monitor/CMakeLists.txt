idf_component_register(
    SRCS "connection_monitor.c"
    INCLUDE_DIRS "."
    REQUIRES freertos esp_timer
)

