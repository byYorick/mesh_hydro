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
#include "esp_wifi.h"
#include "esp_system.h"
// #include "esp_task_wdt.h"  // Закомментировано для совместимости с ESP-IDF v5.5
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdbool.h>
#include <time.h>

static const char *TAG = "climate_ctrl";

static climate_node_config_t *s_config = NULL;
static TaskHandle_t s_main_task = NULL;
static TaskHandle_t s_heartbeat_task = NULL;
static bool s_discovery_sent = false;
static uint32_t s_boot_time = 0;

// Forward declarations
static void climate_main_task(void *arg);
static void heartbeat_task(void *arg);
static void send_telemetry(float temp, float humidity, uint16_t co2, uint16_t lux);
static void send_discovery(void);
static void send_heartbeat(void);
static esp_err_t read_all_sensors(float *temp, float *humidity, uint16_t *co2, uint16_t *lux);
static int8_t get_rssi_to_parent(void);

esp_err_t climate_controller_init(climate_node_config_t *config) {
    if (!config) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    s_config = config;
    s_boot_time = (uint32_t)time(NULL);
    s_discovery_sent = false;

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

    // Запуск главной задачи (telemetry каждые 30 сек)
    // Stack увеличен в 2 раза для безопасности: 4096 → 8192
    BaseType_t ret = xTaskCreate(climate_main_task,
                                  "climate_main",
                                  8192,
                                  NULL,
                                  5,
                                  &s_main_task);
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main task");
        return ESP_FAIL;
    }

    // Запуск задачи heartbeat (каждые 60 сек)
    // Stack увеличен в 2 раза для безопасности: 3072 → 6144
    ret = xTaskCreate(heartbeat_task,
                      "heartbeat",
                      6144,
                      NULL,
                      4,
                      &s_heartbeat_task);
    
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Failed to create heartbeat task");
        // Не критично - продолжаем работу
    }

    ESP_LOGI(TAG, "Climate Controller started");
    return ESP_OK;
}

esp_err_t climate_controller_stop(void) {
    if (s_main_task != NULL) {
        vTaskDelete(s_main_task);
        s_main_task = NULL;
    }
    if (s_heartbeat_task != NULL) {
        vTaskDelete(s_heartbeat_task);
        s_heartbeat_task = NULL;
    }
    ESP_LOGI(TAG, "Climate Controller stopped");
    return ESP_OK;
}

