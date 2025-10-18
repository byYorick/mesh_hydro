/**
 * @file sensor_base.c
 * @brief Реализация базовых функций датчиков
 */

#include "sensor_base.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "sensor_base";

typedef struct {
    char sensor_id[32];
    float value;
    uint64_t timestamp;
} sensor_cache_entry_t;

#define MAX_CACHE_ENTRIES 16
static sensor_cache_entry_t s_cache[MAX_CACHE_ENTRIES];
static int s_cache_count = 0;

esp_err_t sensor_read_with_retry(esp_err_t (*read_fn)(float *value), float *value, int max_retries) {
    esp_err_t err = ESP_FAIL;
    
    for (int i = 0; i < max_retries; i++) {
        err = read_fn(value);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Задержка перед retry
    }
    
    ESP_LOGW(TAG, "Sensor read failed after %d retries", max_retries);
    return err;
}

bool sensor_validate_range(float value, float min, float max) {
    return (value >= min && value <= max);
}

void sensor_cache_value(const char *sensor_id, float value) {
    // Поиск существующей записи
    for (int i = 0; i < s_cache_count; i++) {
        if (strcmp(s_cache[i].sensor_id, sensor_id) == 0) {
            s_cache[i].value = value;
            return;
        }
    }
    
    // Добавление новой записи
    if (s_cache_count < MAX_CACHE_ENTRIES) {
        strncpy(s_cache[s_cache_count].sensor_id, sensor_id, sizeof(s_cache[0].sensor_id) - 1);
        s_cache[s_cache_count].value = value;
        s_cache_count++;
    }
}

float sensor_get_cached_value(const char *sensor_id) {
    for (int i = 0; i < s_cache_count; i++) {
        if (strcmp(s_cache[i].sensor_id, sensor_id) == 0) {
            return s_cache[i].value;
        }
    }
    return 0.0f;  // Не найдено
}

