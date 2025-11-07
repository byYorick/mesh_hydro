/**
 * @file root_config.c
 * @brief ⭐ ЗОНИРОВАНИЕ: Конфигурация Root Node (Zone Identification)
 */

#include "root_config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "RootConfig";

#define NVS_NAMESPACE "root_config"
#define NVS_KEY_ROOT_ID "root_id"
#define NVS_KEY_MESH_ID "mesh_id"
#define NVS_KEY_MQTT_PREFIX "mqtt_prefix"
#define NVS_KEY_ZONE_NAME "zone_name"
#define NVS_KEY_ZONE_LOCATION "zone_loc"
#define NVS_KEY_ZONE_NUMBER "zone_num"
#define NVS_KEY_IS_CONFIGURED "configured"
#define NVS_KEY_CONFIGURED_AT "conf_at"

// Глобальная конфигурация (кэш)
static root_config_t g_root_config = {0};
static bool g_initialized = false;

//================================================
// Приватные функции
//================================================

/**
 * @brief Загрузка конфигурации из NVS
 */
static esp_err_t load_config_from_nvs(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Не удалось открыть NVS: %s", esp_err_to_name(err));
        return err;
    }

    size_t len;

    // Root Node ID
    len = sizeof(g_root_config.root_node_id);
    nvs_get_str(nvs_handle, NVS_KEY_ROOT_ID, g_root_config.root_node_id, &len);

    // Mesh Network ID
    len = sizeof(g_root_config.mesh_network_id);
    nvs_get_str(nvs_handle, NVS_KEY_MESH_ID, g_root_config.mesh_network_id, &len);

    // MQTT Topic Prefix
    len = sizeof(g_root_config.mqtt_topic_prefix);
    nvs_get_str(nvs_handle, NVS_KEY_MQTT_PREFIX, g_root_config.mqtt_topic_prefix, &len);

    // Zone Name
    len = sizeof(g_root_config.zone_name);
    nvs_get_str(nvs_handle, NVS_KEY_ZONE_NAME, g_root_config.zone_name, &len);

    // Zone Location
    len = sizeof(g_root_config.zone_location);
    nvs_get_str(nvs_handle, NVS_KEY_ZONE_LOCATION, g_root_config.zone_location, &len);

    // Zone Number
    nvs_get_u8(nvs_handle, NVS_KEY_ZONE_NUMBER, &g_root_config.zone_number);

    // Is Configured
    uint8_t configured = 0;
    nvs_get_u8(nvs_handle, NVS_KEY_IS_CONFIGURED, &configured);
    g_root_config.is_configured = (configured == 1);

    // Configured At
    nvs_get_u64(nvs_handle, NVS_KEY_CONFIGURED_AT, &g_root_config.configured_at);

    nvs_close(nvs_handle);
    return ESP_OK;
}

/**
 * @brief Инициализация дефолтной конфигурации
 */
static void init_default_config(void) {
    memset(&g_root_config, 0, sizeof(g_root_config));
    
    // Дефолтные значения из mesh_config.h
    #ifdef ROOT_NODE_ID
    strncpy(g_root_config.root_node_id, ROOT_NODE_ID, sizeof(g_root_config.root_node_id) - 1);
    #else
    strncpy(g_root_config.root_node_id, "root_001", sizeof(g_root_config.root_node_id) - 1);
    #endif

    #ifdef MESH_NETWORK_ID
    strncpy(g_root_config.mesh_network_id, MESH_NETWORK_ID, sizeof(g_root_config.mesh_network_id) - 1);
    #else
    strncpy(g_root_config.mesh_network_id, "HYDRO1_ZONE1", sizeof(g_root_config.mesh_network_id) - 1);
    #endif

    #ifdef MQTT_TOPIC_PREFIX
    strncpy(g_root_config.mqtt_topic_prefix, MQTT_TOPIC_PREFIX, sizeof(g_root_config.mqtt_topic_prefix) - 1);
    #else
    strncpy(g_root_config.mqtt_topic_prefix, "hydro/zone1/", sizeof(g_root_config.mqtt_topic_prefix) - 1);
    #endif

    g_root_config.zone_number = 1;
    g_root_config.is_configured = false;
    
    ESP_LOGI(TAG, "⭐ Инициализирована дефолтная конфигурация зоны");
}

