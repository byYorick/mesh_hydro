/**
 * @file ph_manager.h
 * @brief Главный менеджер NODE pH
 */

#ifndef PH_MANAGER_H
#define PH_MANAGER_H

#include "esp_err.h"
#include "node_config.h"
#include <stdbool.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация менеджера pH
 * 
 * @param config Указатель на конфигурацию узла
 * @return ESP_OK при успехе
 */
esp_err_t ph_manager_init(ph_node_config_t *config);

/**
 * @brief Запуск главной задачи управления
 * 
 * @return ESP_OK при успехе
 */
esp_err_t ph_manager_start(void);

/**
 * @brief Остановка менеджера
 * 
 * @return ESP_OK при успехе
 */
esp_err_t ph_manager_stop(void);

/**
 * @brief Обработка команды от ROOT
 * 
 * @param command Название команды
 * @param params JSON параметры
 */
void ph_manager_handle_command(const char *command, cJSON *params);

/**
 * @brief Обработка обновления конфигурации
 * 
 * @param config_json JSON конфигурация
 */
void ph_manager_handle_config_update(cJSON *config_json);

/**
 * @brief Установка Emergency режима
 * 
 * @param enable true - включить, false - выключить
 * @return ESP_OK при успехе
 */
esp_err_t ph_manager_set_emergency(bool enable);

/**
 * @brief Получение текущего значения pH
 * 
 * @param ph Указатель для pH
 * @return ESP_OK при успехе
 */
esp_err_t ph_manager_get_value(float *ph);

// Внутренние функции для обработки команд
void ph_manager_handle_ph_target_command(cJSON *params);
void ph_manager_handle_pump_command(cJSON *params);
void ph_manager_handle_reset_stats_command(void);
void ph_manager_handle_manual_pump_command(cJSON *params);
void ph_manager_handle_safety_command(cJSON *params);
void ph_manager_handle_pid_command(cJSON *params);
void ph_manager_handle_autonomous_command(cJSON *params);
void ph_manager_send_config_response(void);

#ifdef __cplusplus
}
#endif

#endif // PH_MANAGER_H

