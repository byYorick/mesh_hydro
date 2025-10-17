/**
 * @file app_main.c
 * @brief NODE pH/EC - критичный узел с автономией
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

// TODO: Добавить остальные компоненты из hydro1.0

static const char *TAG = "ph_ec_main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== NODE pH/EC Starting ===");
    ESP_LOGI(TAG, "ESP32-S3 - pH/EC Control with Autonomy");

    // Инициализация NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // TODO: Инициализация всех компонентов
    // - sensor_manager (pH, EC)
    // - pump_manager (5 насосов)
    // - adaptive_pid
    // - oled_display
    // - connection_monitor
    // - local_storage
    // - buzzer_led

    // Mesh (NODE режим)
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .mesh_password = "hydro_mesh_pass",
        .channel = 1
    };
    
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    ESP_ERROR_CHECK(mesh_manager_start());

    ESP_LOGI(TAG, "=== NODE pH/EC Running ===");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

