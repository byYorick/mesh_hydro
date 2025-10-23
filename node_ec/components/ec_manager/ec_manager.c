/**
 * @file ec_manager.c
 * @brief EC Manager implementation
 */

#include "ec_manager.h"
#include "ec_sensor.h"
#include "pump_controller.h"
#include "pid_controller.h"
#include "adaptive_pid.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include <string.h>
#include <time.h>
#include <math.h>

static const char *TAG = "ec_mgr";

// Состояние менеджера
static ec_node_config_t *s_config = NULL;
static TaskHandle_t s_main_task = NULL;
static TaskHandle_t s_heartbeat_task = NULL;
static bool s_discovery_sent = false;
static uint32_t s_boot_time = 0;
static bool s_emergency_mode = false;
static bool s_autonomous_mode = false;

// Адаптивный PID контроллер (один для всех насосов EC)
static adaptive_pid_t s_pid_ec;

// Текущее значение
static float s_current_ec = 2.0f;

// Forward declarations
static void main_task(void *arg);
static void heartbeat_task(void *arg);
static void send_discovery(void);
static void send_telemetry(void);
static void send_heartbeat(void);
static int8_t get_rssi_to_parent(void);
static void read_sensor(void);
static void control_ec(void);
static void check_emergency_conditions(void);

esp_err_t ec_manager_init(ec_node_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    s_config = config;
    s_boot_time = (uint32_t)time(NULL);
    s_discovery_sent = false;
    s_emergency_mode = false;
    s_autonomous_mode = false;
    
    // Инициализация адаптивного PID контроллера для EC
    adaptive_pid_init(&s_pid_ec, s_config->ec_target,
                     s_config->pump_pid[PUMP_EC_A].kp, 
                     s_config->pump_pid[PUMP_EC_A].ki, 
                     s_config->pump_pid[PUMP_EC_A].kd);
    
    // Настройка зон для EC (dead=0.2, close=0.5, far=1.5)
    adaptive_pid_set_zones(&s_pid_ec, 0.2f, 0.5f, 1.5f);
    
    // Safety: макс 10 мл за раз, минимум 60 сек между дозами
    adaptive_pid_set_safety(&s_pid_ec, 10.0f, 60000);
    
    // Лимиты выхода
    adaptive_pid_set_output_limits(&s_pid_ec, 0.0f, 10.0f);
    
    ESP_LOGI(TAG, "EC Manager initialized");
    ESP_LOGI(TAG, "Node ID: %s, EC target: %.2f", 
             s_config->base.node_id, s_config->ec_target);
    
    return ESP_OK;
}

esp_err_t ec_manager_start(void) {
    if (s_main_task != NULL) {
        ESP_LOGW(TAG, "Already running");
        return ESP_OK;
    }
    
    // Запуск главной задачи
    BaseType_t ret = xTaskCreate(main_task, "EC_main", 6144, NULL, 5, &s_main_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main task");
        return ESP_FAIL;
    }
    
    // Запуск heartbeat задачи
    ret = xTaskCreate(heartbeat_task, "heartbeat", 3072, NULL, 4, &s_heartbeat_task);
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Failed to create heartbeat task");
    }
    
    ESP_LOGI(TAG, "EC Manager started");
    return ESP_OK;
}

esp_err_t ec_manager_stop(void) {
    if (s_main_task != NULL) {
        vTaskDelete(s_main_task);
        s_main_task = NULL;
    }
    if (s_heartbeat_task != NULL) {
        vTaskDelete(s_heartbeat_task);
        s_heartbeat_task = NULL;
    }
    
    // Остановка всех насосов
    pump_controller_emergency_stop();
    
    ESP_LOGI(TAG, "EC Manager stopped");
    return ESP_OK;
}

esp_err_t ec_manager_get_value(float *EC) {
    if (EC) *EC = s_current_ec;
    return ESP_OK;
}

esp_err_t ec_manager_set_emergency(bool enable) {
    s_emergency_mode = enable;
    
    if (enable) {
        ESP_LOGW(TAG, "EMERGENCY MODE ACTIVATED");
        pump_controller_emergency_stop();
    } else {
        ESP_LOGI(TAG, "Emergency mode deactivated");
    }
    
    return ESP_OK;
}

