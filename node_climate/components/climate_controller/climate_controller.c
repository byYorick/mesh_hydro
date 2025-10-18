/**
 * @file climate_controller.c
 * @brief Реализация контроллера климата
 */

#include "climate_controller.h"
#include "sht3x_driver.h"
#include "ccs811_driver.h"
#include "lux_sensor.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

#include "esp_log.h"
// #include "esp_task_wdt.h"  // Закомментировано для совместимости с ESP-IDF v5.5
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdbool.h>

static const char *TAG = "climate_ctrl";

static climate_node_config_t *s_config = NULL;
static TaskHandle_t s_main_task = NULL;

// Forward declarations
static void climate_main_task(void *arg);
static void send_telemetry(float temp, float humidity, uint16_t co2, uint16_t lux);
static esp_err_t read_all_sensors(float *temp, float *humidity, uint16_t *co2, uint16_t *lux);

esp_err_t climate_controller_init(climate_node_config_t *config) {
    if (!config) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    s_config = config;

    ESP_LOGI(TAG, "Climate Controller initialized");
    ESP_LOGI(TAG, "Node ID: %s, Zone: %s", s_config->base.node_id, s_config->base.zone);
    ESP_LOGI(TAG, "Read interval: %d ms", s_config->read_interval_ms);

    return ESP_OK;
}

esp_err_t climate_controller_start(void) {
    if (s_main_task != NULL) {
        ESP_LOGW(TAG, "Main task already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(climate_main_task,
                                  "climate_main",
                                  4096,
                                  NULL,
                                  5,
                                  &s_main_task);
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Climate Controller started");
    return ESP_OK;
}

esp_err_t climate_controller_stop(void) {
    if (s_main_task != NULL) {
        vTaskDelete(s_main_task);
        s_main_task = NULL;
        ESP_LOGI(TAG, "Climate Controller stopped");
    }
    return ESP_OK;
}

// Главная задача чтения датчиков
static void climate_main_task(void *arg) {
    ESP_LOGI(TAG, "Main task running (interval: %d ms)", s_config->read_interval_ms);
    
    // Регистрация в watchdog (закомментировано для ESP-IDF v5.5)
    // esp_task_wdt_add(NULL);

    float temp, humidity;
    uint16_t co2, lux;

    while (1) {
        // Сброс watchdog (закомментировано для ESP-IDF v5.5)
        // esp_task_wdt_reset();

        // Чтение всех датчиков
        esp_err_t ret = read_all_sensors(&temp, &humidity, &co2, &lux);

        if (ret == ESP_OK) {
            // Отправка телеметрии на ROOT (включая частичные данные)
            send_telemetry(temp, humidity, co2, lux);

            // Валидация данных (только для валидных значений)
            if (temp > -100.0f) {  // Валидная температура
                if (temp < -10.0f || temp > 50.0f) {
                    ESP_LOGW(TAG, "Temperature out of range: %.1f°C", temp);
                }
            }
            if (co2 > 0 && co2 > 5000) {
                ESP_LOGW(TAG, "CO2 very high: %d ppm", co2);
            }
        } else {
            ESP_LOGW(TAG, "All sensors failed - skipping telemetry");
        }

        // Интервал чтения
        vTaskDelay(pdMS_TO_TICKS(s_config->read_interval_ms));
    }
}

// Чтение всех датчиков с retry логикой
static esp_err_t read_all_sensors(float *temp, float *humidity, uint16_t *co2, uint16_t *lux) {
    const int max_retries = 3;
    esp_err_t ret_temp = ESP_FAIL;
    esp_err_t ret_co2 = ESP_FAIL;
    esp_err_t ret_lux = ESP_FAIL;

    // Retry для каждого датчика
    for (int i = 0; i < max_retries; i++) {
        // SHT3x (температура + влажность)
        if (ret_temp != ESP_OK) {
            ret_temp = sht3x_read(temp, humidity);
        }

        // CCS811 (CO2)
        if (ret_co2 != ESP_OK) {
            ret_co2 = ccs811_read(co2, NULL);
            
            // Компенсация температуры/влажности для CCS811
            if (ret_temp == ESP_OK) {
                ccs811_set_environment(*temp, *humidity);
            }
        }

        // Lux sensor
        if (ret_lux != ESP_OK) {
            ret_lux = lux_sensor_read(lux);
        }

        // Все датчики успешно?
        if (ret_temp == ESP_OK && ret_co2 == ESP_OK && ret_lux == ESP_OK) {
            break;
        }

        // Ожидание перед повтором
        if (i < max_retries - 1) {
            ESP_LOGW(TAG, "Sensor read failed, retry %d/%d", i + 1, max_retries);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    // Установка дефолтных значений для неудачных чтений
    if (ret_temp != ESP_OK) {
        ESP_LOGW(TAG, "SHT3x read failed - using default values");
        *temp = -127.0f;  // Невалидное значение
        *humidity = 0.0f;
    }
    if (ret_co2 != ESP_OK) {
        ESP_LOGW(TAG, "CCS811 read failed - using default value");
        *co2 = 0;
    }
    if (ret_lux != ESP_OK) {
        ESP_LOGW(TAG, "Lux sensor read failed - using default value");
        *lux = 0;
    }

    // Возвращаем успех если хотя бы один датчик прочитан
    bool at_least_one_ok = (ret_temp == ESP_OK || ret_co2 == ESP_OK || ret_lux == ESP_OK);
    
    if (!at_least_one_ok) {
        ESP_LOGE(TAG, "All sensors failed - no data available");
    }
    
    return at_least_one_ok ? ESP_OK : ESP_FAIL;
}

// Отправка телеметрии на ROOT
static void send_telemetry(float temp, float humidity, uint16_t co2, uint16_t lux) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, telemetry skipped");
        // TODO: local buffer
        return;
    }

    // Создание JSON
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "temp", temp);
    cJSON_AddNumberToObject(data, "humidity", humidity);
    cJSON_AddNumberToObject(data, "co2", co2);
    cJSON_AddNumberToObject(data, "lux", lux);

    char json_buf[512];
    if (mesh_protocol_create_telemetry(s_config->base.node_id, data,
                                        json_buf, sizeof(json_buf))) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Telemetry sent: %.1f°C, %.0f%%, %dppm, %dlux", 
                     temp, humidity, co2, lux);
        } else {
            ESP_LOGW(TAG, "Failed to send telemetry: %s", esp_err_to_name(err));
        }
    }

    cJSON_Delete(data);
}

void climate_controller_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "Command received: %s", command);

    if (strcmp(command, "set_read_interval") == 0) {
        cJSON *interval = cJSON_GetObjectItem(params, "interval_ms");
        if (interval && cJSON_IsNumber(interval)) {
            s_config->read_interval_ms = (uint32_t)interval->valueint;
            node_config_save(s_config, sizeof(climate_node_config_t), "climate_ns");
            ESP_LOGI(TAG, "Read interval updated: %d ms", s_config->read_interval_ms);
        }
    }
    // Другие команды по необходимости
}

void climate_controller_handle_config_update(cJSON *config_json) {
    ESP_LOGI(TAG, "Config update received");

    // Обновление конфигурации
    node_config_update_from_json(s_config, config_json, "climate");
    
    // Сохранение в NVS
    node_config_save(s_config, sizeof(climate_node_config_t), "climate_ns");
    
    ESP_LOGI(TAG, "Config updated and saved");
}

