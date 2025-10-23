/**
 * @file ph_manager.c
 * @brief pH Manager implementation
 */

#include "ph_manager.h"
#include "ph_sensor.h"
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

static const char *TAG = "ph_mgr";

// Состояние менеджера
static ph_node_config_t *s_config = NULL;
static TaskHandle_t s_main_task = NULL;
static TaskHandle_t s_heartbeat_task = NULL;
static bool s_discovery_sent = false;
static uint32_t s_boot_time = 0;
static bool s_emergency_mode = false;
static bool s_autonomous_mode = false;

// Адаптивные PID контроллеры
static adaptive_pid_t s_pid_ph_up;
static adaptive_pid_t s_pid_ph_down;

// Текущее значение
static float s_current_ph = 7.0f;

// Forward declarations
static void main_task(void *arg);
static void heartbeat_task(void *arg);
static void send_discovery(void);
static void send_telemetry(void);
static void send_heartbeat(void);
static void send_event(mesh_event_level_t level, const char *message, float value);
static int8_t get_rssi_to_parent(void);
static void read_sensor(void);
static void control_ph(void);
static void check_emergency_conditions(void);

esp_err_t ph_manager_init(ph_node_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    s_config = config;
    s_boot_time = (uint32_t)time(NULL);
    s_discovery_sent = false;
    s_emergency_mode = false;
    s_autonomous_mode = false;
    
    // Инициализация адаптивных PID контроллеров
    adaptive_pid_init(&s_pid_ph_up, s_config->ph_target,
                     s_config->pump_pid[PUMP_PH_UP].kp, 
                     s_config->pump_pid[PUMP_PH_UP].ki, 
                     s_config->pump_pid[PUMP_PH_UP].kd);
    
    // Настройка зон для pH (dead=0.1, close=0.3, far=1.0)
    adaptive_pid_set_zones(&s_pid_ph_up, 0.1f, 0.3f, 1.0f);
    
    // Safety: макс 5 мл за раз, минимум 60 сек между дозами
    adaptive_pid_set_safety(&s_pid_ph_up, 5.0f, 60000);
    
    // Лимиты выхода
    adaptive_pid_set_output_limits(&s_pid_ph_up, 0.0f, 5.0f);
    
    // pH DOWN контроллер
    adaptive_pid_init(&s_pid_ph_down, s_config->ph_target,
                     s_config->pump_pid[PUMP_PH_DOWN].kp,
                     s_config->pump_pid[PUMP_PH_DOWN].ki,
                     s_config->pump_pid[PUMP_PH_DOWN].kd);
    
    adaptive_pid_set_zones(&s_pid_ph_down, 0.1f, 0.3f, 1.0f);
    adaptive_pid_set_safety(&s_pid_ph_down, 5.0f, 60000);
    adaptive_pid_set_output_limits(&s_pid_ph_down, 0.0f, 5.0f);
    
    ESP_LOGI(TAG, "pH Manager initialized");
    ESP_LOGI(TAG, "Node ID: %s, pH target: %.2f", 
             s_config->base.node_id, s_config->ph_target);
    
    return ESP_OK;
}

esp_err_t ph_manager_start(void) {
    if (s_main_task != NULL) {
        ESP_LOGW(TAG, "Already running");
        return ESP_OK;
    }
    
    // Запуск главной задачи
    BaseType_t ret = xTaskCreate(main_task, "ph_main", 6144, NULL, 5, &s_main_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main task");
        return ESP_FAIL;
    }
    
    // Запуск heartbeat задачи
    ret = xTaskCreate(heartbeat_task, "heartbeat", 3072, NULL, 4, &s_heartbeat_task);
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Failed to create heartbeat task");
    }
    
    ESP_LOGI(TAG, "pH Manager started");
    return ESP_OK;
}

esp_err_t ph_manager_stop(void) {
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
    
    ESP_LOGI(TAG, "pH Manager stopped");
    return ESP_OK;
}

esp_err_t ph_manager_get_value(float *ph) {
    if (ph) *ph = s_current_ph;
    return ESP_OK;
}

