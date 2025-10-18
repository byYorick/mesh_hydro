/**
 * @file climate_logic.c
 * @brief Реализация резервной климатической логики
 */

#include "climate_logic.h"
#include "node_registry.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "climate_logic";

static TaskHandle_t s_climate_task = NULL;
static bool s_fallback_active = false;

// Простые таймеры для fallback логики
#define WINDOW_OPEN_INTERVAL_MS    3600000  // 1 час
#define WINDOW_OPEN_DURATION_MS    300000   // 5 минут
#define FAN_CHECK_INTERVAL_MS      600000   // 10 минут

// Forward declaration
static void climate_fallback_task(void *arg);
static void send_command_to_relay(const char *command);

esp_err_t climate_logic_init(void) {
    ESP_LOGI(TAG, "Climate fallback logic initialized");
    return climate_logic_start();
}

esp_err_t climate_logic_start(void) {
    if (s_climate_task != NULL) {
        ESP_LOGW(TAG, "Climate task already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(climate_fallback_task, 
                                  "climate_fallback", 
                                  4096, 
                                  NULL, 
                                  5, 
                                  &s_climate_task);
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create climate task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Climate fallback task started");
    return ESP_OK;
}

esp_err_t climate_logic_stop(void) {
    if (s_climate_task != NULL) {
        vTaskDelete(s_climate_task);
        s_climate_task = NULL;
        s_fallback_active = false;
        ESP_LOGI(TAG, "Climate fallback task stopped");
    }
    return ESP_OK;
}

bool climate_logic_is_fallback_active(void) {
    return s_fallback_active;
}

// Задача резервной климатической логики
static void climate_fallback_task(void *arg) {
    uint64_t last_window_open = 0;
    uint64_t last_fan_check = 0;
    bool windows_are_open = false;

    ESP_LOGI(TAG, "Climate fallback task running");

    while (1) {
        // Проверка наличия Climate узла
        bool climate_online = node_registry_has_type("climate");

        if (!climate_online) {
            if (!s_fallback_active) {
                ESP_LOGW(TAG, "Climate node OFFLINE - activating fallback logic");
                s_fallback_active = true;
            }

            uint64_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

            // Логика проветривания (каждый час на 5 минут)
            if (now - last_window_open > WINDOW_OPEN_INTERVAL_MS) {
                if (!windows_are_open) {
                    ESP_LOGI(TAG, "Fallback: Opening windows");
                    send_command_to_relay("open_windows");
                    windows_are_open = true;
                    last_window_open = now;
                }
            }

            // Закрытие окон через 5 минут
            if (windows_are_open && (now - last_window_open > WINDOW_OPEN_DURATION_MS)) {
                ESP_LOGI(TAG, "Fallback: Closing windows");
                send_command_to_relay("close_windows");
                windows_are_open = false;
            }

            // Периодическая проверка вентиляции
            if (now - last_fan_check > FAN_CHECK_INTERVAL_MS) {
                ESP_LOGI(TAG, "Fallback: Starting fan");
                send_command_to_relay("start_fan");
                
                // Остановка вентилятора через 2 минуты
                vTaskDelay(pdMS_TO_TICKS(120000));
                send_command_to_relay("stop_fan");
                
                last_fan_check = now;
            }

        } else {
            // Climate узел онлайн - деактивация fallback
            if (s_fallback_active) {
                ESP_LOGI(TAG, "Climate node ONLINE - deactivating fallback logic");
                s_fallback_active = false;
                
                // Сброс состояния
                if (windows_are_open) {
                    send_command_to_relay("close_windows");
                    windows_are_open = false;
                }
            }
        }

        // Проверка каждые 60 секунд
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

// Отправка команды на Relay узел
static void send_command_to_relay(const char *command) {
    node_info_t *relay = node_registry_get("relay_001");
    
    if (!relay || !relay->online) {
        ESP_LOGW(TAG, "Relay node offline, cannot send command: %s", command);
        return;
    }

    // Создание JSON команды
    cJSON *params = cJSON_CreateObject();
    
    char json_buf[256];
    if (mesh_protocol_create_command("relay_001", command, params, 
                                      json_buf, sizeof(json_buf))) {
        esp_err_t err = mesh_manager_send(relay->mac_addr, 
                                         (uint8_t *)json_buf, strlen(json_buf));
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Sent command to relay: %s", command);
        } else {
            ESP_LOGE(TAG, "Failed to send command: %s", esp_err_to_name(err));
        }
    }
    
    cJSON_Delete(params);
}

