/**
 * @file adaptive_pid.h
 * @brief Адаптивный PID контроллер
 * 
 * TODO: Портировать из hydro1.0/components/adaptive_pid/
 * 
 * AI PID контроллер с адаптацией параметров:
 * - pH UP/DOWN контроллеры
 * - EC A/B/C контроллеры
 * - Адаптация Kp, Ki, Kd
 * - Anti-windup
 */

#ifndef ADAPTIVE_PID_H
#define ADAPTIVE_PID_H

#include "esp_err.h"
#include "node_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация PID контроллера
 * 
 * @param config Конфигурация с PID параметрами
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_init(ph_ec_node_config_t *config);

/**
 * @brief Обновление PID контроллера (один цикл)
 * 
 * @param ph Текущее значение pH
 * @param ec Текущее значение EC
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_update(float ph, float ec);

/**
 * @brief Сброс PID интеграторов
 */
void adaptive_pid_reset(void);

#ifdef __cplusplus
}
#endif

#endif // ADAPTIVE_PID_H

