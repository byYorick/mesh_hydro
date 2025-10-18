idf_component_register(
    SRCS "climate_logic.c"
    INCLUDE_DIRS "."
    REQUIRES node_registry mesh_manager mesh_protocol json freertos
)

