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
 * @brief Базовая структура сообщения
 */
typedef struct {
    mesh_msg_type_t type;
    char node_id[32];
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
 * @brief Создание JSON строки телеметрии
 * 
 * @param node_id ID узла
 * @param node_type Тип узла (climate, ph, ec, display и т.д.)
 * @param data cJSON объект с данными
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_telemetry(const char *node_id, const char *node_type, cJSON *data, char *out_json, size_t max_len);

/**
 * @brief Создание JSON строки команды
 * 
 * @param node_id ID целевого узла
 * @param command Команда (например "run_pump")
 * @param params cJSON объект с параметрами
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_command(const char *node_id, const char *command, cJSON *params, char *out_json, size_t max_len);

/**
 * @brief Создание JSON строки конфигурации
 * 
 * @param node_id ID целевого узла
 * @param config cJSON объект с конфигурацией
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_config(const char *node_id, cJSON *config, char *out_json, size_t max_len);

/**
 * @brief Создание JSON строки события
 * 
 * @param node_id ID узла
 * @param level Уровень события
 * @param message Текст сообщения
 * @param data cJSON объект с дополнительными данными (может быть NULL)
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_event(const char *node_id, mesh_event_level_t level, const char *message, cJSON *data, char *out_json, size_t max_len);

/**
 * @brief Создание JSON строки heartbeat
 * 
 * @param node_id ID узла
 * @param node_type Тип узла (climate, ph, ec, display и т.д.)
 * @param uptime Время работы (секунды)
 * @param heap_free Свободная память (байты)
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_heartbeat(const char *node_id, const char *node_type, uint32_t uptime, uint32_t heap_free, char *out_json, size_t max_len);

/**
 * @brief Создание JSON строки запроса
 * 
 * @param from_id ID отправителя
 * @param request Тип запроса (например "all_nodes_data")
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_request(const char *from_id, const char *request, char *out_json, size_t max_len);

/**
 * @brief Создание JSON строки ответа
 * 
 * @param to_id ID получателя
 * @param data cJSON объект с данными ответа
 * @param out_json Буфер для JSON строки
 * @param max_len Размер буфера
 * @return true при успехе
 */
bool mesh_protocol_create_response(const char *to_id, cJSON *data, char *out_json, size_t max_len);

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

#ifdef __cplusplus
}
#endif

#endif // MESH_PROTOCOL_H

