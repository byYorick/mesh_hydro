/**
 * @file pump_manager.h
 * @brief Управление 5 насосами с safety проверками
 * 
 * TODO: Портировать из hydro1.0/components/pump_manager/
 * 
 * Функции:
 * - 5 насосов (pH UP, pH DOWN, EC A, EC B, EC C)
 * - Safety: max time, cooldown
 * - Статистика (мл за час/день)
 * - Emergency stop
 */

#ifndef PUMP_MANAGER_H
#define PUMP_MANAGER_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PUMP_PH_UP = 0,
    PUMP_PH_DOWN,
    PUMP_EC_A,
    PUMP_EC_B,
    PUMP_EC_C,
    PUMP_MAX
} pump_id_t;

/**
 * @brief Инициализация менеджера насосов
 * 
 * @return ESP_OK при успехе
 */
esp_err_t pump_manager_init(void);

/**
 * @brief Запуск насоса
 * 
 * @param pump_id ID насоса
 * @param duration_ms Длительность работы (мс)
 * @return ESP_OK при успехе
 */
esp_err_t pump_manager_run(pump_id_t pump_id, uint32_t duration_ms);

/**
 * @brief Аварийная остановка всех насосов
 */
void pump_manager_emergency_stop(void);

/**
 * @brief Получение статистики насоса
 * 
 * @param pump_id ID насоса
 * @param ml_hour мл за час
 * @param ml_day мл за день
 */
void pump_manager_get_stats(pump_id_t pump_id, uint32_t *ml_hour, uint32_t *ml_day);

#ifdef __cplusplus
}
#endif

#endif // PUMP_MANAGER_H

