idf_component_register(
    SRCS "climate_controller.c"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos
        node_config
        sht3x_driver
        ccs811_driver
        lux_sensor
        mesh_manager
        mesh_protocol
        json
)