// Главная задача управления
static void main_task(void *arg) {
    ESP_LOGI(TAG, "Main task started");
    
    // Ждём подключения к mesh
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Отправка discovery при старте
    send_discovery();
    s_discovery_sent = true;
    
    TickType_t last_telemetry = 0;
    TickType_t last_control = 0;
    
    while (1) {
        TickType_t now = xTaskGetTickCount();
        
        // Чтение датчика каждые 10 секунд
        if (now - last_control >= pdMS_TO_TICKS(10000)) {
            read_sensor();
            check_emergency_conditions();
            
            if (!s_emergency_mode) {
                control_ec();
            }
            
            last_control = now;
        }
        
        // Отправка telemetry каждые 30 секунд
        if (now - last_telemetry >= pdMS_TO_TICKS(30000)) {
            send_telemetry();
            last_telemetry = now;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Heartbeat задача
static void heartbeat_task(void *arg) {
    ESP_LOGI(TAG, "Heartbeat task started");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000)); // Каждую минуту
        send_heartbeat();
    }
}

// Отправка discovery сообщения
static void send_discovery(void) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Cannot send discovery - mesh not connected");
        return;
    }
    
    // Создание JSON
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON");
        return;
    }
    
    cJSON_AddStringToObject(root, "type", "discovery");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "ec");
    
    // Sensors
    cJSON *sensors = cJSON_CreateArray();
    if (sensors) {
        cJSON_AddItemToArray(sensors, cJSON_CreateString("ec"));
        cJSON_AddItemToObject(root, "sensors", sensors);
    }
    
    // Actuators
    cJSON *actuators = cJSON_CreateArray();
    if (actuators) {
        cJSON_AddItemToArray(actuators, cJSON_CreateString("pump_ec_a"));
        cJSON_AddItemToArray(actuators, cJSON_CreateString("pump_ec_b"));
        cJSON_AddItemToArray(actuators, cJSON_CreateString("pump_ec_c"));
        cJSON_AddItemToObject(root, "actuators", actuators);
    }
    
    // System info
    cJSON_AddNumberToObject(root, "heap_free", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "wifi_rssi", get_rssi_to_parent());
    
    // Получение MAC адреса
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(root, "mac", mac_str);
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        ESP_LOGI(TAG, "Discovery sent: %s", json_str);
        free(json_str);
    }
    
    cJSON_Delete(root);
}

// Отправка telemetry
static void send_telemetry(void) {
    if (!mesh_manager_is_connected()) {
        s_autonomous_mode = true;
        return;
    }
    
    s_autonomous_mode = false;
    
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }
    
    cJSON_AddStringToObject(root, "type", "telemetry");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "ec");  // ВАЖНО: тип узла для backend
    
    cJSON *data = cJSON_CreateObject();
    if (!data) {
        ESP_LOGE(TAG, "Failed to create data JSON object");
        cJSON_Delete(root);
        return;
    }
    
    cJSON_AddNumberToObject(data, "ec", s_current_ec);
    cJSON_AddNumberToObject(data, "ec_target", s_config->ec_target);
    cJSON_AddNumberToObject(data, "pump_ec_a_ml", pump_controller_get_total_ml(PUMP_EC_A));
    cJSON_AddNumberToObject(data, "pump_ec_b_ml", pump_controller_get_total_ml(PUMP_EC_B));
    cJSON_AddNumberToObject(data, "pump_ec_c_ml", pump_controller_get_total_ml(PUMP_EC_C));
    cJSON_AddNumberToObject(data, "rssi_to_parent", get_rssi_to_parent());
    cJSON_AddBoolToObject(data, "emergency", s_emergency_mode);
    cJSON_AddBoolToObject(data, "autonomous", s_autonomous_mode);
    cJSON_AddItemToObject(root, "data", data);
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        ESP_LOGD(TAG, "Telemetry sent");
        free(json_str);
    }
    
    cJSON_Delete(root);
}

// Отправка heartbeat
static void send_heartbeat(void) {
    if (!mesh_manager_is_connected()) {
        return;
    }
    
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }
    
    cJSON_AddStringToObject(root, "type", "heartbeat");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "ec");  // ВАЖНО: тип узла для backend
    cJSON_AddNumberToObject(root, "uptime", (uint32_t)time(NULL) - s_boot_time);
    cJSON_AddNumberToObject(root, "heap_free", esp_get_free_heap_size());
    cJSON_AddBoolToObject(root, "autonomous", s_autonomous_mode);
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        free(json_str);
    }
    
    cJSON_Delete(root);
}

