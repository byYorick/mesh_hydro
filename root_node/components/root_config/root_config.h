/**
 * @file root_config.h
 * @brief ⭐ ЗОНИРОВАНИЕ: Конфигурация Root Node (Zone Identification)
 * 
 * Этот компонент отвечает за:
 * - Идентификацию Root Node как уникальной зоны
 * - Хранение ROOT_NODE_ID и MESH_NETWORK_ID в NVS
 * - Генерацию MQTT topic prefix для зоны
 * - Настройку mesh сети для зоны
 */

#ifndef ROOT_CONFIG_H
#define ROOT_CONFIG_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ⭐ Конфигурация Root Node (Зона)
 */
typedef struct {
    char root_node_id[32];        ///< ID Root Node (например "root_001")
    char mesh_network_id[32];     ///< Уникальный MESH_NETWORK_ID зоны (например "HYDRO1_ZONE1")
    char mqtt_topic_prefix[64];   ///< MQTT prefix для зоны (например "hydro/zone1/")
    char mqtt_host[128];          ///< MQTT broker host для зоны
    uint16_t mqtt_port;           ///< MQTT broker port
    char zone_name[64];           ///< Название зоны (например "Зона 1 - NFT")
    char zone_location[64];       ///< Расположение зоны (например "Лаборатория А")
    uint8_t zone_number;          ///< Номер зоны (1, 2, 3...)
    bool is_configured;           ///< Флаг конфигурации зоны
    uint64_t configured_at;       ///< Timestamp конфигурации
} root_config_t;

/**
 * @brief ⭐ Инициализация Root Config
 * 
 * Загружает конфигурацию из NVS или создает дефолтную
 * 
 * @return ESP_OK при успехе
 */
esp_err_t root_config_init(void);

/**
 * @brief ⭐ Получение Root Node ID
 * 
 * @param root_id Буфер для Root Node ID (мин. 32 байта)
 * @return ESP_OK при успехе
 */
esp_err_t root_config_get_root_node_id(char *root_id);

/**
 * @brief ⭐ Установка Root Node ID
 * 
 * ВАЖНО: Должно быть уникальным для каждого Root Node!
 * 
 * @param root_id Root Node ID (например "root_001")
 * @return ESP_OK при успехе
 */
esp_err_t root_config_set_root_node_id(const char *root_id);

/**
 * @brief ⭐ Получение Mesh Network ID
 * 
 * @param mesh_id Буфер для Mesh Network ID (мин. 32 байта)
 * @return ESP_OK при успехе
 */
esp_err_t root_config_get_mesh_network_id(char *mesh_id);

/**
 * @brief ⭐ Установка Mesh Network ID
 * 
 * ВАЖНО: Должно быть уникальным для каждой зоны!
 * 
 * @param mesh_id Mesh Network ID (например "HYDRO1_ZONE1")
 * @return ESP_OK при успехе
 */
esp_err_t root_config_set_mesh_network_id(const char *mesh_id);

/**
 * @brief Получение настроек MQTT брокера
 */
esp_err_t root_config_get_mqtt_endpoint(char *host, size_t host_len, uint16_t *port);

/**
 * @brief Установка настроек MQTT брокера
 */
esp_err_t root_config_set_mqtt_endpoint(const char *host, uint16_t port);

/**
 * @brief ⭐ Получение MQTT Topic Prefix
 * 
 * @param prefix Буфер для MQTT prefix (мин. 64 байта)
 * @return ESP_OK при успехе
 */
esp_err_t root_config_get_mqtt_topic_prefix(char *prefix);

/**
 * @brief ⭐ Установка MQTT Topic Prefix
 * 
 * @param prefix MQTT prefix (например "hydro/zone1/")
 * @return ESP_OK при успехе
 */
esp_err_t root_config_set_mqtt_topic_prefix(const char *prefix);

/**
 * @brief ⭐ Получение номера зоны
 * 
 * @return Номер зоны (1, 2, 3...) или 0 если не настроено
 */
uint8_t root_config_get_zone_number(void);

/**
 * @brief ⭐ Установка номера зоны
 * 
 * @param zone_number Номер зоны (1, 2, 3...)
 * @return ESP_OK при успехе
 */
esp_err_t root_config_set_zone_number(uint8_t zone_number);

/**
 * @brief ⭐ Проверка конфигурации зоны
 * 
 * @return true если зона настроена, false если требуется настройка
 */
bool root_config_is_zone_configured(void);

/**
 * @brief ⭐ Полная настройка зоны
 * 
 * Устанавливает все параметры зоны за один раз
 * 
 * @param zone_number Номер зоны (1, 2, 3...)
 * @param zone_name Название зоны (опционально, может быть NULL)
 * @param zone_location Расположение зоны (опционально, может быть NULL)
 * @return ESP_OK при успехе
 */
esp_err_t root_config_configure_zone(uint8_t zone_number, const char *zone_name, const char *zone_location);

/**
 * @brief ⭐ Автоматическая настройка зоны по номеру
 * 
 * Генерирует root_node_id, mesh_network_id, mqtt_topic_prefix автоматически
 * 
 * Примеры:
 * - zone_number=1 → root_001, HYDRO1_ZONE1, hydro/zone1/
 * - zone_number=2 → root_002, HYDRO1_ZONE2, hydro/zone2/
 * 
 * @param zone_number Номер зоны (1-99)
 * @return ESP_OK при успехе
 */
esp_err_t root_config_auto_configure(uint8_t zone_number);

/**
 * @brief ⭐ Получение полной конфигурации зоны
 * 
 * @param config Указатель на структуру конфигурации
 * @return ESP_OK при успехе
 */
esp_err_t root_config_get(root_config_t *config);

/**
 * @brief ⭐ Сохранение полной конфигурации зоны
 * 
 * @param config Указатель на структуру конфигурации
 * @return ESP_OK при успехе
 */
esp_err_t root_config_save(const root_config_t *config);

/**
 * @brief ⭐ Сброс конфигурации зоны (factory reset)
 * 
 * @return ESP_OK при успехе
 */
esp_err_t root_config_reset(void);

/**
 * @brief ⭐ Вывод конфигурации зоны в лог
 */
void root_config_print(void);

#ifdef __cplusplus
}
#endif

#endif // ROOT_CONFIG_H

