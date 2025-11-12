idf_component_register(
    SRCS "mesh_protocol.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash cjson zone_config
)
