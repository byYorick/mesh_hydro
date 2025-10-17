/**
 * @file app_main.c
 * @brief NODE Template - пример нового узла
 */

#include "esp_log.h"
#include "nvs_flash.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"

static const char *TAG = "template_main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== NODE Template Starting ===");
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // Mesh (NODE режим)
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .mesh_password = "hydro_mesh_pass",
        .channel = 1
    };
    
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    ESP_ERROR_CHECK(mesh_manager_start());

    ESP_LOGI(TAG, "=== NODE Template Running ===");
    
    while (1) {
        // TODO: Твоя логика здесь
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