esp_err_t ph_manager_set_emergency(bool enable) {
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
                control_ph();
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
    cJSON_AddStringToObject(root, "node_type", "ph");
    
    // Sensors
    cJSON *sensors = cJSON_CreateArray();
    if (sensors) {
        cJSON_AddItemToArray(sensors, cJSON_CreateString("ph"));
        cJSON_AddItemToObject(root, "sensors", sensors);
    }
    
    // Actuators
    cJSON *actuators = cJSON_CreateArray();
    if (actuators) {
        cJSON_AddItemToArray(actuators, cJSON_CreateString("pump_ph_up"));
        cJSON_AddItemToArray(actuators, cJSON_CreateString("pump_ph_down"));
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
    cJSON_AddStringToObject(root, "node_type", "ph");  // ВАЖНО: тип узла для backend
    
    cJSON *data = cJSON_CreateObject();
    if (!data) {
        ESP_LOGE(TAG, "Failed to create data JSON object");
        cJSON_Delete(root);
        return;
    }
    
    cJSON_AddNumberToObject(data, "ph", s_current_ph);
    cJSON_AddNumberToObject(data, "ph_target", s_config->ph_target);
    cJSON_AddNumberToObject(data, "pump_ph_up_ml", pump_controller_get_total_ml(PUMP_PH_UP));
    cJSON_AddNumberToObject(data, "pump_ph_down_ml", pump_controller_get_total_ml(PUMP_PH_DOWN));
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
    cJSON_AddStringToObject(root, "node_type", "ph");  // ВАЖНО: тип узла для backend
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
    float ph;
    esp_err_t ret = ph_sensor_read(&ph);
    
    if (ret == ESP_OK) {
        // Дополнительная валидация в менеджере
        if (ph < 0.0f || ph > 14.0f) {
            ESP_LOGE(TAG, "CRITICAL: Invalid pH from sensor: %.2f - using last value", ph);
            ESP_LOGW(TAG, "Current pH: %.2f (last valid)", s_current_ph);
        } else {
            s_current_ph = ph + s_config->ph_cal_offset;
            ESP_LOGI(TAG, "pH: %.2f (target: %.2f, mock=%s)", 
                     s_current_ph, s_config->ph_target, 
                     ph_sensor_is_mock_mode() ? "YES" : "NO");
        }
    } else {
        ESP_LOGW(TAG, "Failed to read pH sensor, using last value: %.2f", s_current_ph);
    }
}

// Управление pH
static void control_ph(void) {
    float output = 0.0f;
    esp_err_t err;
    
    // Если pH < target - нужно повысить (pH UP)
    if (s_current_ph < s_config->ph_target) {
        err = adaptive_pid_compute(&s_pid_ph_up, s_current_ph, 10.0f, &output);
        
        if (err == ESP_OK && output > 0.0f) {
            pid_zone_t zone = adaptive_pid_get_zone(&s_pid_ph_up);
            ESP_LOGI(TAG, "pH UP: %.2f ml [%s zone] (current=%.2f, target=%.2f)", 
                     output, adaptive_pid_zone_to_str(zone), s_current_ph, s_config->ph_target);
            pump_controller_run_dose(PUMP_PH_UP, output);
            
            // Отправка события при коррекции в FAR зоне
            if (zone == ZONE_FAR) {
                send_event(MESH_EVENT_WARNING, "pH far from target, aggressive correction", s_current_ph);
            }
        }
    }
    // Если pH > target - нужно понизить (pH DOWN)
    else if (s_current_ph > s_config->ph_target) {
        err = adaptive_pid_compute(&s_pid_ph_down, s_current_ph, 10.0f, &output);
        
        if (err == ESP_OK && output > 0.0f) {
            pid_zone_t zone = adaptive_pid_get_zone(&s_pid_ph_down);
            ESP_LOGI(TAG, "pH DOWN: %.2f ml [%s zone] (current=%.2f, target=%.2f)", 
                     output, adaptive_pid_zone_to_str(zone), s_current_ph, s_config->ph_target);
            pump_controller_run_dose(PUMP_PH_DOWN, output);
            
            // Отправка события при коррекции в FAR зоне
            if (zone == ZONE_FAR) {
                send_event(MESH_EVENT_WARNING, "pH far from target, aggressive correction", s_current_ph);
            }
        }
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
    cJSON_AddStringToObject(root, "node_type", "ph");
    cJSON_AddStringToObject(root, "level", mesh_protocol_event_level_to_str(level));
    cJSON_AddStringToObject(root, "message", message);
    cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "ph", value);
    cJSON_AddNumberToObject(data, "ph_target", s_config->ph_target);
    cJSON_AddNumberToObject(data, "ph_min", s_config->ph_min);
    cJSON_AddNumberToObject(data, "ph_max", s_config->ph_max);
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
    // Emergency если pH слишком низкий или высокий
    if (s_current_ph < s_config->ph_min || s_current_ph > s_config->ph_max) {
        if (!s_emergency_mode) {
            ESP_LOGE(TAG, "EMERGENCY: pH out of range (%.2f)", s_current_ph);
            ph_manager_set_emergency(true);
            
            // Отправка критичного события
            if (s_current_ph < s_config->ph_min) {
                send_event(MESH_EVENT_CRITICAL, "pH too low", s_current_ph);
            } else {
                send_event(MESH_EVENT_CRITICAL, "pH too high", s_current_ph);
            }
        }
    } else {
        if (s_emergency_mode) {
            ESP_LOGI(TAG, "pH back to normal range");
            ph_manager_set_emergency(false);
            
            // Отправка события восстановления
            send_event(MESH_EVENT_INFO, "pH back to normal", s_current_ph);
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
void ph_manager_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "=== PH_MANAGER_HANDLE_COMMAND ===");
    ESP_LOGI(TAG, "Command received: %s", command ? command : "NULL");
    ESP_LOGI(TAG, "Params: %s", params ? "found" : "NULL");
    
    if (strcmp(command, "set_ph_target") == 0) {
        cJSON *target = cJSON_GetObjectItem(params, "target");
        if (cJSON_IsNumber(target)) {
            float new_target = (float)target->valuedouble;
            
            // Валидация диапазона
            if (new_target < 5.0f || new_target > 9.0f) {
                ESP_LOGW(TAG, "Invalid pH target: %.2f (must be 5.0-9.0)", new_target);
                return;
            }
            
            s_config->ph_target = new_target;
            adaptive_pid_set_setpoint(&s_pid_ph_up, s_config->ph_target);
            adaptive_pid_set_setpoint(&s_pid_ph_down, s_config->ph_target);
            
            // ВАЖНО: Сохранение в NVS!
            esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "pH target set to %.2f and saved to NVS", s_config->ph_target);
            } else {
                ESP_LOGE(TAG, "Failed to save pH target to NVS: %s", esp_err_to_name(err));
            }
        }
    }
    else if (strcmp(command, "emergency_stop") == 0) {
        ph_manager_set_emergency(true);
    }
    else if (strcmp(command, "reset_emergency") == 0) {
        ph_manager_set_emergency(false);
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
        pump_controller_reset_stats(PUMP_PH_UP);
        pump_controller_reset_stats(PUMP_PH_DOWN);
        ESP_LOGI(TAG, "Pump stats reset");
    }
    else if (strcmp(command, "run_pump_manual") == 0) {
        ESP_LOGI(TAG, "=== RUN PUMP MANUAL COMMAND ===");
        
        // Диагностика блокировок перед запуском
        if (s_emergency_mode) {
            ESP_LOGW(TAG, "Manual pump blocked: EMERGENCY mode active");
            return;
        }
        if (!mesh_manager_is_connected()) {
            ESP_LOGW(TAG, "Manual pump: mesh not connected (command will still be executed locally)");
        }
        
        // Отладочный вывод параметров
        if (params) {
            char *params_str = cJSON_PrintUnformatted(params);
            ESP_LOGI(TAG, "Command params: %s", params_str ? params_str : "NULL");
            if (params_str) free(params_str);
        } else {
            ESP_LOGW(TAG, "Command params is NULL!");
        }
        
        cJSON *pump_id = cJSON_GetObjectItem(params, "pump_id");
        cJSON *duration_sec = cJSON_GetObjectItem(params, "duration_sec");
        
        ESP_LOGI(TAG, "pump_id: %s, duration_sec: %s", 
                 pump_id ? "found" : "NULL", 
                 duration_sec ? "found" : "NULL");
        
        if (cJSON_IsNumber(pump_id) && cJSON_IsNumber(duration_sec)) {
            int pump = pump_id->valueint;
            float duration = (float)duration_sec->valuedouble;
            
            // Валидация
            if (pump < 0 || pump > 1) {
                ESP_LOGW(TAG, "Invalid pump_id: %d (must be 0-1)", pump);
                return;
            }
            if (duration <= 0.0f || duration > 30.0f) {
                ESP_LOGW(TAG, "Invalid duration: %.1f sec (must be 0.1-30.0)", duration);
                return;
            }
            
            // Запуск насоса вручную
            pump_id_t pid = (pump == 0) ? PUMP_PH_UP : PUMP_PH_DOWN;
            uint32_t duration_ms = (uint32_t)(duration * 1000.0f);
            
            ESP_LOGI(TAG, "Manual pump run: PUMP_%s (id=%d) for %.1f sec (%lu ms)", 
                     (pump == 0) ? "UP" : "DOWN", pump, duration, (unsigned long)duration_ms);
            // Информативный вывод текущей калибровки
            ESP_LOGI(TAG, "Calibration: pump %d = %.2f ml/s", pump, s_config->pump_calibration[pump].ml_per_second);
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
            if (pump < 0 || pump > 1) {
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
            esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Pump %d calibrated: %.2f ml/sec (%.1f ml in %.1f sec)", 
                         pump, s_config->pump_calibration[pump].ml_per_second, volume, duration);
            } else {
                ESP_LOGE(TAG, "Failed to save calibration to NVS");
            }
        }
    }
    else if (strcmp(command, "force_mock_mode") == 0) {
        cJSON *enable = cJSON_GetObjectItem(params, "enable");
        if (cJSON_IsBool(enable)) {
            bool mock_enable = cJSON_IsTrue(enable);
            ph_sensor_force_mock_mode(mock_enable);
            ESP_LOGI(TAG, "Mock mode %s", mock_enable ? "enabled" : "disabled");
        }
    }
    else if (strcmp(command, "get_sensor_status") == 0) {
        // Проверка подключения к mesh
        if (!mesh_manager_is_connected()) {
            ESP_LOGW(TAG, "Cannot send status: mesh offline");
            return;
        }
        
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            ESP_LOGE(TAG, "Failed to create root JSON object");
            return;
        }
        
        cJSON_AddStringToObject(root, "type", "sensor_status");
        cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
        cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
        
        cJSON *status = cJSON_CreateObject();
        if (status) {
            cJSON_AddBoolToObject(status, "connected", ph_sensor_is_connected());
            cJSON_AddBoolToObject(status, "mock_mode", ph_sensor_is_mock_mode());
            cJSON_AddBoolToObject(status, "stable", ph_sensor_is_stable());
            cJSON_AddNumberToObject(status, "current_ph", s_current_ph);
            cJSON_AddNumberToObject(status, "ph_target", s_config->ph_target);
            cJSON_AddItemToObject(root, "status", status);
        }
        
        char *json_str = cJSON_PrintUnformatted(root);
        if (json_str) {
            mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
            ESP_LOGI(TAG, "Sensor status sent");
            free(json_str);
        }
        
        cJSON_Delete(root);
    }
    else if (strcmp(command, "set_ph_target") == 0) {
        cJSON *ph_target = cJSON_GetObjectItem(params, "ph_target");
        if (cJSON_IsNumber(ph_target)) {
            float new_target = (float)ph_target->valuedouble;
            if (new_target >= 5.0f && new_target <= 8.0f) {
                s_config->ph_target = new_target;
                // Обновляем PID контроллеры
                adaptive_pid_set_target(&s_pid_ph_up, new_target);
                adaptive_pid_set_target(&s_pid_ph_down, new_target);
                
                // Сохраняем в NVS
                esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "pH target set to %.2f and saved to NVS", new_target);
                } else {
                    ESP_LOGE(TAG, "Failed to save pH target to NVS: %s", esp_err_to_name(err));
                }
            } else {
                ESP_LOGW(TAG, "Invalid pH target: %.2f (must be 5.0-8.0)", new_target);
            }
        }
    }
    else if (strcmp(command, "set_ec_target") == 0) {
        cJSON *ec_target = cJSON_GetObjectItem(params, "ec_target");
        if (cJSON_IsNumber(ec_target)) {
            float new_target = (float)ec_target->valuedouble;
            if (new_target >= 0.5f && new_target <= 3.0f) {
                // Для pH узла EC target не используется, но сохраняем для совместимости
                ESP_LOGI(TAG, "EC target set to %.2f (not used in pH node)", new_target);
            } else {
                ESP_LOGW(TAG, "Invalid EC target: %.2f (must be 0.5-3.0)", new_target);
            }
        }
    }
    else if (strcmp(command, "set_autonomous_mode") == 0) {
        cJSON *enable = cJSON_GetObjectItem(params, "enable");
        if (cJSON_IsBool(enable)) {
            bool autonomous = cJSON_IsTrue(enable);
            s_config->autonomous_enabled = autonomous;
            s_autonomous_mode = autonomous;
            
            // Сохраняем в NVS
            esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Autonomous mode %s", autonomous ? "enabled" : "disabled");
            } else {
                ESP_LOGE(TAG, "Failed to save autonomous mode to NVS");
            }
        }
    }
    else if (strcmp(command, "set_safety_settings") == 0) {
        cJSON *max_pump_time = cJSON_GetObjectItem(params, "max_pump_time_ms");
        cJSON *cooldown = cJSON_GetObjectItem(params, "cooldown_ms");
        
        if (cJSON_IsNumber(max_pump_time)) {
            s_config->max_pump_time_ms = (uint32_t)max_pump_time->valuedouble;
        }
        if (cJSON_IsNumber(cooldown)) {
            s_config->cooldown_ms = (uint32_t)cooldown->valuedouble;
        }
        
        // Сохраняем в NVS
        esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Safety settings updated: max_pump=%lu ms, cooldown=%lu ms", 
                     s_config->max_pump_time_ms, s_config->cooldown_ms);
        } else {
            ESP_LOGE(TAG, "Failed to save safety settings to NVS");
        }
    }
    else if (strcmp(command, "emergency_stop") == 0) {
        ph_manager_set_emergency(true);
        ESP_LOGW(TAG, "EMERGENCY STOP activated via command");
    }
    else if (strcmp(command, "reset_emergency") == 0) {
        ph_manager_set_emergency(false);
        ESP_LOGI(TAG, "Emergency mode reset via command");
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
        cJSON_AddStringToObject(config, "node_type", "ph");
        cJSON_AddStringToObject(config, "zone", s_config->base.zone);
        
        // pH параметры
        cJSON_AddNumberToObject(config, "ph_target", s_config->ph_target);
        cJSON_AddNumberToObject(config, "ph_min", s_config->ph_min);
        cJSON_AddNumberToObject(config, "ph_max", s_config->ph_max);
        cJSON_AddNumberToObject(config, "ph_cal_offset", s_config->ph_cal_offset);
        
        // Калибровка насосов
        cJSON *pumps_cal = cJSON_CreateArray();
        if (pumps_cal) {
            for (int i = 0; i < 2; i++) {
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
            for (int i = 0; i < 2; i++) {
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
void ph_manager_handle_config_update(cJSON *config_json) {
    ESP_LOGI(TAG, "Config update received");
    
    bool config_changed = false;
    
    cJSON *ph_target = cJSON_GetObjectItem(config_json, "ph_target");
    if (cJSON_IsNumber(ph_target)) {
        float new_target = (float)ph_target->valuedouble;
        if (new_target >= 5.0f && new_target <= 9.0f) {
            s_config->ph_target = new_target;
            adaptive_pid_set_setpoint(&s_pid_ph_up, s_config->ph_target);
            adaptive_pid_set_setpoint(&s_pid_ph_down, s_config->ph_target);
            config_changed = true;
            ESP_LOGI(TAG, "pH target updated: %.2f", s_config->ph_target);
        }
    }
    
    cJSON *ph_min = cJSON_GetObjectItem(config_json, "ph_min");
    if (cJSON_IsNumber(ph_min)) {
        float new_min = (float)ph_min->valuedouble;
        if (new_min >= 4.0f && new_min <= 8.0f) {
            s_config->ph_min = new_min;
            config_changed = true;
            ESP_LOGI(TAG, "pH min updated: %.2f", s_config->ph_min);
        }
    }
    
    cJSON *ph_max = cJSON_GetObjectItem(config_json, "ph_max");
    if (cJSON_IsNumber(ph_max)) {
        float new_max = (float)ph_max->valuedouble;
        if (new_max >= 6.0f && new_max <= 10.0f) {
            s_config->ph_max = new_max;
            config_changed = true;
            ESP_LOGI(TAG, "pH max updated: %.2f", s_config->ph_max);
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
            
            // Переинициализация адаптивных PID
            adaptive_pid_init(&s_pid_ph_up, s_config->ph_target,
                             s_config->pump_pid[0].kp, 
                             s_config->pump_pid[0].ki, 
                             s_config->pump_pid[0].kd);
            adaptive_pid_init(&s_pid_ph_down, s_config->ph_target,
                             s_config->pump_pid[1].kp,
                             s_config->pump_pid[1].ki, 
                             s_config->pump_pid[1].kd);
            
            config_changed = true;
            ESP_LOGI(TAG, "PID params updated: Kp=%.2f Ki=%.2f Kd=%.2f", 
                     (float)kp->valuedouble, (float)ki->valuedouble, (float)kd->valuedouble);
        }
    }
    
    // ВАЖНО: Сохранение в NVS
    if (config_changed) {
        esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Configuration saved to NVS");
        } else {
            ESP_LOGE(TAG, "Failed to save config to NVS: %s", esp_err_to_name(err));
        }
    }
}

