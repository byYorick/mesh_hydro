/**
 * @file mesh_protocol.h
 * @brief Протокол JSON обмена данными в mesh-сети
 */

#ifndef MESH_PROTOCOL_H
#define MESH_PROTOCOL_H

#include "cJSON.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Типы сообщений
 */
typedef enum {
    MESH_MSG_TELEMETRY = 0,  ///< Телеметрия (NODE → ROOT)
    MESH_MSG_COMMAND,        ///< Команда (ROOT → NODE)
    MESH_MSG_CONFIG,         ///< Конфигурация (ROOT → NODE)
    MESH_MSG_EVENT,          ///< Событие (NODE → ROOT)
    MESH_MSG_HEARTBEAT,      ///< Heartbeat (NODE → ROOT)
    MESH_MSG_REQUEST,        ///< Запрос данных (Display → ROOT)
    MESH_MSG_RESPONSE,       ///< Ответ на запрос (ROOT → Display)
    MESH_MSG_DISCOVERY,      ///< Discovery (NODE → ROOT для регистрации)
    MESH_MSG_CONFIG_CONFIRMATION, ///< Подтверждение конфигурации (NODE → ROOT)
    MESH_MSG_UNKNOWN         ///< Неизвестный тип
} mesh_msg_type_t;

/**
 * @brief Уровни событий
 */
typedef enum {
    MESH_EVENT_INFO = 0,     ///< Информация
    MESH_EVENT_WARNING,      ///< Предупреждение
    MESH_EVENT_CRITICAL,     ///< Критично
    MESH_EVENT_EMERGENCY     ///< Авария
} mesh_event_level_t;

/**
 * @brief ⭐ Базовая структура сообщения (с зонированием)
 */
typedef struct {
    mesh_msg_type_t type;
    char node_id[32];
    char root_node_id[32];  // ⭐ ЗОНИРОВАНИЕ: ID Root Node (зоны)
    char mesh_network_id[32];  // ⭐ ЗОНИРОВАНИЕ: mesh-network / topic segment
    uint64_t timestamp;
    cJSON *data;  // Дополнительные данные (зависят от типа)
} mesh_message_t;

/**
 * @brief Парсинг JSON строки в структуру сообщения
 * 
 * @param json_str JSON строка
 * @param msg Указатель на структуру для заполнения
 * @return true при успехе
 */
bool mesh_protocol_parse(const char *json_str, mesh_message_t *msg);

/**
 * @brief Предварительная валидация структуры JSON перед парсингом
 *
 * @param json_str JSON строка
 * @param error_ptr [out] указатель на позицию ошибки (может быть NULL)
 * @return true если JSON корректен
 */
bool mesh_protocol_validate_structure(const char *json_str, const char **error_ptr);

/**
 * @brief ⭐ Создание JSON строки телеметрии (с зонированием)
 * 
 * @param node_id ID узла
 * @param root_node_id ⭐ ID Root Node (зоны)
 * @param node_type Тип узла (climate, ph, ec, display и т.д.)
 * @param data cJSON объект с данными
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_telemetry(const char *node_id, const char *root_node_id, const char *mesh_network_id, const char *node_type, cJSON *data, char *out_json, size_t max_len);

/**
 * @brief ⭐ Создание JSON строки команды (с зонированием)
 * 
 * @param node_id ID целевого узла
 * @param root_node_id ⭐ ID Root Node (зоны)
 * @param command Команда (например "run_pump")
 * @param params cJSON объект с параметрами
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_command(const char *node_id, const char *root_node_id, const char *mesh_network_id, const char *command, cJSON *params, char *out_json, size_t max_len);

/**
 * @brief ⭐ Создание JSON строки конфигурации (с зонированием)
 * 
 * @param node_id ID целевого узла
 * @param root_node_id ⭐ ID Root Node (зоны)
 * @param config cJSON объект с конфигурацией
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_config(const char *node_id, const char *root_node_id, const char *mesh_network_id, cJSON *config, char *out_json, size_t max_len);

/**
 * @brief ⭐ Создание JSON строки события (с зонированием)
 * 
 * @param node_id ID узла
 * @param root_node_id ⭐ ID Root Node (зоны)
 * @param level Уровень события
 * @param message Текст сообщения
 * @param data cJSON объект с дополнительными данными (может быть NULL)
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_event(const char *node_id, const char *root_node_id, const char *mesh_network_id, mesh_event_level_t level, const char *message, cJSON *data, char *out_json, size_t max_len);

/**
 * @brief ⭐ Создание JSON строки heartbeat (с зонированием)
 * 
 * @param node_id ID узла
 * @param root_node_id ⭐ ID Root Node (зоны)
 * @param node_type Тип узла (climate, ph, ec, display и т.д.)
 * @param uptime Время работы (секунды)
 * @param heap_free Свободная память (байты)
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_heartbeat(const char *node_id, const char *root_node_id, const char *mesh_network_id, const char *node_type, uint32_t uptime, uint32_t heap_free, char *out_json, size_t max_len);

/**
 * @brief ⭐ Создание JSON строки запроса (с зонированием)
 * 
 * @param from_id ID отправителя
 * @param root_node_id ⭐ ID Root Node (зоны)
 * @param request Тип запроса (например "all_nodes_data")
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_request(const char *from_id, const char *root_node_id, const char *mesh_network_id, const char *request, char *out_json, size_t max_len);

/**
 * @brief ⭐ Создание JSON строки ответа (с зонированием)
 * 
 * @param to_id ID получателя
 * @param root_node_id ⭐ ID Root Node (зоны)
 * @param data cJSON объект с данными ответа
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_response(const char *to_id, const char *root_node_id, const char *mesh_network_id, cJSON *data, char *out_json, size_t max_len);

/**
 * @brief Освобождение ресурсов сообщения
 * 
 * @param msg Указатель на сообщение
 */
void mesh_protocol_free_message(mesh_message_t *msg);

/**
 * @brief Получение текущего timestamp (Unix time)
 * 
 * @return Timestamp в секундах
 */
uint64_t mesh_protocol_get_timestamp(void);

/**
 * @brief Преобразование уровня события в строку
 * 
 * @param level Уровень события
 * @return Строка уровня
 */
const char* mesh_protocol_event_level_to_str(mesh_event_level_t level);

/**
 * @brief Сформировать MQTT топик в мультизонном формате
 * 
 * Формат: hydro/{mesh_id}/{msg_type}/{node_id}
 * 
 * @param out Буфер для результата
 * @param max_len Размер буфера
 * @param mesh_id Идентификатор mesh (NULL = использовать zone_config_get_mesh_id())
 * @param msg_type Тип сообщения ("heartbeat", "telemetry", "event", "command", "config", "discovery", "config_response")
 * @param node_id Идентификатор узла
 * @return true если топик успешно сформирован, false если буфер мал или параметры невалидны
 * 
 * Примеры:
 * @code
 * char topic[192];
 * mesh_topic_format(topic, sizeof(topic), "zone_greenhouse_1", "heartbeat", "climate_001");
 * // Результат: "hydro/zone_greenhouse_1/heartbeat/climate_001"
 * 
 * // Использовать mesh_id из zone_config
 * mesh_topic_format(topic, sizeof(topic), NULL, "telemetry", "ph_001");
 * @endcode
 */
bool mesh_topic_format(char *out, size_t max_len, 
                       const char *mesh_id, 
                       const char *msg_type, 
                       const char *node_id);

#ifdef __cplusplus
}
#endif

#endif // MESH_PROTOCOL_H

