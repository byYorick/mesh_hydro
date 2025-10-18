/**
 * @file node_registry.c
 * @brief Реализация реестра узлов
 */

#include "node_registry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include <string.h>

static const char *TAG = "node_registry";

// Внутренний массив узлов
static node_info_t s_nodes[MAX_NODES];
static int s_node_count = 0;

esp_err_t node_registry_init(void) {
    memset(s_nodes, 0, sizeof(s_nodes));
    s_node_count = 0;
    
    ESP_LOGI(TAG, "Node Registry initialized (max %d nodes)", MAX_NODES);
    return ESP_OK;
}

void node_registry_update_last_seen(const char *node_id, const uint8_t *mac_addr) {
    if (!node_id || !mac_addr) {
        return;
    }

    // Поиск существующего узла
    node_info_t *node = NULL;
    for (int i = 0; i < s_node_count; i++) {
        if (strcmp(s_nodes[i].node_id, node_id) == 0) {
            node = &s_nodes[i];
            break;
        }
    }

    // Если не найден - добавить новый
    if (node == NULL) {
        if (s_node_count >= MAX_NODES) {
            ESP_LOGW(TAG, "Registry full, cannot add node %s", node_id);
            return;
        }

        node = &s_nodes[s_node_count++];
        strncpy(node->node_id, node_id, sizeof(node->node_id) - 1);
        memcpy(node->mac_addr, mac_addr, 6);
        node->last_data = NULL;

        ESP_LOGI(TAG, "New node added: %s ("MACSTR")", 
                 node_id, MAC2STR(mac_addr));
    }

    // Обновление статуса
    bool was_offline = !node->online;
    node->online = true;
    node->last_seen_ms = esp_timer_get_time() / 1000;

    if (was_offline) {
        ESP_LOGI(TAG, "Node %s is now ONLINE", node_id);
    }
}

void node_registry_update_data(const char *node_id, cJSON *data) {
    if (!node_id || !data) {
        return;
    }

    node_info_t *node = node_registry_get(node_id);
    if (!node) {
        ESP_LOGW(TAG, "Cannot update data: node %s not found", node_id);
        return;
    }

    // Освобождение старых данных
    if (node->last_data) {
        cJSON_Delete(node->last_data);
    }

    // Копирование новых данных
    node->last_data = cJSON_Duplicate(data, true);

    // Извлечение типа и зоны если есть
    cJSON *type = cJSON_GetObjectItem(data, "node_type");
    if (type && cJSON_IsString(type)) {
        strncpy(node->node_type, type->valuestring, sizeof(node->node_type) - 1);
    }

    cJSON *zone = cJSON_GetObjectItem(data, "zone");
    if (zone && cJSON_IsString(zone)) {
        strncpy(node->zone, zone->valuestring, sizeof(node->zone) - 1);
    }
}

void node_registry_check_timeouts(void) {
    uint64_t now_ms = esp_timer_get_time() / 1000;

    for (int i = 0; i < s_node_count; i++) {
        if (s_nodes[i].online) {
            uint64_t elapsed = now_ms - s_nodes[i].last_seen_ms;

            if (elapsed > NODE_TIMEOUT_MS) {
                s_nodes[i].online = false;
                ESP_LOGW(TAG, "Node %s TIMEOUT -> OFFLINE (elapsed: %llu ms)", 
                         s_nodes[i].node_id, elapsed);
            }
        }
    }
}

node_info_t* node_registry_get(const char *node_id) {
    if (!node_id) {
        return NULL;
    }

    for (int i = 0; i < s_node_count; i++) {
        if (strcmp(s_nodes[i].node_id, node_id) == 0) {
            return &s_nodes[i];
        }
    }

    return NULL;
}

int node_registry_get_count(void) {
    int count = 0;
    for (int i = 0; i < s_node_count; i++) {
        if (s_nodes[i].online) {
            count++;
        }
    }
    return count;
}

cJSON* node_registry_export_all_to_json(void) {
    cJSON *root = cJSON_CreateArray();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON array");
        return NULL;
    }

    for (int i = 0; i < s_node_count; i++) {
        if (s_nodes[i].online) {
            cJSON *node_obj = cJSON_CreateObject();
            if (!node_obj) continue;

            cJSON_AddStringToObject(node_obj, "node_id", s_nodes[i].node_id);
            cJSON_AddStringToObject(node_obj, "node_type", s_nodes[i].node_type);
            cJSON_AddStringToObject(node_obj, "zone", s_nodes[i].zone);
            cJSON_AddBoolToObject(node_obj, "online", s_nodes[i].online);

            // MAC адрес
            char mac_str[18];
            snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(s_nodes[i].mac_addr));
            cJSON_AddStringToObject(node_obj, "mac_addr", mac_str);

            // Последние данные
            if (s_nodes[i].last_data) {
                cJSON_AddItemToObject(node_obj, "data", 
                                     cJSON_Duplicate(s_nodes[i].last_data, true));
            }

            // Время последнего контакта
            uint64_t now_ms = esp_timer_get_time() / 1000;
            uint64_t seconds_ago = (now_ms - s_nodes[i].last_seen_ms) / 1000;
            cJSON_AddNumberToObject(node_obj, "last_seen_seconds_ago", (double)seconds_ago);

            cJSON_AddItemToArray(root, node_obj);
        }
    }

    return root;
}

int node_registry_get_all(node_info_t *nodes) {
    if (!nodes) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < s_node_count && count < MAX_NODES; i++) {
        if (s_nodes[i].online) {
            memcpy(&nodes[count], &s_nodes[i], sizeof(node_info_t));
            // Не копируем указатель на cJSON, чтобы избежать двойного освобождения
            nodes[count].last_data = NULL;
            count++;
        }
    }

    return count;
}

bool node_registry_has_type(const char *node_type) {
    if (!node_type) {
        return false;
    }

    for (int i = 0; i < s_node_count; i++) {
        if (s_nodes[i].online && strcmp(s_nodes[i].node_type, node_type) == 0) {
            return true;
        }
    }

    return false;
}

