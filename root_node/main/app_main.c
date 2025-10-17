/**
 * @file app_main.c
 * @brief Точка входа ROOT узла
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "node_registry.h"
#include "mqtt_client_manager.h"
#include "data_router.h"
#include "climate_logic.h"

static const char *TAG = "root_main";

#define MESH_ID "HYDRO1"
#define MESH_PASSWORD "hydro_mesh_pass"
#define MESH_CHANNEL 1
#define MAX_CONNECTIONS 10

#define ROUTER_SSID CONFIG_WIFI_SSID
#define ROUTER_PASSWORD CONFIG_WIFI_PASSWORD

void app_main(void)
{
    ESP_LOGI(TAG, "=== ROOT NODE Starting ===");
    ESP_LOGI(TAG, "ESP32-S3 #1 - Mesh Coordinator + MQTT Bridge");

    // Инициализация NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Инициализация компонентов
    ESP_LOGI(TAG, "Initializing components...");
    
    // 1. Node Registry
    node_registry_init();
    
    // 2. Mesh Manager (ROOT режим)
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_ROOT,
        .mesh_id = MESH_ID,
        .mesh_password = MESH_PASSWORD,
        .channel = MESH_CHANNEL,
        .max_connection = MAX_CONNECTIONS,
        .router_ssid = ROUTER_SSID,
        .router_password = ROUTER_PASSWORD
    };
    
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    
    // 3. Data Router
    data_router_init();
    
    // 4. Climate Logic (резервная)
    climate_logic_init();
    
    // 5. MQTT Client
    mqtt_client_manager_init();
    
    // Запуск mesh
    ESP_LOGI(TAG, "Starting mesh network...");
    ESP_ERROR_CHECK(mesh_manager_start());
    
    ESP_LOGI(TAG, "=== ROOT NODE Running ===");
    
    // Основной цикл
    while (1) {
        // Статистика каждые 30 секунд
        vTaskDelay(pdMS_TO_TICKS(30000));
        
        int total_nodes = mesh_manager_get_total_nodes();
        ESP_LOGI(TAG, "Mesh nodes: %d, Registry: %d", 
                 total_nodes, node_registry_get_count());
    }
}

