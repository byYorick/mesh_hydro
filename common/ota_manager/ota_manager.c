/**
 * @file ota_manager.c
 * @brief Реализация OTA менеджера
 */

#include "ota_manager.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"

static const char *TAG = "ota_manager";

static ota_progress_cb_t s_progress_cb = NULL;
static bool s_is_updating = false;

esp_err_t ota_manager_init(void) {
    ESP_LOGI(TAG, "OTA manager initialized");
    return ESP_OK;
}

esp_err_t ota_manager_start_update(const char *url) {
    ESP_LOGI(TAG, "Starting OTA update from URL: %s", url);
    s_is_updating = true;
    
    // TODO: Реализовать HTTP(S) OTA
    
    s_is_updating = false;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ota_manager_start_update_from_mesh(const uint8_t *firmware_data, size_t firmware_size, const char *md5_hash) {
    ESP_LOGI(TAG, "Starting OTA update from mesh (%d bytes)", firmware_size);
    s_is_updating = true;
    
    // TODO: Реализовать OTA через mesh
    
    s_is_updating = false;
    return ESP_ERR_NOT_SUPPORTED;
}

void ota_manager_register_progress_cb(ota_progress_cb_t cb) {
    s_progress_cb = cb;
}

bool ota_manager_is_updating(void) {
    return s_is_updating;
}

