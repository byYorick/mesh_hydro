/**
 * @file mesh_manager.h
 * @brief ESP-WIFI-MESH управление для ROOT и NODE режимов
 * 
 * Обеспечивает единый API для работы с mesh-сетью независимо от режима узла.
 */

#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include "esp_mesh.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Режим работы узла
 */
typedef enum {
    MESH_MODE_ROOT = 0,     ///< ROOT узел (координатор)
    MESH_MODE_NODE          ///< Обычный NODE узел
} mesh_mode_t;

/**
 * @brief Конфигурация mesh-менеджера
 */
typedef struct {
    mesh_mode_t mode;               ///< Режим работы (ROOT или NODE)
    const char *mesh_id;            ///< ID mesh-сети (6 байт)
    const char *mesh_password;      ///< Пароль mesh-сети
    uint8_t channel;                ///< WiFi канал (1-13, 0=auto)
    uint8_t max_connection;         ///< Макс. подключений (для ROOT)
    const char *router_ssid;        ///< SSID роутера (нужен для ROOT и NODE!)
    const char *router_password;    ///< Пароль роутера (нужен для ROOT и NODE!)
    const uint8_t *router_bssid;    ///< BSSID роутера (NULL=auto)
} mesh_manager_config_t;

/**
 * @brief Callback для приема данных из mesh
 * 
 * @param src_addr MAC адрес отправителя
 * @param data Указатель на данные
 * @param len Длина данных
 */
typedef void (*mesh_recv_cb_t)(const uint8_t *src_addr, const uint8_t *data, size_t len);

/**
 * @brief Инициализация mesh-менеджера
 * 
 * @param config Конфигурация
 * @return ESP_OK при успехе
 */
esp_err_t mesh_manager_init(const mesh_manager_config_t *config);

/**
 * @brief Запуск mesh-сети
 * 
 * @return ESP_OK при успехе
 */
esp_err_t mesh_manager_start(void);

/**
 * @brief Остановка mesh-сети
 * 
 * @return ESP_OK при успехе
 */
esp_err_t mesh_manager_stop(void);

/**
 * @brief Отправка данных в mesh
 * 
 * @param dest_addr MAC адрес получателя (NULL для ROOT)
 * @param data Указатель на данные
 * @param len Длина данных
 * @return ESP_OK при успехе
 */
esp_err_t mesh_manager_send(const uint8_t *dest_addr, const uint8_t *data, size_t len);

/**
 * @brief Отправка данных на ROOT (для NODE)
 * 
 * @param data Указатель на данные
 * @param len Длина данных
 * @return ESP_OK при успехе
 */
esp_err_t mesh_manager_send_to_root(const uint8_t *data, size_t len);

/**
 * @brief Broadcast данных всем узлам (для ROOT)
 * 
 * @param data Указатель на данные
 * @param len Длина данных
 * @return ESP_OK при успехе
 */
esp_err_t mesh_manager_broadcast(const uint8_t *data, size_t len);

/**
 * @brief Регистрация callback для приема данных
 * 
 * @param cb Callback функция
 */
void mesh_manager_register_recv_cb(mesh_recv_cb_t cb);

/**
 * @brief Проверка, является ли узел ROOT
 * 
 * @return true если ROOT
 */
bool mesh_manager_is_root(void);

/**
 * @brief Проверка подключения к mesh
 * 
 * @return true если подключен
 */
bool mesh_manager_is_connected(void);

/**
 * @brief Получение количества подключенных узлов (для ROOT)
 * 
 * @return Количество узлов
 */
int mesh_manager_get_total_nodes(void);

/**
 * @brief Получение MAC адреса узла
 * 
 * @param mac Буфер для MAC адреса (6 байт)
 * @return ESP_OK при успехе
 */
esp_err_t mesh_manager_get_mac(uint8_t *mac);

#ifdef __cplusplus
}
#endif

#endif // MESH_MANAGER_H

