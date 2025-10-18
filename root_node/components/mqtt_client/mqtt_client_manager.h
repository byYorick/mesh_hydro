/**
 * @file mqtt_client_manager.h
 * @brief MQTT клиент для связи с сервером
 * 
 * Управляет подключением к MQTT broker, публикацией телеметрии
 * и подпиской на команды от сервера.
 */

#ifndef MQTT_CLIENT_MANAGER_H
#define MQTT_CLIENT_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback для обработки входящих MQTT сообщений
 * 
 * @param topic MQTT топик
 * @param data Данные сообщения
 * @param data_len Длина данных
 */
typedef void (*mqtt_recv_callback_t)(const char *topic, const char *data, int data_len);

/**
 * @brief Инициализация MQTT клиента
 * 
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_init(void);

/**
 * @brief Запуск MQTT клиента
 * 
 * Подключается к broker и подписывается на топики команд
 * 
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_start(void);

/**
 * @brief Остановка MQTT клиента
 * 
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_stop(void);

/**
 * @brief Публикация сообщения в MQTT
 * 
 * @param topic MQTT топик
 * @param data Данные для публикации
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_publish(const char *topic, const char *data);

/**
 * @brief Регистрация callback для обработки входящих сообщений
 * 
 * @param cb Callback функция
 */
void mqtt_client_manager_register_recv_cb(mqtt_recv_callback_t cb);

/**
 * @brief Проверка подключения к MQTT broker
 * 
 * @return true если подключен
 */
bool mqtt_client_manager_is_connected(void);

/**
 * @brief Переподключение к MQTT broker
 * 
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_reconnect(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_MANAGER_H

