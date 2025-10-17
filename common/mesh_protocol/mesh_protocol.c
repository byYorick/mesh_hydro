/**
 * @file mesh_protocol.c
 * @brief Реализация протокола mesh обмена
 */

#include "mesh_protocol.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "mesh_protocol";

// Константы типов сообщений
static const char *MSG_TYPE_TELEMETRY = "telemetry";
static const char *MSG_TYPE_COMMAND = "command";
static const char *MSG_TYPE_CONFIG = "config";
static const char *MSG_TYPE_EVENT = "event";
static const char *MSG_TYPE_HEARTBEAT = "heartbeat";
static const char *MSG_TYPE_REQUEST = "request";
static const char *MSG_TYPE_RESPONSE = "response";

// Константы уровней событий
static const char *EVENT_LEVEL_INFO = "info";
static const char *EVENT_LEVEL_WARNING = "warning";
static const char *EVENT_LEVEL_CRITICAL = "critical";
static const char *EVENT_LEVEL_EMERGENCY = "emergency";

static mesh_msg_type_t str_to_msg_type(const char *str) {
    if (strcmp(str, MSG_TYPE_TELEMETRY) == 0) return MESH_MSG_TELEMETRY;
    if (strcmp(str, MSG_TYPE_COMMAND) == 0) return MESH_MSG_COMMAND;
    if (strcmp(str, MSG_TYPE_CONFIG) == 0) return MESH_MSG_CONFIG;
    if (strcmp(str, MSG_TYPE_EVENT) == 0) return MESH_MSG_EVENT;
    if (strcmp(str, MSG_TYPE_HEARTBEAT) == 0) return MESH_MSG_HEARTBEAT;
    if (strcmp(str, MSG_TYPE_REQUEST) == 0) return MESH_MSG_REQUEST;
    if (strcmp(str, MSG_TYPE_RESPONSE) == 0) return MESH_MSG_RESPONSE;
    return MESH_MSG_UNKNOWN;
}

static const char* msg_type_to_str(mesh_msg_type_t type) {
    switch (type) {
        case MESH_MSG_TELEMETRY: return MSG_TYPE_TELEMETRY;
        case MESH_MSG_COMMAND: return MSG_TYPE_COMMAND;
        case MESH_MSG_CONFIG: return MSG_TYPE_CONFIG;
        case MESH_MSG_EVENT: return MSG_TYPE_EVENT;
        case MESH_MSG_HEARTBEAT: return MSG_TYPE_HEARTBEAT;
        case MESH_MSG_REQUEST: return MSG_TYPE_REQUEST;
        case MESH_MSG_RESPONSE: return MSG_TYPE_RESPONSE;
        default: return "unknown";
    }
}

bool mesh_protocol_parse(const char *json_str, mesh_message_t *msg) {
    if (json_str == NULL || msg == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return false;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "JSON parse error");
        return false;
    }

    // Парсинг type
    cJSON *type_obj = cJSON_GetObjectItem(root, "type");
    if (type_obj == NULL || !cJSON_IsString(type_obj)) {
        ESP_LOGE(TAG, "Missing or invalid 'type' field");
        cJSON_Delete(root);
        return false;
    }
    msg->type = str_to_msg_type(type_obj->valuestring);

    // Парсинг node_id
    cJSON *node_id_obj = cJSON_GetObjectItem(root, "node_id");
    if (node_id_obj != NULL && cJSON_IsString(node_id_obj)) {
        strncpy(msg->node_id, node_id_obj->valuestring, sizeof(msg->node_id) - 1);
    } else {
        msg->node_id[0] = '\0';
    }

    // Парсинг timestamp
    cJSON *timestamp_obj = cJSON_GetObjectItem(root, "timestamp");
    if (timestamp_obj != NULL && cJSON_IsNumber(timestamp_obj)) {
        msg->timestamp = (uint64_t)timestamp_obj->valuedouble;
    } else {
        msg->timestamp = mesh_protocol_get_timestamp();
    }

    // Парсинг data (детальные данные зависят от типа сообщения)
    cJSON *data_obj = cJSON_GetObjectItem(root, "data");
    if (data_obj != NULL) {
        msg->data = cJSON_Duplicate(data_obj, true);
    } else {
        msg->data = NULL;
    }

    cJSON_Delete(root);
    return true;
}

bool mesh_protocol_create_telemetry(const char *node_id, cJSON *data, char *out_json, size_t max_len) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "type", MSG_TYPE_TELEMETRY);
    cJSON_AddStringToObject(root, "node_id", node_id);
    cJSON_AddNumberToObject(root, "timestamp", (double)mesh_protocol_get_timestamp());
    
    if (data != NULL) {
        cJSON_AddItemToObject(root, "data", cJSON_Duplicate(data, true));
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return false;
    }

    if (strlen(json_str) >= max_len) {
        ESP_LOGW(TAG, "JSON too large: %d bytes", strlen(json_str));
        free(json_str);
        cJSON_Delete(root);
        return false;
    }

    strcpy(out_json, json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return true;
}

