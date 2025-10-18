idf_component_register(
    SRCS "node_controller.c"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos
        esp_task_wdt
        node_config
        connection_monitor
        local_storage
        oled_display
        buzzer_led
        mesh_manager
        mesh_protocol
        json
)

