#include "zone_config.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <ctype.h>
#include <string.h>

static const char *TAG = "zone_config";

static char s_mesh_id[ZONE_CONFIG_MAX_LEN] = ZONE_CONFIG_UNCONFIGURED;
static char s_root_id[ZONE_CONFIG_MAX_LEN] = ZONE_CONFIG_UNCONFIGURED;
static bool s_cache_initialized = false;

static esp_err_t zone_config_read_from_nvs(char *mesh_id,
                                           size_t mesh_id_len,
                                           char *root_id,
                                           size_t root_id_len) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ZONE_CONFIG_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS namespace '%s': %s",
                 ZONE_CONFIG_NAMESPACE, esp_err_to_name(err));
        return err;
    }

    if (mesh_id && mesh_id_len > 0) {
        size_t required = mesh_id_len;
        err = nvs_get_str(handle, ZONE_CONFIG_KEY_MESH_ID, mesh_id, &required);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            mesh_id[0] = '\0';
        } else if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error reading mesh_id: %s", esp_err_to_name(err));
            nvs_close(handle);
            return err;
        }
    }

    if (root_id && root_id_len > 0) {
        size_t required = root_id_len;
        err = nvs_get_str(handle, ZONE_CONFIG_KEY_ROOT_ID, root_id, &required);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            root_id[0] = '\0';
        } else if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error reading root_id: %s", esp_err_to_name(err));
            nvs_close(handle);
            return err;
        }
    }

    nvs_close(handle);
    return ESP_OK;
}

static void zone_config_cache_defaults(void) {
    strncpy(s_mesh_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_mesh_id));
    s_mesh_id[sizeof(s_mesh_id) - 1] = '\0';

    strncpy(s_root_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_root_id));
    s_root_id[sizeof(s_root_id) - 1] = '\0';
}

esp_err_t zone_config_init(void) {
    if (s_cache_initialized) {
        return ESP_OK;
    }

    zone_config_cache_defaults();

    char mesh_id[ZONE_CONFIG_MAX_LEN] = {0};
    char root_id[ZONE_CONFIG_MAX_LEN] = {0};
    esp_err_t err = zone_config_read_from_nvs(mesh_id, sizeof(mesh_id),
                                              root_id, sizeof(root_id));

    if (err == ESP_OK) {
        if (mesh_id[0] != '\0' && zone_config_validate(mesh_id)) {
            strncpy(s_mesh_id, mesh_id, sizeof(s_mesh_id) - 1);
            s_mesh_id[sizeof(s_mesh_id) - 1] = '\0';
        }
        if (root_id[0] != '\0' && zone_config_validate(root_id)) {
            strncpy(s_root_id, root_id, sizeof(s_root_id) - 1);
            s_root_id[sizeof(s_root_id) - 1] = '\0';
        }
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Unable to load zone config from NVS");
    }

    s_cache_initialized = true;
    return ESP_OK;
}

esp_err_t zone_config_load(char *mesh_id,
                           size_t mesh_id_len,
                           char *root_id,
                           size_t root_id_len) {
    zone_config_init();

    if (mesh_id && mesh_id_len > 0) {
        strncpy(mesh_id, s_mesh_id, mesh_id_len - 1);
        mesh_id[mesh_id_len - 1] = '\0';
    }

    if (root_id && root_id_len > 0) {
        strncpy(root_id, s_root_id, root_id_len - 1);
        root_id[root_id_len - 1] = '\0';
    }

    return (strcmp(s_mesh_id, ZONE_CONFIG_UNCONFIGURED) == 0)
               ? ESP_ERR_NVS_NOT_FOUND
               : ESP_OK;
}

esp_err_t zone_config_save(const char *mesh_id, const char *root_id) {
    if (!mesh_id || !root_id) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!zone_config_validate(mesh_id) || !zone_config_validate(root_id)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(ZONE_CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace '%s': %s",
                 ZONE_CONFIG_NAMESPACE, esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(handle, ZONE_CONFIG_KEY_MESH_ID, mesh_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write mesh_id: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    err = nvs_set_str(handle, ZONE_CONFIG_KEY_ROOT_ID, root_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write root_id: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit zone config: %s", esp_err_to_name(err));
        return err;
    }

    strncpy(s_mesh_id, mesh_id, sizeof(s_mesh_id) - 1);
    s_mesh_id[sizeof(s_mesh_id) - 1] = '\0';

    strncpy(s_root_id, root_id, sizeof(s_root_id) - 1);
    s_root_id[sizeof(s_root_id) - 1] = '\0';

    return ESP_OK;
}

bool zone_config_validate(const char *str) {
    if (!str) {
        return false;
    }

    size_t len = strlen(str);
    if (len < 3 || len > 31) {
        return false;
    }

    if (strcmp(str, ZONE_CONFIG_SETUP) == 0 ||
        strcmp(str, ZONE_CONFIG_UNCONFIGURED) == 0) {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        char c = str[i];
        if (!isalnum((unsigned char)c) && c != '_' && c != '-') {
            return false;
        }
    }

    return true;
}

const char *zone_config_get_mesh_id(void) {
    zone_config_init();
    return s_mesh_id;
}

const char *zone_config_get_root_id(void) {
    zone_config_init();
    return s_root_id;
}

