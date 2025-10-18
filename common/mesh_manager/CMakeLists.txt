idf_component_register(
    SRCS "mesh_manager.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash
    PRIV_REQUIRES esp_netif
)

