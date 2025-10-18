idf_component_register(
    SRCS "data_router.c"
    INCLUDE_DIRS "."
    REQUIRES mesh_manager mesh_protocol node_registry mqtt_client json
    PRIV_REQUIRES esp_netif
)

