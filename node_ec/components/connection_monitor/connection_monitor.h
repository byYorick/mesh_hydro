/**
 * @file connection_monitor.h
 * @brief Мониторинг связи с ROOT узлом
 * 
 * Отслеживает качество связи и автоматически переключает режимы:
 * ONLINE → DEGRADED → AUTONOMOUS → EMERGENCY
 */

#ifndef CONNECTION_MONITOR_H
#define CONNECTION_MONITOR_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Состояния подключения
 */
typedef enum {
    CONN_STATE_ONLINE = 0,      ///< Нормальная связь
    CONN_STATE_DEGRADED,        ///< Нестабильная связь (10-30 сек без контакта)
    CONN_STATE_AUTONOMOUS,      ///< Автономный режим (>30 сек)
    CONN_STATE_EMERGENCY        ///< Аварийная ситуация
} connection_state_t;

/**
 * @brief Callback при изменении состояния подключения
 * 
 * @param new_state Новое состояние
 * @param old_state Предыдущее состояние
 */
typedef void (*connection_state_changed_cb_t)(connection_state_t new_state, connection_state_t old_state);

/**
 * @brief Инициализация мониторинга связи
 * 
 * @return ESP_OK при успехе
 */
esp_err_t connection_monitor_init(void);

/**
 * @brief Запуск мониторинга (фоновая задача)
 * 
 * @return ESP_OK при успехе
 */
esp_err_t connection_monitor_start(void);

/**
 * @brief Остановка мониторинга
 * 
 * @return ESP_OK при успехе
 */
esp_err_t connection_monitor_stop(void);

/**
 * @brief Отметка получения сообщения от ROOT
 * 
 * Вызывай каждый раз при получении данных от ROOT
 */
void connection_monitor_mark_root_contact(void);

/**
 * @brief Получение текущего состояния подключения
 * 
 * @return Текущее состояние
 */
connection_state_t connection_monitor_get_state(void);

/**
 * @brief Регистрация callback при изменении состояния
 * 
 * @param cb Callback функция
 */
void connection_monitor_register_state_cb(connection_state_changed_cb_t cb);

/**
 * @brief Получение времени с последнего контакта с ROOT (миллисекунды)
 * 
 * @return Время в миллисекундах
 */
uint64_t connection_monitor_get_time_since_last_contact(void);

#ifdef __cplusplus
}
#endif

#endif // CONNECTION_MONITOR_H