// Главная задача чтения датчиков и отправки telemetry
static void climate_main_task(void *arg) {
    ESP_LOGI(TAG, "Main task running (telemetry every 30 sec)");
    
    // Регистрация в watchdog (закомментировано для ESP-IDF v5.5)
    // esp_task_wdt_add(NULL);

    float temp, humidity;
    uint16_t co2, lux;

    // Отправка discovery сообщения при старте
    // Ожидаем подключения к mesh (до 30 секунд)
    int wait_count = 0;
    while (!mesh_manager_is_connected() && wait_count < 30) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        wait_count++;
    }
    
    // Дополнительная задержка после подключения для стабилизации mesh
    if (mesh_manager_is_connected()) {
        ESP_LOGI(TAG, "Mesh connected, waiting 3 seconds for stabilization...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        send_discovery();
        s_discovery_sent = true;
    }

    while (1) {
        // Сброс watchdog (закомментировано для ESP-IDF v5.5)
        // esp_task_wdt_reset();

        // Если mesh подключен, но discovery не отправлен - отправить
        if (mesh_manager_is_connected() && !s_discovery_sent) {
            send_discovery();
            s_discovery_sent = true;
        }

        // Чтение всех датчиков (или моковых значений в MOCK MODE)
        esp_err_t ret = read_all_sensors(&temp, &humidity, &co2, &lux);

        // ⚠️ MOCK MODE: Всегда отправляем телеметрию (даже с моковыми данными)
        if (ret == ESP_OK) {
            // Отправка телеметрии на ROOT
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
        }
        // Убрана проверка "else" которая пропускала телеметрию - теперь всегда отправляется!

        // Интервал telemetry - 5 секунд (DEBUG режим!)
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Задача heartbeat (каждые 5 секунд для DEBUG)
static void heartbeat_task(void *arg) {
    ESP_LOGI(TAG, "Heartbeat task running (every 5 sec - DEBUG mode)");
    
    // Начальная задержка
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1) {
        if (mesh_manager_is_connected()) {
            send_heartbeat();
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Каждые 5 секунд (DEBUG)
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

    // Установка моковых значений для неудачных чтений (MOCK MODE)
    if (ret_temp != ESP_OK) {
        ESP_LOGW(TAG, "⚠️ SHT3x read failed - using MOCK values");
        *temp = 22.5f;     // Моковая температура (вместо -127.0f)
        *humidity = 65.0f; // Моковая влажность (вместо 0.0f)
    }
    if (ret_co2 != ESP_OK) {
        ESP_LOGW(TAG, "⚠️ CCS811 read failed - using MOCK value");
        *co2 = 800;  // Моковое CO2 (вместо 0)
    }
    if (ret_lux != ESP_OK) {
        ESP_LOGW(TAG, "⚠️ Lux sensor read failed - using MOCK value");
        *lux = 500;  // Моковое освещение (вместо 0)
    }

    // ⚠️ MOCK MODE: Всегда возвращаем ESP_OK (даже если все датчики не работают)
    // Телеметрия будет отправлена с моковыми значениями
    bool at_least_one_ok = (ret_temp == ESP_OK || ret_co2 == ESP_OK || ret_lux == ESP_OK);
    
    if (!at_least_one_ok) {
        ESP_LOGW(TAG, "⚠️ All sensors failed - using MOCK data (temp=22.5°C, hum=65%, co2=800ppm, lux=500)");
    }
    
    ESP_LOGD(TAG, "📊 Sensors: %.1f°C, %.0f%%, %dppm, %dlux", *temp, *humidity, *co2, *lux);
    return ESP_OK;  // Всегда успех для отправки телеметрии с моковыми данными
}

// Получение RSSI к родительскому узлу (ROOT)
static int8_t get_rssi_to_parent(void) {
    return mesh_manager_get_parent_rssi();
}

// Отправка discovery сообщения
static void send_discovery(void) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, discovery skipped");
        return;
    }

    // Получение MAC адреса
    uint8_t mac[6];
    mesh_manager_get_mac(mac);

    // Получение данных о системе
    uint32_t heap_free = esp_get_free_heap_size();
    int8_t rssi = get_rssi_to_parent();

    // Создание discovery JSON
    char discovery_msg[512];
    snprintf(discovery_msg, sizeof(discovery_msg),
            "{\"type\":\"discovery\","
            "\"node_id\":\"%s\","
            "\"node_type\":\"climate\","
            "\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
            "\"firmware\":\"1.0.0\","
            "\"hardware\":\"ESP32\","
            "\"sensors\":[\"sht3x\",\"ccs811\",\"lux\"],"
            "\"heap_free\":%lu,"
            "\"wifi_rssi\":%d}",
            s_config->base.node_id,
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
            (unsigned long)heap_free,
            rssi);

    esp_err_t err = mesh_manager_send_to_root((uint8_t *)discovery_msg, strlen(discovery_msg));
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "🔍 Discovery sent: %s (RSSI=%d)", s_config->base.node_id, rssi);
    } else {
        ESP_LOGW(TAG, "Failed to send discovery: %s", esp_err_to_name(err));
    }
}

// Отправка heartbeat
static void send_heartbeat(void) {
    if (!mesh_manager_is_connected()) {
        return;
    }

    // Получение MAC адреса
    uint8_t mac[6];
    mesh_manager_get_mac(mac);

    uint32_t uptime = (uint32_t)time(NULL) - s_boot_time;
    uint32_t heap_free = esp_get_free_heap_size();
    int8_t rssi = get_rssi_to_parent();

    // Создание heartbeat JSON с MAC адресом
    char heartbeat_msg[384];
    snprintf(heartbeat_msg, sizeof(heartbeat_msg),
            "{\"type\":\"heartbeat\","
            "\"node_id\":\"%s\","
            "\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
            "\"uptime\":%lu,"
            "\"heap_free\":%lu,"
            "\"rssi_to_parent\":%d}",
            s_config->base.node_id,
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
            (unsigned long)uptime,
            (unsigned long)heap_free,
            rssi);

    esp_err_t err = mesh_manager_send_to_root((uint8_t *)heartbeat_msg, strlen(heartbeat_msg));
    
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "💓 Heartbeat sent (uptime=%lus, heap=%luB, RSSI=%d)", 
                 (unsigned long)uptime, (unsigned long)heap_free, rssi);
    }
}

// Отправка телеметрии на ROOT с RSSI
static void send_telemetry(float temp, float humidity, uint16_t co2, uint16_t lux) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, telemetry skipped");
        // TODO: local buffer
        return;
    }

    // Получение RSSI к родительскому узлу
    int8_t rssi = get_rssi_to_parent();

    // Создание JSON
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "temperature", temp);
    cJSON_AddNumberToObject(data, "humidity", humidity);
    cJSON_AddNumberToObject(data, "co2", co2);
    cJSON_AddNumberToObject(data, "lux", lux);
    cJSON_AddNumberToObject(data, "rssi_to_parent", rssi);

    char json_buf[512];
    if (mesh_protocol_create_telemetry(s_config->base.node_id, data,
                                        json_buf, sizeof(json_buf))) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "📊 Telemetry: %.1f°C, %.0f%%, %dppm, %dlux, RSSI=%d", 
                     temp, humidity, co2, lux, rssi);
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