//================================================
// Публичные функции
//================================================

esp_err_t root_config_init(void) {
    if (g_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "⭐ Инициализация Root Config (Зонирование)...");

    // Инициализация NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Очистка NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Попытка загрузить конфигурацию из NVS
    err = load_config_from_nvs();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Конфигурация не найдена в NVS, используется дефолтная");
        init_default_config();
    } else {
        ESP_LOGI(TAG, "✅ Конфигурация загружена из NVS");
    }

    g_initialized = true;
    root_config_print();

    return ESP_OK;
}

esp_err_t root_config_get_root_node_id(char *root_id) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (root_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(root_id, g_root_config.root_node_id, 32);
    return ESP_OK;
}

esp_err_t root_config_set_root_node_id(const char *root_id) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (root_id == NULL || strlen(root_id) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(g_root_config.root_node_id, root_id, sizeof(g_root_config.root_node_id) - 1);

    // Сохранение в NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(nvs_handle, NVS_KEY_ROOT_ID, root_id);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "⭐ Root Node ID установлен: %s", root_id);
    return err;
}

esp_err_t root_config_get_mesh_network_id(char *mesh_id) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (mesh_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(mesh_id, g_root_config.mesh_network_id, 32);
    return ESP_OK;
}

esp_err_t root_config_set_mesh_network_id(const char *mesh_id) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (mesh_id == NULL || strlen(mesh_id) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(g_root_config.mesh_network_id, mesh_id, sizeof(g_root_config.mesh_network_id) - 1);

    // Сохранение в NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(nvs_handle, NVS_KEY_MESH_ID, mesh_id);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "⭐ Mesh Network ID установлен: %s", mesh_id);
    return err;
}

esp_err_t root_config_get_mqtt_topic_prefix(char *prefix) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (prefix == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(prefix, g_root_config.mqtt_topic_prefix, 64);
    return ESP_OK;
}

esp_err_t root_config_set_mqtt_topic_prefix(const char *prefix) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (prefix == NULL || strlen(prefix) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(g_root_config.mqtt_topic_prefix, prefix, sizeof(g_root_config.mqtt_topic_prefix) - 1);

    // Сохранение в NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(nvs_handle, NVS_KEY_MQTT_PREFIX, prefix);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "⭐ MQTT Topic Prefix установлен: %s", prefix);
    return err;
}

uint8_t root_config_get_zone_number(void) {
    if (!g_initialized) {
        return 0;
    }
    return g_root_config.zone_number;
}

esp_err_t root_config_set_zone_number(uint8_t zone_number) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (zone_number == 0 || zone_number > 99) {
        return ESP_ERR_INVALID_ARG;
    }

    g_root_config.zone_number = zone_number;

    // Сохранение в NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_u8(nvs_handle, NVS_KEY_ZONE_NUMBER, zone_number);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "⭐ Номер зоны установлен: %d", zone_number);
    return err;
}

bool root_config_is_zone_configured(void) {
    if (!g_initialized) {
        return false;
    }
    return g_root_config.is_configured;
}

