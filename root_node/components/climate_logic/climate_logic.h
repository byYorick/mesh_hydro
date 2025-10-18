/**
 * @file climate_logic.h
 * @brief Резервная логика управления климатом
 * 
 * Активируется когда Climate node offline > 30 сек.
 * Использует простую таймерную логику для управления
 * форточками и вентиляцией через Relay node.
 */

#ifndef CLIMATE_LOGIC_H
#define CLIMATE_LOGIC_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация резервной климатической логики
 * 
 * @return ESP_OK при успехе
 */
esp_err_t climate_logic_init(void);

/**
 * @brief Запуск резервной логики (фоновая задача)
 * 
 * @return ESP_OK при успехе
 */
esp_err_t climate_logic_start(void);

/**
 * @brief Остановка резервной логики
 * 
 * @return ESP_OK при успехе
 */
esp_err_t climate_logic_stop(void);

/**
 * @brief Проверка, активна ли резервная логика
 * 
 * @return true если fallback режим активен
 */
bool climate_logic_is_fallback_active(void);

#ifdef __cplusplus
}
#endif

#endif // CLIMATE_LOGIC_H

