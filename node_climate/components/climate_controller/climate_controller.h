/**
 * @file climate_controller.h
 * @brief Главная логика NODE Climate
 * 
 * Координирует чтение датчиков и отправку телеметрии
 */

#ifndef CLIMATE_CONTROLLER_H
#define CLIMATE_CONTROLLER_H

#include "esp_err.h"
#include "node_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация контроллера
 * 
 * @param config Конфигурация узла
 * @return ESP_OK при успехе
 */
esp_err_t climate_controller_init(climate_node_config_t *config);

/**
 * @brief Запуск главной задачи
 * 
 * @return ESP_OK при успехе
 */
esp_err_t climate_controller_start(void);

/**
 * @brief Остановка контроллера
 * 
 * @return ESP_OK при успехе
 */
esp_err_t climate_controller_stop(void);

/**
 * @brief Обработка команды от ROOT
 * 
 * @param command Команда
 * @param params Параметры (cJSON)
 */
void climate_controller_handle_command(const char *command, cJSON *params);

/**
 * @brief Обновление конфигурации
 * 
 * @param config_json Новая конфигурация (cJSON)
 */
void climate_controller_handle_config_update(cJSON *config_json);

#ifdef __cplusplus
}
#endif

#endif // CLIMATE_CONTROLLER_H

