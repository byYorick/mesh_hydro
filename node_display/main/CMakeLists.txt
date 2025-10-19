idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        freertos
        nvs_flash
        esp_wifi
        esp_timer
        json
)