// Чтение датчика
static void read_sensor(void) {
    float EC;
    esp_err_t ret = ec_sensor_read(&EC);
    
    if (ret == ESP_OK) {
        s_current_ec = EC + s_config->ec_cal_offset;
        ESP_LOGI(TAG, "EC: %.2f (target: %.2f)", s_current_ec, s_config->ec_target);
    } else {
        ESP_LOGW(TAG, "Failed to read EC sensor, using last value: %.2f", s_current_ec);
    }
}

// Управление EC (распределение по 3 насосам)
static void control_ec(void) {
    float output = 0.0f;
    esp_err_t err;
    
    // Если EC < target - нужно повысить
    if (s_current_ec < s_config->ec_target) {
        err = adaptive_pid_compute(&s_pid_ec, s_current_ec, 10.0f, &output);
        
        if (err == ESP_OK && output > 0.0f) {
            // Распределение по 3 насосам: A = 50%, B = 40%, C = 10%
            float dose_a = output * 0.5f;
            float dose_b = output * 0.4f;
            float dose_c = output * 0.1f;
            
            pid_zone_t zone = adaptive_pid_get_zone(&s_pid_ec);
            ESP_LOGI(TAG, "EC: %.2f ml [%s zone] (A:%.2f B:%.2f C:%.2f) current=%.2f target=%.2f", 
                     output, adaptive_pid_zone_to_str(zone), dose_a, dose_b, dose_c, 
                     s_current_ec, s_config->ec_target);
            
            // Запуск всех 3 насосов последовательно
            pump_controller_run_dose(PUMP_EC_A, dose_a);
            vTaskDelay(pdMS_TO_TICKS(100)); // Небольшая задержка между насосами
            pump_controller_run_dose(PUMP_EC_B, dose_b);
            vTaskDelay(pdMS_TO_TICKS(100));
            pump_controller_run_dose(PUMP_EC_C, dose_c);
            
            // Отправка события при коррекции в FAR зоне
            if (zone == ZONE_FAR) {
                send_event(MESH_EVENT_WARNING, "EC far from target, aggressive correction", s_current_ec);
            }
        }
    }
    // Если EC > target - только логирование (EC не понижается насосами)
    else {
        ESP_LOGD(TAG, "EC above target (%.2f > %.2f), waiting for dilution", 
                 s_current_ec, s_config->ec_target);
    }
}

// Отправка event сообщения
static void send_event(mesh_event_level_t level, const char *message, float value) {
    if (!mesh_manager_is_connected()) {
        return;  // Нельзя отправить если offline
    }
    
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }
    
    cJSON_AddStringToObject(root, "type", "event");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "ec");
    cJSON_AddStringToObject(root, "level", mesh_protocol_event_level_to_str(level));
    cJSON_AddStringToObject(root, "message", message);
    cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "ec", value);
    cJSON_AddNumberToObject(data, "ec_target", s_config->ec_target);
    cJSON_AddNumberToObject(data, "ec_min", s_config->ec_min);
    cJSON_AddNumberToObject(data, "ec_max", s_config->ec_max);
    cJSON_AddItemToObject(root, "data", data);
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        ESP_LOGI(TAG, "Event sent: %s - %s", mesh_protocol_event_level_to_str(level), message);
        free(json_str);
    }
    
    cJSON_Delete(root);
}

// Проверка аварийных условий
static void check_emergency_conditions(void) {
    // Emergency если EC слишком низкий или высокий
    if (s_current_ec < s_config->ec_min || s_current_ec > s_config->ec_max) {
        if (!s_emergency_mode) {
            ESP_LOGE(TAG, "EMERGENCY: EC out of range (%.2f)", s_current_ec);
            ec_manager_set_emergency(true);
            
            // Отправка критичного события
            if (s_current_ec < s_config->ec_min) {
                send_event(MESH_EVENT_CRITICAL, "EC too low", s_current_ec);
            } else {
                send_event(MESH_EVENT_CRITICAL, "EC too high", s_current_ec);
            }
        }
    } else {
        if (s_emergency_mode) {
            ESP_LOGI(TAG, "EC back to normal range");
            ec_manager_set_emergency(false);
            
            // Отправка события восстановления
            send_event(MESH_EVENT_INFO, "EC back to normal", s_current_ec);
        }
    }
}

// Получение RSSI
static int8_t get_rssi_to_parent(void) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return -100;
}

