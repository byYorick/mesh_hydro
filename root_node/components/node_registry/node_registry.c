/**
 * @file node_registry.c
 * @brief Реализация реестра узлов
 */

#include "node_registry.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>

static const char *TAG = "node_registry";

#define MAX_NODES 32
#define NODE_TIMEOUT_MS 30000

static node_info_t s_nodes[MAX_NODES];
static int s_node_count = 0;

esp_err_t node_registry_init(void) {
    memset(s_nodes, 0, sizeof(s_nodes));
    s_node_count = 0;
    ESP_LOGI(TAG, "Node registry initialized");
    return ESP_OK;
}

esp_err_t node_registry_add(const node_info_t *node) {
    if (s_node_count >= MAX_NODES) {
        ESP_LOGE(TAG, "Registry full, cannot add node");
        return ESP_ERR_NO_MEM;
    }
    
    // Проверка дубликатов
    for (int i = 0; i < s_node_count; i++) {
        if (strcmp(s_nodes[i].node_id, node->node_id) == 0) {
            ESP_LOGW(TAG, "Node %s already exists", node->node_id);
            return ESP_ERR_INVALID_STATE;
        }
    }
    
    memcpy(&s_nodes[s_node_count], node, sizeof(node_info_t));
    s_node_count++;
    
    ESP_LOGI(TAG, "Node added: %s (type: %s, zone: %s)", 
             node->node_id, node->node_type, node->zone);
    
    return ESP_OK;
}

esp_err_t node_registry_update_last_seen(const char *node_id) {
    for (int i = 0; i < s_node_count; i++) {
        if (strcmp(s_nodes[i].node_id, node_id) == 0) {
            s_nodes[i].last_seen = esp_timer_get_time() / 1000;  // в миллисекундах
            s_nodes[i].online = true;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t node_registry_update_telemetry(const char *node_id, void *telemetry) {
    for (int i = 0; i < s_node_count; i++) {
        if (strcmp(s_nodes[i].node_id, node_id) == 0) {
            s_nodes[i].last_telemetry = telemetry;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t node_registry_remove(const char *node_id) {
    for (int i = 0; i < s_node_count; i++) {
        if (strcmp(s_nodes[i].node_id, node_id) == 0) {
            // Сдвиг массива
            for (int j = i; j < s_node_count - 1; j++) {
                s_nodes[j] = s_nodes[j + 1];
            }
            s_node_count--;
            ESP_LOGI(TAG, "Node removed: %s", node_id);
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

node_info_t* node_registry_get(const char *node_id) {
    for (int i = 0; i < s_node_count; i++) {
        if (strcmp(s_nodes[i].node_id, node_id) == 0) {
            return &s_nodes[i];
        }
    }
    return NULL;
}

node_info_t* node_registry_get_all(int *count) {
    *count = s_node_count;
    return s_nodes;
}

int node_registry_get_count(void) {
    return s_node_count;
}

void node_registry_check_timeouts(void) {
    uint64_t now = esp_timer_get_time() / 1000;
    
    for (int i = 0; i < s_node_count; i++) {
        if (s_nodes[i].online && (now - s_nodes[i].last_seen > NODE_TIMEOUT_MS)) {
            s_nodes[i].online = false;
            ESP_LOGW(TAG, "Node %s timeout (offline)", s_nodes[i].node_id);
        }
    }
}

