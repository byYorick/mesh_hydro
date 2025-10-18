/**
 * @file actuator_base.h
 * @brief Базовые функции для работы с исполнительными устройствами
 */

#ifndef ACTUATOR_BASE_H
#define ACTUATOR_BASE_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Safety check перед включением
bool actuator_safety_check(const char *actuator_id, uint32_t duration_ms, uint32_t max_duration_ms);

// Статистика работы
void actuator_log_usage(const char *actuator_id, uint32_t duration_ms);
uint32_t actuator_get_total_time(const char *actuator_id);

#ifdef __cplusplus
}
#endif

#endif // ACTUATOR_BASE_H

