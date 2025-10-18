idf_component_register(
    SRCS "mesh_protocol.c"
    INCLUDE_DIRS "."
    REQUIRES json
    PRIV_REQUIRES esp_timer
)