// Обработка команд
void ec_manager_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "Command received: %s", command);
    
    if (strcmp(command, "set_ec_target") == 0) {
        cJSON *target = cJSON_GetObjectItem(params, "target");
        if (cJSON_IsNumber(target)) {
            float new_target = (float)target->valuedouble;
            
            // Валидация диапазона
            if (new_target < 0.5f || new_target > 5.0f) {
                ESP_LOGW(TAG, "Invalid EC target: %.2f (must be 0.5-5.0)", new_target);
                return;
            }
            
            s_config->ec_target = new_target;
            adaptive_pid_set_setpoint(&s_pid_ec, s_config->ec_target);
            
            // ВАЖНО: Сохранение в NVS!
            esp_err_t err = node_config_save(s_config, sizeof(ec_node_config_t), "ec_ns");
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "EC target set to %.2f and saved to NVS", s_config->ec_target);
            } else {
                ESP_LOGE(TAG, "Failed to save EC target to NVS: %s", esp_err_to_name(err));
            }
        }
    }
    else if (strcmp(command, "emergency_stop") == 0) {
        ec_manager_set_emergency(true);
    }
    else if (strcmp(command, "reset_emergency") == 0) {
        ec_manager_set_emergency(false);
    }
    else if (strcmp(command, "run_pump") == 0) {
        cJSON *pump_id = cJSON_GetObjectItem(params, "pump_id");
        cJSON *duration = cJSON_GetObjectItem(params, "duration_ms");
        
        if (cJSON_IsNumber(pump_id) && cJSON_IsNumber(duration)) {
            pump_id_t pump = (pump_id_t)pump_id->valueint;
            uint32_t dur = (uint32_t)duration->valueint;
            
            if (pump < PUMP_MAX && dur > 0 && dur <= 10000) {
                ESP_LOGI(TAG, "Manual pump run: %d for %lu ms", pump, dur);
                pump_controller_run(pump, dur);
            }
        }
    }
    else if (strcmp(command, "reset_stats") == 0) {
        pump_controller_reset_stats(PUMP_EC_A);
        pump_controller_reset_stats(PUMP_EC_B);
        pump_controller_reset_stats(PUMP_EC_C);
        ESP_LOGI(TAG, "Pump stats reset");
    }
    else if (strcmp(command, "run_pump_manual") == 0) {
        cJSON *pump_id = cJSON_GetObjectItem(params, "pump_id");
        cJSON *duration_sec = cJSON_GetObjectItem(params, "duration_sec");
        
        if (cJSON_IsNumber(pump_id) && cJSON_IsNumber(duration_sec)) {
            int pump = pump_id->valueint;
            float duration = (float)duration_sec->valuedouble;
            
            // Валидация
            if (pump < 0 || pump > 2) {
                ESP_LOGW(TAG, "Invalid pump_id: %d (must be 0-2)", pump);
                return;
            }
            if (duration <= 0.0f || duration > 30.0f) {
                ESP_LOGW(TAG, "Invalid duration: %.1f sec (must be 0.1-30.0)", duration);
                return;
            }
            
            // Запуск насоса вручную
            pump_id_t pid = (pump == 0) ? PUMP_EC_A : (pump == 1) ? PUMP_EC_B : PUMP_EC_C;
            uint32_t duration_ms = (uint32_t)(duration * 1000.0f);
            
            ESP_LOGI(TAG, "Manual pump run: PUMP_%c for %.1f sec", 
                     (pump == 0) ? 'A' : (pump == 1) ? 'B' : 'C', duration);
            pump_controller_run(pid, duration_ms);
        }
    }
    else if (strcmp(command, "calibrate_pump") == 0) {
        cJSON *pump_id = cJSON_GetObjectItem(params, "pump_id");
        cJSON *duration_sec = cJSON_GetObjectItem(params, "duration_sec");
        cJSON *volume_ml = cJSON_GetObjectItem(params, "volume_ml");
        
        if (cJSON_IsNumber(pump_id) && cJSON_IsNumber(duration_sec) && cJSON_IsNumber(volume_ml)) {
            int pump = pump_id->valueint;
            float duration = (float)duration_sec->valuedouble;
            float volume = (float)volume_ml->valuedouble;
            
            // Валидация
            if (pump < 0 || pump > 2) {
                ESP_LOGW(TAG, "Invalid pump_id: %d", pump);
                return;
            }
            if (duration <= 0.0f || volume <= 0.0f) {
                ESP_LOGW(TAG, "Invalid calibration params");
                return;
            }
            
            // Сохранение калибровки
            s_config->pump_calibration[pump].calibration_time_ms = (uint32_t)(duration * 1000.0f);
            s_config->pump_calibration[pump].calibration_volume_ml = volume;
            s_config->pump_calibration[pump].ml_per_second = volume / duration;
            s_config->pump_calibration[pump].is_calibrated = true;
            s_config->pump_calibration[pump].last_calibrated = (uint64_t)time(NULL);
            
            // Сохранение в NVS
            esp_err_t err = node_config_save(s_config, sizeof(ec_node_config_t), "ec_ns");
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Pump %d calibrated: %.2f ml/sec (%.1f ml in %.1f sec)", 
                         pump, s_config->pump_calibration[pump].ml_per_second, volume, duration);
            } else {
                ESP_LOGE(TAG, "Failed to save calibration to NVS");
            }
        }
    }
    else if (strcmp(command, "get_config") == 0) {
        // Проверка подключения к mesh
        if (!mesh_manager_is_connected()) {
            ESP_LOGW(TAG, "Cannot send config: mesh offline");
            return;
        }
        
        // Создание корневого объекта
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            ESP_LOGE(TAG, "Failed to create root JSON object");
            return;
        }
        
        cJSON_AddStringToObject(root, "type", "config_response");
        cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
        cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
        
        // Создание объекта конфигурации
        cJSON *config = cJSON_CreateObject();
        if (!config) {
            ESP_LOGE(TAG, "Failed to create config JSON object");
            cJSON_Delete(root);
            return;
        }
        
        cJSON_AddStringToObject(config, "node_id", s_config->base.node_id);
        cJSON_AddStringToObject(config, "node_type", "ec");
        cJSON_AddStringToObject(config, "zone", s_config->base.zone);
        
        // EC параметры
        cJSON_AddNumberToObject(config, "ec_target", s_config->ec_target);
        cJSON_AddNumberToObject(config, "ec_min", s_config->ec_min);
        cJSON_AddNumberToObject(config, "ec_max", s_config->ec_max);
        cJSON_AddNumberToObject(config, "ec_cal_offset", s_config->ec_cal_offset);
        
        // Калибровка насосов
        cJSON *pumps_cal = cJSON_CreateArray();
        if (pumps_cal) {
            for (int i = 0; i < 3; i++) {
                cJSON *pump = cJSON_CreateObject();
                if (pump) {
                    cJSON_AddNumberToObject(pump, "pump_id", i);
                    cJSON_AddNumberToObject(pump, "ml_per_second", s_config->pump_calibration[i].ml_per_second);
                    cJSON_AddNumberToObject(pump, "calibration_volume_ml", s_config->pump_calibration[i].calibration_volume_ml);
                    cJSON_AddNumberToObject(pump, "calibration_time_ms", s_config->pump_calibration[i].calibration_time_ms);
                    cJSON_AddBoolToObject(pump, "is_calibrated", s_config->pump_calibration[i].is_calibrated);
                    cJSON_AddNumberToObject(pump, "last_calibrated", (double)s_config->pump_calibration[i].last_calibrated);
                    cJSON_AddItemToArray(pumps_cal, pump);
                }
            }
            cJSON_AddItemToObject(config, "pumps_calibration", pumps_cal);
        }
        
        // PID параметры
        cJSON *pumps_pid = cJSON_CreateArray();
        if (pumps_pid) {
            for (int i = 0; i < 3; i++) {
                cJSON *pid = cJSON_CreateObject();
                if (pid) {
                    cJSON_AddNumberToObject(pid, "pump_id", i);
                    cJSON_AddNumberToObject(pid, "kp", s_config->pump_pid[i].kp);
                    cJSON_AddNumberToObject(pid, "ki", s_config->pump_pid[i].ki);
                    cJSON_AddNumberToObject(pid, "kd", s_config->pump_pid[i].kd);
                    cJSON_AddBoolToObject(pid, "enabled", s_config->pump_pid[i].enabled);
                    cJSON_AddItemToArray(pumps_pid, pid);
                }
            }
            cJSON_AddItemToObject(config, "pumps_pid", pumps_pid);
        }
        
        // Safety
        cJSON_AddNumberToObject(config, "max_pump_time_ms", s_config->max_pump_time_ms);
        cJSON_AddNumberToObject(config, "cooldown_ms", s_config->cooldown_ms);
        cJSON_AddBoolToObject(config, "autonomous_enabled", s_config->autonomous_enabled);
        
        // Добавление config к root
        cJSON_AddItemToObject(root, "config", config);
        
        // Отправка конфигурации
        char *json_str = cJSON_PrintUnformatted(root);
        if (json_str) {
            esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Config sent to ROOT (%d bytes)", strlen(json_str));
            } else {
                ESP_LOGE(TAG, "Failed to send config: %s", esp_err_to_name(err));
            }
            free(json_str);
        } else {
            ESP_LOGE(TAG, "Failed to serialize config JSON");
        }
        
        // Всегда освобождаем память
        cJSON_Delete(root);
    }
    else {
        ESP_LOGW(TAG, "Unknown command: %s", command);
    }
}

