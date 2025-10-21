/**
 * @file pump_controller.h
 * @brief Управление 3 перистальтическими насосами с PWM (EC A/B/C)
 */

#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ID насосов
 */
typedef enum {
    PUMP_EC_A = 0,       // GPIO 2  - EC удобрение A
    PUMP_EC_B = 1,       // GPIO 3  - EC удобрение B
    PUMP_EC_C = 2,       // GPIO 4  - EC удобрение C (микроэлементы)
    PUMP_MAX = 3
} pump_id_t;

/**
 * @brief Статистика насоса
 */
typedef struct {
    uint32_t total_runs;         // Количество запусков
    float total_ml;              // Общий объём (мл)
    uint64_t total_time_ms;      // Общее время работы (мс)
    uint64_t last_run_time;      // Время последнего запуска
} pump_stats_t;

/**
 * @brief Инициализация контроллера насосов
 * 
 * Настраивает PWM на 3 GPIO (ESP32-C3)
 * 
 * @return ESP_OK при успехе
 */
esp_err_t pump_controller_init(void);

/**
 * @brief Включение насоса на заданное время
 * 
 * @param pump ID насоса
 * @param duration_ms Длительность (мс), макс 10000
 * @return ESP_OK при успехе
 */
esp_err_t pump_controller_run(pump_id_t pump, uint32_t duration_ms);

/**
 * @brief Включение насоса с заданной дозой
 * 
 * @param pump ID насоса
 * @param dose_ml Доза в мл
 * @return ESP_OK при успехе
 */
esp_err_t pump_controller_run_dose(pump_id_t pump, float dose_ml);

/**
 * @brief Остановка насоса
 * 
 * @param pump ID насоса
 * @return ESP_OK при успехе
 */
esp_err_t pump_controller_stop(pump_id_t pump);

/**
 * @brief Остановка всех насосов (аварийная)
 * 
 * @return ESP_OK при успехе
 */
esp_err_t pump_controller_emergency_stop(void);

/**
 * @brief Получение статистики насоса
 * 
 * @param pump ID насоса
 * @param stats Указатель на структуру статистики
 * @return ESP_OK при успехе
 */
esp_err_t pump_controller_get_stats(pump_id_t pump, pump_stats_t *stats);

/**
 * @brief Получение общего объёма за всё время
 * 
 * @param pump ID насоса
 * @return Объём в мл
 */
float pump_controller_get_total_ml(pump_id_t pump);

/**
 * @brief Сброс статистики насоса
 * 
 * @param pump ID насоса
 * @return ESP_OK при успехе
 */
esp_err_t pump_controller_reset_stats(pump_id_t pump);

/**
 * @brief Установка калибровки (мл/сек)
 * 
 * @param pump ID насоса
 * @param ml_per_sec Производительность (мл/сек)
 * @return ESP_OK при успехе
 */
esp_err_t pump_controller_set_calibration(pump_id_t pump, float ml_per_sec);

/**
 * @brief Проверка, работает ли насос
 * 
 * @param pump ID насоса
 * @return true если работает
 */
bool pump_controller_is_running(pump_id_t pump);

#ifdef __cplusplus
}
#endif

#endif // PUMP_CONTROLLER_H

