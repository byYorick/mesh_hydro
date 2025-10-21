/**
 * @file connection_monitor.c
 * @brief Реализация мониторинга связи
 */

#include "connection_monitor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "conn_monitor";

// Пороги переключения состояний (мс)
#define DEGRADED_THRESHOLD_MS   10000   // 10 сек без контакта
#define AUTONOMOUS_THRESHOLD_MS 30000   // 30 сек без контакта

static connection_state_t s_current_state = CONN_STATE_ONLINE;
static uint64_t s_last_root_contact_ms = 0;
static TaskHandle_t s_monitor_task = NULL;
static connection_state_changed_cb_t s_state_cb = NULL;

// Forward declaration
static void connection_monitor_task(void *arg);
static void change_state(connection_state_t new_state);
static const char* state_to_string(connection_state_t state);

esp_err_t connection_monitor_init(void) {
    s_current_state = CONN_STATE_ONLINE;
    s_last_root_contact_ms = esp_timer_get_time() / 1000;
    
    ESP_LOGI(TAG, "Connection Monitor initialized");
    return ESP_OK;
}

esp_err_t connection_monitor_start(void) {
    if (s_monitor_task != NULL) {
        ESP_LOGW(TAG, "Monitor task already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(connection_monitor_task,
                                  "conn_monitor",
                                  3072,
                                  NULL,
                                  5,
                                  &s_monitor_task);
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create monitor task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Connection Monitor started");
    return ESP_OK;
}

esp_err_t connection_monitor_stop(void) {
    if (s_monitor_task != NULL) {
        vTaskDelete(s_monitor_task);
        s_monitor_task = NULL;
        ESP_LOGI(TAG, "Connection Monitor stopped");
    }
    return ESP_OK;
}

void connection_monitor_mark_root_contact(void) {
    s_last_root_contact_ms = esp_timer_get_time() / 1000;
    
    // Если были в деградированном или автономном режиме - вернуться в ONLINE
    if (s_current_state != CONN_STATE_ONLINE) {
        change_state(CONN_STATE_ONLINE);
    }
}

connection_state_t connection_monitor_get_state(void) {
    return s_current_state;
}

void connection_monitor_register_state_cb(connection_state_changed_cb_t cb) {
    s_state_cb = cb;
}

uint64_t connection_monitor_get_time_since_last_contact(void) {
    uint64_t now_ms = esp_timer_get_time() / 1000;
    return now_ms - s_last_root_contact_ms;
}

// Задача мониторинга
static void connection_monitor_task(void *arg) {
    ESP_LOGI(TAG, "Monitor task running");

    while (1) {
        uint64_t elapsed_ms = connection_monitor_get_time_since_last_contact();

        // Машина состояний
        switch (s_current_state) {
            case CONN_STATE_ONLINE:
                if (elapsed_ms > DEGRADED_THRESHOLD_MS) {
                    change_state(CONN_STATE_DEGRADED);
                }
                break;

            case CONN_STATE_DEGRADED:
                if (elapsed_ms > AUTONOMOUS_THRESHOLD_MS) {
                    change_state(CONN_STATE_AUTONOMOUS);
                } else if (elapsed_ms < DEGRADED_THRESHOLD_MS) {
                    change_state(CONN_STATE_ONLINE);
                }
                break;

            case CONN_STATE_AUTONOMOUS:
                if (elapsed_ms < DEGRADED_THRESHOLD_MS) {
                    change_state(CONN_STATE_ONLINE);
                }
                // AUTONOMOUS → EMERGENCY переключается внешне (при критичных событиях)
                break;

            case CONN_STATE_EMERGENCY:
                // Из EMERGENCY можно выйти только вручную или при восстановлении связи
                if (elapsed_ms < DEGRADED_THRESHOLD_MS) {
                    change_state(CONN_STATE_ONLINE);
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(5000));  // Проверка каждые 5 сек
    }
}

// Изменение состояния с уведомлением
static void change_state(connection_state_t new_state) {
    if (new_state == s_current_state) {
        return;
    }

    connection_state_t old_state = s_current_state;
    s_current_state = new_state;

    ESP_LOGI(TAG, "State changed: %s → %s",
             state_to_string(old_state),
             state_to_string(new_state));

    // Вызов callback если зарегистрирован
    if (s_state_cb) {
        s_state_cb(new_state, old_state);
    }
}

// Преобразование состояния в строку
static const char* state_to_string(connection_state_t state) {
    switch (state) {
        case CONN_STATE_ONLINE:     return "ONLINE";
        case CONN_STATE_DEGRADED:   return "DEGRADED";
        case CONN_STATE_AUTONOMOUS: return "AUTONOMOUS";
        case CONN_STATE_EMERGENCY:  return "EMERGENCY";
        default:                    return "UNKNOWN";
    }
}

