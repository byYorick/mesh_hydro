/**
 * @file node_controller.h
 * @brief Главная логика NODE pH/EC
 * 
 * Координирует работу всех компонентов:
 * - Чтение датчиков
 * - PID управление
 * - Отправка телеметрии
 * - Обработка команд
 * - Автономная работа
 */

#ifndef NODE_CONTROLLER_H
#define NODE_CONTROLLER_H

#include "esp_err.h"
#include "node_config.h"
#include "connection_monitor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация контроллера узла
 * 
 * @param config Указатель на конфигурацию
 * @return ESP_OK при успехе
 */
esp_err_t node_controller_init(ph_ec_node_config_t *config);

/**
 * @brief Запуск главной задачи контроллера
 * 
 * @return ESP_OK при успехе
 */
esp_err_t node_controller_start(void);

/**
 * @brief Остановка контроллера
 * 
 * @return ESP_OK при успехе
 */
esp_err_t node_controller_stop(void);

/**
 * @brief Обработка команды от ROOT
 * 
 * @param command Команда
 * @param params JSON объект с параметрами
 */
void node_controller_handle_command(const char *command, cJSON *params);

/**
 * @brief Обработка обновления конфигурации от ROOT
 * 
 * @param config_json JSON объект с новой конфигурацией
 */
void node_controller_handle_config_update(cJSON *config_json);

/**
 * @brief Переключение в автономный режим
 */
void node_controller_enter_autonomous(void);

/**
 * @brief Выход из автономного режима
 */
void node_controller_exit_autonomous(void);

/**
 * @brief Обработка аварийной ситуации
 * 
 * @param message Описание аварии
 * @param value Значение параметра
 */
void node_controller_handle_emergency(const char *message, float value);

#ifdef __cplusplus
}
#endif

#endif // NODE_CONTROLLER_H

