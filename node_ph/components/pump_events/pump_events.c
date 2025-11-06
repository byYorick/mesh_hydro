/**
 * @file pump_events.c
 * @brief Реализация системы событий для насосов pH ноды с данными PID
 */

#include "pump_events.h"
#include "mesh_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <time.h>

static const char *TAG = "pump_events_ph";

// Внешние переменные для получения конфигурации
extern const char *g_node_id;
extern bool g_emergency_mode;
extern bool g_autonomous_mode;

/**
 * @brief Создание JSON сообщения события pH
 */
static esp_err_t create_event_json(const pump_event_t *event, char *json_buf, size_t buf_size) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON root");
        return ESP_FAIL;
    }

    // Основные поля события
    cJSON_AddStringToObject(root, "type", "event");
    cJSON_AddStringToObject(root, "node_id", g_node_id);
    cJSON_AddStringToObject(root, "node_type", "ph");
    cJSON_AddNumberToObject(root, "timestamp", event->timestamp);
    cJSON_AddStringToObject(root, "level", "info");

    // Данные события насоса pH
    cJSON *event_data = cJSON_CreateObject();
    if (!event_data) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // Тип события насоса
    const char *event_type_str;
    switch (event->type) {
        case PUMP_EVENT_START:
            event_type_str = "pump_start";
            break;
        case PUMP_EVENT_STOP:
            event_type_str = "pump_stop";
            break;
        case PUMP_EVENT_EMERGENCY_STOP:
            event_type_str = "pump_emergency_stop";
            break;
        case PUMP_EVENT_TIMEOUT:
            event_type_str = "pump_timeout";
            break;
        case PUMP_EVENT_CALIBRATION_START:
            event_type_str = "pump_calibration_start";
            break;
        case PUMP_EVENT_CALIBRATION_END:
            event_type_str = "pump_calibration_end";
            break;
        default:
            event_type_str = "pump_unknown";
            break;
    }
    cJSON_AddStringToObject(event_data, "event_type", event_type_str);

    // Данные насоса pH
    const char *pump_name = (event->pump_id == PUMP_PH_UP) ? "pH UP" : "pH DOWN";
    cJSON_AddNumberToObject(event_data, "pump_id", event->pump_id);
    cJSON_AddStringToObject(event_data, "pump_name", pump_name);
    cJSON_AddNumberToObject(event_data, "duration_ms", event->duration_ms);
    cJSON_AddNumberToObject(event_data, "dose_ml", event->dose_ml);
    cJSON_AddNumberToObject(event_data, "ml_per_second", event->ml_per_second);

    // Данные PID pH
    cJSON *pid_data = cJSON_CreateObject();
    if (pid_data) {
        cJSON_AddNumberToObject(pid_data, "kp", event->pid_data.kp);
        cJSON_AddNumberToObject(pid_data, "ki", event->pid_data.ki);
        cJSON_AddNumberToObject(pid_data, "kd", event->pid_data.kd);
        cJSON_AddNumberToObject(pid_data, "setpoint", event->pid_data.setpoint);
        cJSON_AddNumberToObject(pid_data, "current_value", event->pid_data.current_value);
        cJSON_AddNumberToObject(pid_data, "error", event->pid_data.error);
        cJSON_AddNumberToObject(pid_data, "output", event->pid_data.output);
        cJSON_AddNumberToObject(pid_data, "integral", event->pid_data.integral);
        cJSON_AddNumberToObject(pid_data, "derivative", event->pid_data.derivative);
        cJSON_AddBoolToObject(pid_data, "enabled", event->pid_data.enabled);
        cJSON_AddItemToObject(event_data, "pid_data", pid_data);
    }

    // Системные данные pH
    cJSON_AddNumberToObject(event_data, "current_ph", event->current_ph);
    cJSON_AddNumberToObject(event_data, "ph_target", event->ph_target);
    cJSON_AddBoolToObject(event_data, "emergency_mode", event->emergency_mode);
    cJSON_AddBoolToObject(event_data, "autonomous_mode", event->autonomous_mode);
    cJSON_AddNumberToObject(event_data, "rssi", event->rssi);

    cJSON_AddItemToObject(root, "data", event_data);

    // Создание детального сообщения на русском языке
    char message[256];
    
    switch (event->type) {
        case PUMP_EVENT_START:
            snprintf(message, sizeof(message), "Насос %s запущен: %.1f мл (%lu мс)", 
                     pump_name, event->dose_ml, (unsigned long)event->duration_ms);
            break;
        case PUMP_EVENT_STOP:
            snprintf(message, sizeof(message), "Насос %s остановлен: %.1f мл (%lu мс)", 
                     pump_name, event->dose_ml, (unsigned long)event->duration_ms);
            break;
        case PUMP_EVENT_EMERGENCY_STOP:
            snprintf(message, sizeof(message), "Насос %s АВАРИЙНАЯ ОСТАНОВКА (%lu мс)", 
                     pump_name, (unsigned long)event->duration_ms);
            break;
        case PUMP_EVENT_TIMEOUT:
            snprintf(message, sizeof(message), "Насос %s ТАЙМАУТ (%lu мс)", 
                     pump_name, (unsigned long)event->duration_ms);
            break;
        case PUMP_EVENT_CALIBRATION_START:
            snprintf(message, sizeof(message), "Насос %s калибровка начата", pump_name);
            break;
        case PUMP_EVENT_CALIBRATION_END:
            snprintf(message, sizeof(message), "Насос %s калибровка завершена: %.1f мл/с", 
                     pump_name, event->ml_per_second);
            break;
        default:
            snprintf(message, sizeof(message), "Насос %s неизвестное событие", pump_name);
            break;
    }
    
    cJSON_AddStringToObject(root, "message", message);

    // Создание сообщения
    char *json_string = cJSON_PrintUnformatted(root);
    if (!json_string) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    if (strlen(json_string) >= buf_size) {
        ESP_LOGE(TAG, "JSON too large: %zu >= %zu", strlen(json_string), buf_size);
        free(json_string);
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    strcpy(json_buf, json_string);
    free(json_string);
    cJSON_Delete(root);

    return ESP_OK;
}