esp_err_t root_config_configure_zone(uint8_t zone_number, const char *zone_name, const char *zone_location) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (zone_number == 0 || zone_number > 99) {
        return ESP_ERR_INVALID_ARG;
    }

    // Установка номера зоны
    g_root_config.zone_number = zone_number;

    // Генерация root_node_id, mesh_network_id, mqtt_topic_prefix
    snprintf(g_root_config.root_node_id, sizeof(g_root_config.root_node_id), "root_%03d", zone_number);
    snprintf(g_root_config.mesh_network_id, sizeof(g_root_config.mesh_network_id), "HYDRO1_ZONE%d", zone_number);
    snprintf(g_root_config.mqtt_topic_prefix, sizeof(g_root_config.mqtt_topic_prefix), "hydro/zone%d/", zone_number);

    // Опциональные поля
    if (zone_name != NULL) {
        strncpy(g_root_config.zone_name, zone_name, sizeof(g_root_config.zone_name) - 1);
    }
    if (zone_location != NULL) {
        strncpy(g_root_config.zone_location, zone_location, sizeof(g_root_config.zone_location) - 1);
    }

    g_root_config.is_configured = true;
    g_root_config.configured_at = (uint64_t)esp_timer_get_time() / 1000000; // seconds

    // Сохранение в NVS
    return root_config_save(&g_root_config);
}

esp_err_t root_config_auto_configure(uint8_t zone_number) {
    return root_config_configure_zone(zone_number, NULL, NULL);
}

esp_err_t root_config_get(root_config_t *config) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(config, &g_root_config, sizeof(root_config_t));
    return ESP_OK;
}

esp_err_t root_config_save(const root_config_t *config) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    // Сохранение всех полей
    nvs_set_str(nvs_handle, NVS_KEY_ROOT_ID, config->root_node_id);
    nvs_set_str(nvs_handle, NVS_KEY_MESH_ID, config->mesh_network_id);
    nvs_set_str(nvs_handle, NVS_KEY_MQTT_PREFIX, config->mqtt_topic_prefix);
    nvs_set_str(nvs_handle, NVS_KEY_ZONE_NAME, config->zone_name);
    nvs_set_str(nvs_handle, NVS_KEY_ZONE_LOCATION, config->zone_location);
    nvs_set_u8(nvs_handle, NVS_KEY_ZONE_NUMBER, config->zone_number);
    nvs_set_u8(nvs_handle, NVS_KEY_IS_CONFIGURED, config->is_configured ? 1 : 0);
    nvs_set_u64(nvs_handle, NVS_KEY_CONFIGURED_AT, config->configured_at);

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        // Обновление кэша
        memcpy(&g_root_config, config, sizeof(root_config_t));
        ESP_LOGI(TAG, "✅ Конфигурация зоны сохранена в NVS");
    }

    return err;
}

esp_err_t root_config_reset(void) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGW(TAG, "⚠️ Сброс конфигурации зоны...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_erase_all(nvs_handle);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        init_default_config();
        ESP_LOGI(TAG, "✅ Конфигурация сброшена к дефолтной");
    }

    return err;
}

void root_config_print(void) {
    if (!g_initialized) {
        ESP_LOGW(TAG, "Root Config не инициализирован");
        return;
    }

    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║       ⭐ КОНФИГУРАЦИЯ ЗОНЫ (ROOT NODE) ⭐      ║");
    ESP_LOGI(TAG, "╠═══════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║ Root Node ID:      %-25s ║", g_root_config.root_node_id);
    ESP_LOGI(TAG, "║ Mesh Network ID:   %-25s ║", g_root_config.mesh_network_id);
    ESP_LOGI(TAG, "║ MQTT Topic Prefix: %-25s ║", g_root_config.mqtt_topic_prefix);
    ESP_LOGI(TAG, "║ Zone Number:       %-25d ║", g_root_config.zone_number);
    ESP_LOGI(TAG, "║ Zone Name:         %-25s ║", g_root_config.zone_name[0] ? g_root_config.zone_name : "-");
    ESP_LOGI(TAG, "║ Zone Location:     %-25s ║", g_root_config.zone_location[0] ? g_root_config.zone_location : "-");
    ESP_LOGI(TAG, "║ Configured:        %-25s ║", g_root_config.is_configured ? "YES" : "NO");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════╝");
}

