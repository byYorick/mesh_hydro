idf_component_register(
    SRCS "node_registry.c"
    INCLUDE_DIRS "."
    REQUIRES json
    PRIV_REQUIRES esp_timer
)

