/**
 * @file node_registry.h
 * @brief Реестр всех узлов mesh-сети
 * 
 * Отслеживает статус всех подключенных узлов, их MAC адреса,
 * последние данные и проверяет таймауты.
 */

#ifndef NODE_REGISTRY_H
#define NODE_REGISTRY_H

#include "esp_err.h"
#include "cJSON.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NODES 20
#define NODE_TIMEOUT_MS 40000  // 40 секунд (больше чем интервал телеметрии 30 сек)

/**
 * @brief Информация об узле
 */
typedef struct {
    char node_id[32];           ///< ID узла ("ph_ec_001")
    uint8_t mac_addr[6];        ///< MAC адрес
    char node_type[16];         ///< Тип узла ("ph_ec", "climate", etc)
    char zone[32];              ///< Зона/помещение
    bool online;                ///< Статус онлайн
    uint64_t last_seen_ms;      ///< Время последнего контакта (мс)
    cJSON *last_data;           ///< Последние данные от узла
} node_info_t;

/**
 * @brief Инициализация реестра узлов
 * 
 * @return ESP_OK при успехе
 */
esp_err_t node_registry_init(void);

/**
 * @brief Обновление времени последнего контакта с узлом
 * 
 * Если узел не существует - добавляет его в реестр.
 * 
 * @param node_id ID узла
 * @param mac_addr MAC адрес узла
 */
void node_registry_update_last_seen(const char *node_id, const uint8_t *mac_addr);

/**
 * @brief Обновление данных узла
 * 
 * @param node_id ID узла
 * @param data cJSON объект с данными (будет скопирован)
 */
void node_registry_update_data(const char *node_id, cJSON *data);

/**
 * @brief Проверка таймаутов всех узлов
 * 
 * Помечает узлы как offline если не было контакта > NODE_TIMEOUT_MS
 */
void node_registry_check_timeouts(void);

/**
 * @brief Получение информации об узле
 * 
 * @param node_id ID узла
 * @return Указатель на node_info_t или NULL если не найден
 */
node_info_t* node_registry_get(const char *node_id);

/**
 * @brief Получение количества онлайн узлов
 * 
 * @return Количество узлов со статусом online
 */
int node_registry_get_count(void);

/**
 * @brief Экспорт всех узлов в JSON массив
 * 
 * @return cJSON массив с данными всех узлов (нужно освободить через cJSON_Delete)
 */
cJSON* node_registry_export_all_to_json(void);

/**
 * @brief Получение списка всех узлов
 * 
 * @param nodes Массив для заполнения (размер MAX_NODES)
 * @return Количество заполненных элементов
 */
int node_registry_get_all(node_info_t *nodes);

/**
 * @brief Проверка, существует ли узел с данным типом
 * 
 * @param node_type Тип узла ("climate", "ph_ec", etc)
 * @return true если узел данного типа есть и online
 */
bool node_registry_has_type(const char *node_type);

#ifdef __cplusplus
}
#endif

#endif // NODE_REGISTRY_H