bool mesh_protocol_create_command(const char *node_id, const char *command, cJSON *params, char *out_json, size_t max_len) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "type", MSG_TYPE_COMMAND);
    cJSON_AddStringToObject(root, "node_id", node_id);
    cJSON_AddNumberToObject(root, "timestamp", (double)mesh_protocol_get_timestamp());
    cJSON_AddStringToObject(root, "command", command);
    
    if (params != NULL) {
        cJSON_AddItemToObject(root, "params", cJSON_Duplicate(params, true));
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return false;
    }

    if (strlen(json_str) >= max_len) {
        ESP_LOGW(TAG, "JSON too large: %d bytes", strlen(json_str));
        free(json_str);
        cJSON_Delete(root);
        return false;
    }

    strcpy(out_json, json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return true;
}

bool mesh_protocol_create_config(const char *node_id, cJSON *config, char *out_json, size_t max_len) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "type", MSG_TYPE_CONFIG);
    cJSON_AddStringToObject(root, "node_id", node_id);
    cJSON_AddNumberToObject(root, "timestamp", (double)mesh_protocol_get_timestamp());
    
    if (config != NULL) {
        cJSON_AddItemToObject(root, "config", cJSON_Duplicate(config, true));
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return false;
    }

    if (strlen(json_str) >= max_len) {
        ESP_LOGW(TAG, "JSON too large: %d bytes", strlen(json_str));
        free(json_str);
        cJSON_Delete(root);
        return false;
    }

    strcpy(out_json, json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return true;
}

bool mesh_protocol_create_event(const char *node_id, mesh_event_level_t level, const char *message, cJSON *data, char *out_json, size_t max_len) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "type", MSG_TYPE_EVENT);
    cJSON_AddStringToObject(root, "node_id", node_id);
    cJSON_AddNumberToObject(root, "timestamp", (double)mesh_protocol_get_timestamp());
    cJSON_AddStringToObject(root, "level", mesh_protocol_event_level_to_str(level));
    cJSON_AddStringToObject(root, "message", message);
    
    if (data != NULL) {
        cJSON_AddItemToObject(root, "data", cJSON_Duplicate(data, true));
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return false;
    }

    if (strlen(json_str) >= max_len) {
        ESP_LOGW(TAG, "JSON too large: %d bytes", strlen(json_str));
        free(json_str);
        cJSON_Delete(root);
        return false;
    }

    strcpy(out_json, json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return true;
}

bool mesh_protocol_create_heartbeat(const char *node_id, uint32_t uptime, uint32_t heap_free, char *out_json, size_t max_len) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "type", MSG_TYPE_HEARTBEAT);
    cJSON_AddStringToObject(root, "node_id", node_id);
    cJSON_AddNumberToObject(root, "timestamp", (double)mesh_protocol_get_timestamp());
    cJSON_AddNumberToObject(root, "uptime", uptime);
    cJSON_AddNumberToObject(root, "heap_free", heap_free);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return false;
    }

    if (strlen(json_str) >= max_len) {
        ESP_LOGW(TAG, "JSON too large: %d bytes", strlen(json_str));
        free(json_str);
        cJSON_Delete(root);
        return false;
    }

    strcpy(out_json, json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return true;
}

bool mesh_protocol_create_request(const char *from_id, const char *request, char *out_json, size_t max_len) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "type", MSG_TYPE_REQUEST);
    cJSON_AddStringToObject(root, "from", from_id);
    cJSON_AddStringToObject(root, "request", request);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return false;
    }

    if (strlen(json_str) >= max_len) {
        ESP_LOGW(TAG, "JSON too large: %d bytes", strlen(json_str));
        free(json_str);
        cJSON_Delete(root);
        return false;
    }

    strcpy(out_json, json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return true;
}

bool mesh_protocol_create_response(const char *to_id, cJSON *data, char *out_json, size_t max_len) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "type", MSG_TYPE_RESPONSE);
    cJSON_AddStringToObject(root, "to", to_id);
    
    if (data != NULL) {
        cJSON_AddItemToObject(root, "data", cJSON_Duplicate(data, true));
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return false;
    }

    if (strlen(json_str) >= max_len) {
        ESP_LOGW(TAG, "JSON too large: %d bytes", strlen(json_str));
        free(json_str);
        cJSON_Delete(root);
        return false;
    }

    strcpy(out_json, json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return true;
}

void mesh_protocol_free_message(mesh_message_t *msg) {
    if (msg != NULL && msg->data != NULL) {
        cJSON_Delete(msg->data);
        msg->data = NULL;
    }
}

uint64_t mesh_protocol_get_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec;
}

const char* mesh_protocol_event_level_to_str(mesh_event_level_t level) {
    switch (level) {
        case MESH_EVENT_INFO: return EVENT_LEVEL_INFO;
        case MESH_EVENT_WARNING: return EVENT_LEVEL_WARNING;
        case MESH_EVENT_CRITICAL: return EVENT_LEVEL_CRITICAL;
        case MESH_EVENT_EMERGENCY: return EVENT_LEVEL_EMERGENCY;
        default: return "unknown";
    }
}