// Обновление конфигурации
void ec_manager_handle_config_update(cJSON *config_json) {
    ESP_LOGI(TAG, "Config update received");
    
    bool config_changed = false;
    
    cJSON *ec_target = cJSON_GetObjectItem(config_json, "ec_target");
    if (cJSON_IsNumber(ec_target)) {
        float new_target = (float)ec_target->valuedouble;
        if (new_target >= 0.5f && new_target <= 5.0f) {
            s_config->ec_target = new_target;
            adaptive_pid_set_setpoint(&s_pid_ec, s_config->ec_target);
            config_changed = true;
            ESP_LOGI(TAG, "EC target updated: %.2f", s_config->ec_target);
        }
    }
    
    cJSON *ec_min = cJSON_GetObjectItem(config_json, "ec_min");
    if (cJSON_IsNumber(ec_min)) {
        float new_min = (float)ec_min->valuedouble;
        if (new_min >= 0.0f && new_min <= 4.0f) {
            s_config->ec_min = new_min;
            config_changed = true;
            ESP_LOGI(TAG, "EC min updated: %.2f", s_config->ec_min);
        }
    }
    
    cJSON *ec_max = cJSON_GetObjectItem(config_json, "ec_max");
    if (cJSON_IsNumber(ec_max)) {
        float new_max = (float)ec_max->valuedouble;
        if (new_max >= 1.0f && new_max <= 6.0f) {
            s_config->ec_max = new_max;
            config_changed = true;
            ESP_LOGI(TAG, "EC max updated: %.2f", s_config->ec_max);
        }
    }
    
    // PID параметры
    cJSON *pid_params = cJSON_GetObjectItem(config_json, "pid_params");
    if (cJSON_IsObject(pid_params)) {
        cJSON *kp = cJSON_GetObjectItem(pid_params, "kp");
        cJSON *ki = cJSON_GetObjectItem(pid_params, "ki");
        cJSON *kd = cJSON_GetObjectItem(pid_params, "kd");
        
        if (cJSON_IsNumber(kp) && cJSON_IsNumber(ki) && cJSON_IsNumber(kd)) {
            s_config->pump_pid[0].kp = (float)kp->valuedouble;
            s_config->pump_pid[0].ki = (float)ki->valuedouble;
            s_config->pump_pid[0].kd = (float)kd->valuedouble;
            s_config->pump_pid[1].kp = (float)kp->valuedouble;
            s_config->pump_pid[1].ki = (float)ki->valuedouble;
            s_config->pump_pid[1].kd = (float)kd->valuedouble;
            s_config->pump_pid[2].kp = (float)kp->valuedouble;
            s_config->pump_pid[2].ki = (float)ki->valuedouble;
            s_config->pump_pid[2].kd = (float)kd->valuedouble;
            
            // Переинициализация PID
            pid_init(&s_pid_ec, s_config->pump_pid[0].kp, 
                     s_config->pump_pid[0].ki, s_config->pump_pid[0].kd);
            
            config_changed = true;
            ESP_LOGI(TAG, "PID params updated: Kp=%.2f Ki=%.2f Kd=%.2f", 
                     (float)kp->valuedouble, (float)ki->valuedouble, (float)kd->valuedouble);
        }
    }
    
    // ВАЖНО: Сохранение в NVS
    if (config_changed) {
        esp_err_t err = node_config_save(s_config, sizeof(ec_node_config_t), "ec_ns");
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Configuration saved to NVS");
        } else {
            ESP_LOGE(TAG, "Failed to save config to NVS: %s", esp_err_to_name(err));
        }
    }
}