/**
 * @brief Отправка события на сервер
 */
static esp_err_t send_event_to_server(const pump_event_t *event) {
    // Логирование события всегда (даже если mesh не подключен)
    const char *pump_name = (event->pump_id == PUMP_PH_UP) ? "pH UP" : "pH DOWN";
    const char *event_type_str;
    switch (event->type) {
        case PUMP_EVENT_START:
            event_type_str = "START";
            ESP_LOGI(TAG, "[PUMP EVENT %s] насос %s запущен: %.1f мл (%lu мс), pH=%.2f, target=%.2f", 
                     event_type_str, pump_name, event->dose_ml, (unsigned long)event->duration_ms, 
                     event->current_ph, event->ph_target);
            break;
        case PUMP_EVENT_STOP:
            event_type_str = "STOP";
            ESP_LOGI(TAG, "[PUMP EVENT %s] насос %s остановлен: %.1f мл (%lu мс)", 
                     event_type_str, pump_name, event->dose_ml, (unsigned long)event->duration_ms);
            break;
        case PUMP_EVENT_EMERGENCY_STOP:
            event_type_str = "EMERGENCY_STOP";
            ESP_LOGE(TAG, "[PUMP EVENT %s] pump %s EMERGENCY STOP [duration=%lu ms]", 
                     event_type_str, pump_name, (unsigned long)event->duration_ms);
            break;
        case PUMP_EVENT_TIMEOUT:
            event_type_str = "TIMEOUT";
            ESP_LOGW(TAG, "[PUMP EVENT %s] pump %s TIMEOUT [duration=%lu ms]", 
                     event_type_str, pump_name, (unsigned long)event->duration_ms);
            break;
        default:
            event_type_str = "UNKNOWN";
            ESP_LOGW(TAG, "[PUMP EVENT %s] pump %s unknown event type %d", 
                     event_type_str, pump_name, event->type);
            break;
    }
    
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "   [WARNING] Event not sent - mesh offline");
        return ESP_FAIL;
    }

    char json_buf[1024];
    esp_err_t err = create_event_json(event, json_buf, sizeof(json_buf));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "   [ERROR] Failed to create event JSON");
        return err;
    }

    // Отправка через mesh
    err = mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "   [OK] Pump event sent to ROOT (%d bytes)", (int)strlen(json_buf));
    } else {
        ESP_LOGW(TAG, "   [ERROR] Failed to send pump event: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t pump_events_init(void) {
    ESP_LOGI(TAG, "pH Pump events system initialized");
    return ESP_OK;
}

esp_err_t pump_events_send_event(const pump_event_t *event) {
    if (!event) {
        ESP_LOGE(TAG, "Event is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    return send_event_to_server(event);
}

esp_err_t pump_events_send_start_event(
    pump_id_t pump_id,
    uint32_t duration_ms,
    float dose_ml,
    const pump_event_pid_data_t *pid_data,
    float ph,
    float ph_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
) {
    pump_event_t event = {
        .type = PUMP_EVENT_START,
        .pump_id = pump_id,
        .timestamp = (uint32_t)time(NULL),
        .duration_ms = duration_ms,
        .dose_ml = dose_ml,
        .ml_per_second = 0.0f, // Будет заполнено из конфигурации
        .current_ph = ph,
        .ph_target = ph_target,
        .emergency_mode = emergency_mode,
        .autonomous_mode = autonomous_mode,
        .rssi = rssi
    };

    if (pid_data) {
        memcpy(&event.pid_data, pid_data, sizeof(pump_event_pid_data_t));
    } else {
        memset(&event.pid_data, 0, sizeof(pump_event_pid_data_t));
    }

    return pump_events_send_event(&event);
}

esp_err_t pump_events_send_stop_event(
    pump_id_t pump_id,
    uint32_t duration_ms,
    float dose_ml,
    const pump_event_pid_data_t *pid_data,
    float ph,
    float ph_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
) {
    pump_event_t event = {
        .type = PUMP_EVENT_STOP,
        .pump_id = pump_id,
        .timestamp = (uint32_t)time(NULL),
        .duration_ms = duration_ms,
        .dose_ml = dose_ml,
        .ml_per_second = 0.0f, // Будет заполнено из конфигурации
        .current_ph = ph,
        .ph_target = ph_target,
        .emergency_mode = emergency_mode,
        .autonomous_mode = autonomous_mode,
        .rssi = rssi
    };

    if (pid_data) {
        memcpy(&event.pid_data, pid_data, sizeof(pump_event_pid_data_t));
    } else {
        memset(&event.pid_data, 0, sizeof(pump_event_pid_data_t));
    }

    return pump_events_send_event(&event);
}

esp_err_t pump_events_send_emergency_stop_event(
    pump_id_t pump_id,
    const char *reason,
    float ph,
    float ph_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
) {
    pump_event_t event = {
        .type = PUMP_EVENT_EMERGENCY_STOP,
        .pump_id = pump_id,
        .timestamp = (uint32_t)time(NULL),
        .duration_ms = 0,
        .dose_ml = 0.0f,
        .ml_per_second = 0.0f,
        .current_ph = ph,
        .ph_target = ph_target,
        .emergency_mode = emergency_mode,
        .autonomous_mode = autonomous_mode,
        .rssi = rssi
    };

    memset(&event.pid_data, 0, sizeof(pump_event_pid_data_t));

    return pump_events_send_event(&event);
}

esp_err_t pump_events_get_pid_data(
    const pid_controller_t *pid,
    float current_value,
    pump_event_pid_data_t *pid_data
) {
    if (!pid || !pid_data) {
        return ESP_ERR_INVALID_ARG;
    }

    pid_data->kp = pid->kp;
    pid_data->ki = pid->ki;
    pid_data->kd = pid->kd;
    pid_data->setpoint = pid->setpoint;
    pid_data->current_value = current_value;
    pid_data->error = pid->setpoint - current_value;
    pid_data->output = 0.0f; // Будет заполнено при вычислении
    pid_data->integral = pid->integral;
    pid_data->derivative = (pid_data->error - pid->prev_error) / 1.0f; // dt = 1.0f
    pid_data->enabled = pid->enabled;

    return ESP_OK;
}
