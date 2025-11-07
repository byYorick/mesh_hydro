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

/**
 * @brief Отправка discovery сообщения
 * 
 * Публикует информацию о ROOT узле в hydro/discovery
 * Используется при первом подключении и периодически для регистрации на сервере
 */
void mqtt_client_manager_send_discovery(void);

/**
 * @brief ⭐ ЗОНИРОВАНИЕ: Установка MQTT Topic Prefix для зоны
 * 
 * ВАЖНО: Должно быть вызвано перед start() для настройки правильных топиков
 * 
 * @param prefix MQTT prefix (например "hydro/zone1/")
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_set_topic_prefix(const char *prefix);

/**
 * @brief ⭐ ЗОНИРОВАНИЕ: Получение текущего MQTT Topic Prefix
 * 
 * @param prefix Буфер для prefix (мин. 64 байта)
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_get_topic_prefix(char *prefix);

/**
 * @brief ⭐ ЗОНИРОВАНИЕ: Публикация с автоматическим добавлением zone prefix
 * 
 * Примеры:
 * - subtopic="telemetry" → публикация в "hydro/zone1/telemetry"
 * - subtopic="events" → публикация в "hydro/zone1/events"
 * 
 * @param subtopic Подтопик (без префикса зоны)
 * @param data Данные для публикации
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_publish_zone(const char *subtopic, const char *data);

/**
 * @brief ⭐ ЗОНИРОВАНИЕ: Подписка на топик с zone prefix
 * 
 * @param subtopic Подтопик (без префикса зоны)
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_subscribe_zone(const char *subtopic);

/**
 * @brief ⭐ ЗОНИРОВАНИЕ: Построение полного топика с zone prefix
 * 
 * Вспомогательная функция для других компонентов
 * 
 * @param subtopic Подтопик
 * @param full_topic Буфер для полного топика (мин. 128 байт)
 * @return ESP_OK при успехе
 */
esp_err_t mqtt_client_manager_build_topic(const char *subtopic, char *full_topic);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_MANAGER_H

