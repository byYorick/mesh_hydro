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
    uint8_t max_connection;         ///< Макс. подключений (для ROOT AP)
    const char *router_ssid;        ///< SSID роутера (ТОЛЬКО ДЛЯ ROOT! NODE = NULL)
    const char *router_password;    ///< Пароль роутера (ТОЛЬКО ДЛЯ ROOT! NODE = NULL)
    const uint8_t *router_bssid;    ///< BSSID роутера (NULL=auto, только для ROOT)
} mesh_manager_config_t;

/**
 * @brief Информация об узле mesh сети
 */
typedef struct {
    uint8_t mac[6];     ///< MAC адрес узла
    int8_t rssi;        ///< RSSI (сила сигнала) к родительскому узлу
    uint8_t layer;      ///< Уровень в mesh дереве (1=ROOT, 2=дети ROOT, и т.д.)
} mesh_node_info_t;

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

/**
 * @brief Получение таблицы маршрутизации с RSSI (для ROOT)
 * 
 * Заполняет массив информацией о всех подключенных узлах mesh сети,
 * включая MAC адрес, RSSI и уровень в дереве.
 * 
 * @param nodes Массив для заполнения (должен быть выделен вызывающей стороной)
 * @param max_count Максимальный размер массива
 * @param actual_count Указатель для записи фактического количества узлов
 * @return ESP_OK при успехе, ESP_ERR_NOT_SUPPORTED если не ROOT
 * 
 * Пример использования:
 * @code
 * mesh_node_info_t nodes[50];
 * int count = 0;
 * esp_err_t ret = mesh_manager_get_routing_table_with_rssi(nodes, 50, &count);
 * if (ret == ESP_OK) {
 *     for (int i = 0; i < count; i++) {
 *         ESP_LOGI(TAG, "Node %d: "MACSTR" RSSI=%d Layer=%d",
 *                  i, MAC2STR(nodes[i].mac), nodes[i].rssi, nodes[i].layer);
 *     }
 * }
 * @endcode
 */
esp_err_t mesh_manager_get_routing_table_with_rssi(mesh_node_info_t *nodes, int max_count, int *actual_count);

/**
 * @brief Получение RSSI к родительскому узлу (для NODE)
 * 
 * @return RSSI в dBm (от -100 до 0), 0 если не подключен или является ROOT
 */
int8_t mesh_manager_get_parent_rssi(void);

#ifdef __cplusplus
}
#endif

#endif // MESH_MANAGER_H

