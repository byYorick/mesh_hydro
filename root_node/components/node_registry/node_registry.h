/**
 * @file node_registry.h
 * @brief Реестр всех узлов в mesh-сети
 */

#ifndef NODE_REGISTRY_H
#define NODE_REGISTRY_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char node_id[32];
    uint8_t mac_addr[6];
    char node_type[16];
    char zone[32];
    bool online;
    uint64_t last_seen;
    void *last_telemetry;  // cJSON объект
} node_info_t;

esp_err_t node_registry_init(void);
esp_err_t node_registry_add(const node_info_t *node);
esp_err_t node_registry_update_last_seen(const char *node_id);
esp_err_t node_registry_update_telemetry(const char *node_id, void *telemetry);
esp_err_t node_registry_remove(const char *node_id);
node_info_t* node_registry_get(const char *node_id);
node_info_t* node_registry_get_all(int *count);
int node_registry_get_count(void);
void node_registry_check_timeouts(void);

#ifdef __cplusplus
}
#endif

#endif // NODE_REGISTRY_H

