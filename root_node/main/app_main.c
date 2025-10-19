/**
 * @file app_main.c
 * @brief ROOT NODE - Точка входа координатора mesh-сети
 * 
 * ESP32-S3 #1 - Mesh ROOT + MQTT Bridge
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "esp_netif.h"

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"

// ROOT компоненты
#include "node_registry.h"
#include "mqtt_client_manager.h"
#include "data_router.h"
#include "climate_logic.h"
#include "root_config.h"

static const char *TAG = "ROOT";

/**
 * @brief Задача мониторинга системы
 * 
 * Периодически логирует статус системы и проверяет таймауты узлов
 */
static void root_monitoring_task(void *arg) {
    uint32_t last_log_ms = 0;
    
    // Регистрация в watchdog
    esp_task_wdt_add(NULL);
    
    ESP_LOGI(TAG, "Monitoring task started");
    
    while (1) {
        // Сброс watchdog
        esp_task_wdt_reset();
        
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Проверка таймаутов узлов
        node_registry_check_timeouts();
        
        // Логирование статуса каждые 30 секунд
        if (now_ms - last_log_ms > ROOT_MONITORING_INTERVAL_MS) {
            uint32_t free_heap = esp_get_free_heap_size();
            int mesh_nodes = mesh_manager_get_total_nodes();
            int registry_nodes = node_registry_get_count();
            bool mqtt_online = mqtt_client_manager_is_connected();
            bool fallback_active = climate_logic_is_fallback_active();
            
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "=== ROOT NODE STATUS ===");
            ESP_LOGI(TAG, "Free heap: %d bytes", free_heap);
            ESP_LOGI(TAG, "Mesh nodes: %d (total), %d (online)", mesh_nodes, registry_nodes);
            ESP_LOGI(TAG, "MQTT: %s", mqtt_online ? "ONLINE" : "OFFLINE");
            ESP_LOGI(TAG, "Climate fallback: %s", fallback_active ? "ACTIVE" : "INACTIVE");
            ESP_LOGI(TAG, "========================================");
            
            // Отправка discovery сообщения (для регистрации на сервере)
            if (mqtt_online) {
                mqtt_client_manager_send_discovery();
            }
            
            // Предупреждение при низкой памяти
            if (free_heap < 50000) {
                ESP_LOGW(TAG, "LOW HEAP MEMORY!");
            }
            
            last_log_ms = now_ms;
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Проверка каждые 5 сек
    }
}

/**
 * @brief Главная функция приложения
 */
void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== HYDRO MESH ROOT NODE Starting ===");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3 #1 - Mesh Coordinator + MQTT Bridge");
    ESP_LOGI(TAG, "Build date: %s %s", __DATE__, __TIME__);
    
    // Шаг 1: Инициализация NVS
    ESP_LOGI(TAG, "[Step 1/7] Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs to be erased");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
    
    // Шаг 2: Инициализация Node Registry
    ESP_LOGI(TAG, "[Step 2/7] Initializing Node Registry...");
    ESP_ERROR_CHECK(node_registry_init());
    
    // Шаг 3: Инициализация Mesh Manager (ROOT режим)
    ESP_LOGI(TAG, "[Step 3/7] Initializing Mesh (ROOT mode)...");
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_ROOT,
        .mesh_id = ROOT_MESH_ID,
        .mesh_password = ROOT_MESH_PASSWORD,
        .channel = ROOT_MESH_CHANNEL,
        .max_connection = ROOT_MAX_CONNECTIONS,
        .router_ssid = ROOT_ROUTER_SSID,
        .router_password = ROOT_ROUTER_PASSWORD,
        .router_bssid = NULL  // NULL = автоопределение роутера
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    ESP_LOGI(TAG, "Mesh ID: %s, Channel: %d", ROOT_MESH_ID, ROOT_MESH_CHANNEL);
    
    // Шаг 4: Запуск Mesh
    ESP_LOGI(TAG, "[Step 4/7] Starting Mesh network...");
    ESP_ERROR_CHECK(mesh_manager_start());
    ESP_LOGI(TAG, "Mesh network started");
    
    // Ожидание подключения к роутеру и получения IP (для MQTT)
    ESP_LOGI(TAG, "Waiting for IP address from DHCP...");
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info;
    
    // Ждем до 30 секунд получения IP
    int retry_count = 0;
    const int max_retries = 60;  // 60 * 500ms = 30 секунд
    bool ip_obtained = false;
    
    while (retry_count < max_retries) {
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            if (ip_info.ip.addr != 0) {
                ESP_LOGI(TAG, "✓ IP address obtained: " IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG, "✓ Netmask: " IPSTR, IP2STR(&ip_info.netmask));
                ESP_LOGI(TAG, "✓ Gateway: " IPSTR, IP2STR(&ip_info.gw));
                ip_obtained = true;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        retry_count++;
        
        // Логируем прогресс каждые 5 секунд
        if (retry_count % 10 == 0) {
            ESP_LOGW(TAG, "Still waiting for IP... (%d/%d)", retry_count, max_retries);
        }
    }
    
    if (!ip_obtained) {
        ESP_LOGE(TAG, "❌ CRITICAL: Failed to obtain IP address from DHCP!");
        ESP_LOGE(TAG, "Possible causes:");
        ESP_LOGE(TAG, "  1. DHCP server not responding");
        ESP_LOGE(TAG, "  2. Router blocking this device");
        ESP_LOGE(TAG, "  3. No available IP addresses in DHCP pool");
        ESP_LOGE(TAG, "MQTT connection will FAIL without IP address!");
        ESP_LOGW(TAG, "Continuing startup, but MQTT will not work...");
    }
    
    // Шаг 5: Инициализация MQTT Client
    ESP_LOGI(TAG, "[Step 5/7] Initializing MQTT Client...");
    if (ip_obtained) {
        ESP_ERROR_CHECK(mqtt_client_manager_init());
        ESP_ERROR_CHECK(mqtt_client_manager_start());
        ESP_LOGI(TAG, "MQTT Client started (connecting to broker...)");
    } else {
        ESP_LOGE(TAG, "Skipping MQTT initialization - no IP address!");
    }
    
    // Шаг 6: Инициализация Data Router
    ESP_LOGI(TAG, "[Step 6/7] Initializing Data Router...");
    ESP_ERROR_CHECK(data_router_init());
    ESP_LOGI(TAG, "Data Router initialized");
    
    // Шаг 7: Инициализация Climate Logic (fallback)
    ESP_LOGI(TAG, "[Step 7/7] Initializing Climate Fallback Logic...");
    ESP_ERROR_CHECK(climate_logic_init());
    ESP_LOGI(TAG, "Climate Fallback Logic initialized");
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== ROOT NODE Running ===");
    ESP_LOGI(TAG, "Mesh ID: %s", ROOT_MESH_ID);
    ESP_LOGI(TAG, "MQTT Broker: %s", ROOT_MQTT_BROKER_URI);
    ESP_LOGI(TAG, "========================================");
    
    // Запуск задачи мониторинга
    // Stack увеличен в 2 раза для безопасности: 4096 → 8192
    xTaskCreate(root_monitoring_task, "root_monitor", 8192, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "All systems operational. ROOT node ready.");
}
