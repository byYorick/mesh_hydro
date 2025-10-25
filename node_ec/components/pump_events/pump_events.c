/**
 * @file pump_events.c
 * @brief Реализация системы событий для насосов EC ноды с данными PID
 */

#include "pump_events.h"
#include "mesh_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <time.h>

static const char *TAG = "pump_events_ec";

// Внешние переменные для получения конфигурации
extern const char *g_node_id;
extern bool g_emergency_mode;
extern bool g_autonomous_mode;

/**
 * @brief Получение RSSI к родительскому узлу
 */
static int8_t get_rssi_to_parent(void) {
    return mesh_manager_get_parent_rssi();
}

/**
 * @brief Создание JSON сообщения события EC
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
    cJSON_AddStringToObject(root, "node_type", "ec");
    cJSON_AddNumberToObject(root, "timestamp", event->timestamp);
    cJSON_AddStringToObject(root, "level", "info");

    // Данные события насоса EC
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

    // Данные насоса EC
    cJSON_AddNumberToObject(event_data, "pump_id", event->pump_id);
    cJSON_AddNumberToObject(event_data, "duration_ms", event->duration_ms);
    cJSON_AddNumberToObject(event_data, "dose_ml", event->dose_ml);
    cJSON_AddNumberToObject(event_data, "ml_per_second", event->ml_per_second);

    // Данные PID EC
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

    // Системные данные EC
    cJSON_AddNumberToObject(event_data, "current_ec", event->current_ec);
    cJSON_AddNumberToObject(event_data, "ec_target", event->ec_target);
    cJSON_AddBoolToObject(event_data, "emergency_mode", event->emergency_mode);
    cJSON_AddBoolToObject(event_data, "autonomous_mode", event->autonomous_mode);
    cJSON_AddNumberToObject(event_data, "rssi", event->rssi);

    cJSON_AddItemToObject(root, "data", event_data);

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
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, event skipped");
        return ESP_FAIL;
    }

    char json_buf[1024];
    esp_err_t err = create_event_json(event, json_buf, sizeof(json_buf));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create event JSON");
        return err;
    }

    // Отправка через mesh
    err = mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "📤 EC Pump event sent: %s (pump %d)", 
                 event->type == PUMP_EVENT_START ? "START" : "STOP", event->pump_id);
    } else {
        ESP_LOGW(TAG, "Failed to send EC pump event: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t pump_events_init(void) {
    ESP_LOGI(TAG, "EC Pump events system initialized");
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
    float ec,
    float ec_target,
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
        .current_ec = ec,
        .ec_target = ec_target,
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
    float ec,
    float ec_target,
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
        .current_ec = ec,
        .ec_target = ec_target,
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
    float ec,
    float ec_target,
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
        .current_ec = ec,
        .ec_target = ec_target,
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
