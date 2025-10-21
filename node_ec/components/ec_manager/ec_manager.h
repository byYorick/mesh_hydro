/**
 * @file ec_manager.h
 * @brief Главный менеджер NODE EC
 */

#ifndef EC_MANAGER_H
#define EC_MANAGER_H

#include "esp_err.h"
#include "node_config.h"
#include <stdbool.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация менеджера EC
 * 
 * @param config Указатель на конфигурацию узла
 * @return ESP_OK при успехе
 */
esp_err_t ec_manager_init(ec_node_config_t *config);

/**
 * @brief Запуск главной задачи управления
 * 
 * @return ESP_OK при успехе
 */
esp_err_t ec_manager_start(void);

/**
 * @brief Остановка менеджера
 * 
 * @return ESP_OK при успехе
 */
esp_err_t ec_manager_stop(void);

/**
 * @brief Обработка команды от ROOT
 * 
 * @param command Название команды
 * @param params JSON параметры
 */
void ec_manager_handle_command(const char *command, cJSON *params);

/**
 * @brief Обработка обновления конфигурации
 * 
 * @param config_json JSON конфигурация
 */
void ec_manager_handle_config_update(cJSON *config_json);

/**
 * @brief Установка Emergency режима
 * 
 * @param enable true - включить, false - выключить
 * @return ESP_OK при успехе
 */
esp_err_t ec_manager_set_emergency(bool enable);

/**
 * @brief Получение текущего значения EC
 * 
 * @param ec Указатель для EC
 * @return ESP_OK при успехе
 */
esp_err_t ec_manager_get_value(float *ec);

#ifdef __cplusplus
}
#endif

#endif // EC_MANAGER_H

