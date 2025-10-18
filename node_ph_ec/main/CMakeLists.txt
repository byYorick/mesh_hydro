idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos
        nvs_flash
        esp_wifi
        esp_event
        esp_task_wdt
        driver
        # Common компоненты
        mesh_manager
        mesh_protocol
        node_config
        # NODE pH/EC компоненты
        node_controller
        connection_monitor
        local_storage
        oled_display
        buzzer_led
        sensor_manager
        pump_manager
        adaptive_pid
        json
)

