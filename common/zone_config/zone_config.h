#ifndef ZONE_CONFIG_H
#define ZONE_CONFIG_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZONE_CONFIG_NAMESPACE   "zone_cfg"
#define ZONE_CONFIG_KEY_MESH_ID "mesh_id"
#define ZONE_CONFIG_KEY_ROOT_ID "root_id"

#define ZONE_CONFIG_MAX_LEN       32
#define ZONE_CONFIG_UNCONFIGURED  "UNCONFIGURED"
#define ZONE_CONFIG_SETUP         "setup"

esp_err_t zone_config_init(void);

esp_err_t zone_config_load(char *mesh_id,
                           size_t mesh_id_len,
                           char *root_id,
                           size_t root_id_len);

esp_err_t zone_config_save(const char *mesh_id, const char *root_id);

bool zone_config_validate(const char *str);

const char *zone_config_get_mesh_id(void);

const char *zone_config_get_root_id(void);

#ifdef __cplusplus
}
#endif

#endif /* ZONE_CONFIG_H */

