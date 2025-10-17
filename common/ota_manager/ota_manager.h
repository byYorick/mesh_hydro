/**
 * @file ota_manager.h
 * @brief OTA обновления для NODE узлов
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ota_progress_cb_t)(int progress_percent);

esp_err_t ota_manager_init(void);
esp_err_t ota_manager_start_update(const char *url);
esp_err_t ota_manager_start_update_from_mesh(const uint8_t *firmware_data, size_t firmware_size, const char *md5_hash);
void ota_manager_register_progress_cb(ota_progress_cb_t cb);
bool ota_manager_is_updating(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_MANAGER_H

