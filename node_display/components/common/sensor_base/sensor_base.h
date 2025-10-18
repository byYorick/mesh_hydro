/**
 * @file sensor_base.h
 * @brief Базовые функции для работы с датчиками
 */

#ifndef SENSOR_BASE_H
#define SENSOR_BASE_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Retry логика при чтении датчика
esp_err_t sensor_read_with_retry(esp_err_t (*read_fn)(float *value), float *value, int max_retries);

// Валидация значения в диапазоне
bool sensor_validate_range(float value, float min, float max);

// Кэширование последнего значения
void sensor_cache_value(const char *sensor_id, float value);
float sensor_get_cached_value(const char *sensor_id);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_BASE_H

