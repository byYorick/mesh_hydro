/**
 * @file root_config.c
 * @brief ⭐ ЗОНИРОВАНИЕ: Конфигурация Root Node (Zone Identification)
 */

#include "root_config.h"
#include "mesh_config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>
#include "esp_system.h"
#include "esp_timer.h"

static const char *TAG = "RootConfig";

#define NVS_NAMESPACE "root_config"
#define NVS_KEY_ROOT_ID "root_id"
#define NVS_KEY_MESH_ID "mesh_id"
#define NVS_KEY_MQTT_PREFIX "mqtt_prefix"
#define NVS_KEY_MQTT_HOST "mqtt_host"
#define NVS_KEY_MQTT_PORT "mqtt_port"
#define NVS_KEY_ZONE_NAME "zone_name"
#define NVS_KEY_ZONE_LOCATION "zone_loc"
#define NVS_KEY_ZONE_NUMBER "zone_num"
#define NVS_KEY_IS_CONFIGURED "configured"
#define NVS_KEY_CONFIGURED_AT "conf_at"
#define NVS_KEY_FORCE_SETUP "force_setup"
#define NVS_KEY_FORCE_PAIR "force_pair"

// Глобальная конфигурация (кэш)
static root_config_t g_root_config = {0};
static bool g_initialized = false;
static bool g_force_setup_requested = false;
static bool g_force_pairing_requested = false;

//================================================
// Приватные функции
//================================================

/**
 * @brief Заменяет недопустимые символы mesh_id для использования в MQTT топиках
 */
static void sanitize_mesh_id(const char *mesh_id, char *out, size_t out_len) {
    if (!mesh_id || mesh_id[0] == '\0') {
        strncpy(out, "zone", out_len - 1);
        out[out_len - 1] = '\0';
        return;
    }

    size_t i = 0;
    size_t j = 0;
    while (mesh_id[i] != '\0' && j < out_len - 1) {
        char c = mesh_id[i++];
        if (c == '/' || c == '+' || c == '#' || c == ' ') {
            out[j++] = '_';
        } else {
            out[j++] = c;
        }
    }
    out[j] = '\0';
}

/**
 * @brief Гарантирует наличие завершающего '/' в prefix
 */
static void ensure_trailing_slash(char *prefix, size_t buf_len) {
    size_t len = strlen(prefix);
    if (len == 0) {
        return;
    }
    if (prefix[len - 1] == '/') {
        return;
    }
    if (len + 1 < buf_len) {
        prefix[len] = '/';
        prefix[len + 1] = '\0';
    } else {
        prefix[len - 1] = '/';
        prefix[len] = '\0';
    }
}

/**
 * @brief Обновляет MQTT topic prefix в соответствии с mesh_network_id (если используется дефолт)
 */
static void update_topic_prefix_for_mesh(void) {
    char sanitized_mesh[64] = {0};
    sanitize_mesh_id(g_root_config.mesh_network_id, sanitized_mesh, sizeof(sanitized_mesh));

    bool should_update =
        g_root_config.mqtt_topic_prefix[0] == '\0' ||
        strncmp(g_root_config.mqtt_topic_prefix, "hydro/zone", 10) == 0;

    if (should_update) {
        int written = snprintf(g_root_config.mqtt_topic_prefix,
                               sizeof(g_root_config.mqtt_topic_prefix),
                               "hydro/%s/", sanitized_mesh);

        if (written < 0 || written >= (int)sizeof(g_root_config.mqtt_topic_prefix)) {
            strncpy(g_root_config.mqtt_topic_prefix,
                    "hydro/zone_default/",
                    sizeof(g_root_config.mqtt_topic_prefix) - 1);
            g_root_config.mqtt_topic_prefix[sizeof(g_root_config.mqtt_topic_prefix) - 1] = '\0';
        }
    } else {
        ensure_trailing_slash(g_root_config.mqtt_topic_prefix, sizeof(g_root_config.mqtt_topic_prefix));
    }
}

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

    // MQTT host/port
    len = sizeof(g_root_config.mqtt_host);
    esp_err_t host_err = nvs_get_str(nvs_handle, NVS_KEY_MQTT_HOST, g_root_config.mqtt_host, &len);
    if (host_err != ESP_OK || g_root_config.mqtt_host[0] == '\0') {
        strncpy(g_root_config.mqtt_host, MQTT_BROKER_HOST, sizeof(g_root_config.mqtt_host) - 1);
    }
    uint16_t port_tmp = MQTT_BROKER_PORT;
    esp_err_t port_err = nvs_get_u16(nvs_handle, NVS_KEY_MQTT_PORT, &port_tmp);
    if (port_err == ESP_OK) {
        g_root_config.mqtt_port = port_tmp;
    } else {
        g_root_config.mqtt_port = MQTT_BROKER_PORT;
    }

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

    // Force setup flag
    uint8_t force_setup = 0;
    nvs_get_u8(nvs_handle, NVS_KEY_FORCE_SETUP, &force_setup);
    g_force_setup_requested = (force_setup == 1);

    uint8_t force_pair = 0;
    nvs_get_u8(nvs_handle, NVS_KEY_FORCE_PAIR, &force_pair);
    g_force_pairing_requested = (force_pair == 1);

    nvs_close(nvs_handle);
    ensure_trailing_slash(g_root_config.mqtt_topic_prefix, sizeof(g_root_config.mqtt_topic_prefix));
    update_topic_prefix_for_mesh();
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
    ensure_trailing_slash(g_root_config.mqtt_topic_prefix, sizeof(g_root_config.mqtt_topic_prefix));
    #else
    update_topic_prefix_for_mesh();
    #endif

    strncpy(g_root_config.mqtt_host, MQTT_BROKER_HOST, sizeof(g_root_config.mqtt_host) - 1);
    g_root_config.mqtt_port = MQTT_BROKER_PORT;

    g_root_config.zone_number = 1;
    g_root_config.is_configured = false;
    g_force_setup_requested = false;
    g_force_pairing_requested = false;
    
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
    update_topic_prefix_for_mesh();

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

    ensure_trailing_slash(g_root_config.mqtt_topic_prefix, sizeof(g_root_config.mqtt_topic_prefix));
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
    ensure_trailing_slash(g_root_config.mqtt_topic_prefix, sizeof(g_root_config.mqtt_topic_prefix));

    // Сохранение в NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(nvs_handle, NVS_KEY_MQTT_PREFIX, g_root_config.mqtt_topic_prefix);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "⭐ MQTT Topic Prefix установлен: %s", prefix);
    return err;
}

esp_err_t root_config_get_mqtt_endpoint(char *host, size_t host_len, uint16_t *port) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (host && host_len > 0) {
        if (g_root_config.mqtt_host[0] == '\0') {
            strncpy(host, MQTT_BROKER_HOST, host_len - 1);
            host[host_len - 1] = '\0';
        } else {
            strncpy(host, g_root_config.mqtt_host, host_len - 1);
            host[host_len - 1] = '\0';
        }
    }
    if (port) {
        *port = g_root_config.mqtt_port ? g_root_config.mqtt_port : MQTT_BROKER_PORT;
    }
    return ESP_OK;
}

esp_err_t root_config_set_mqtt_endpoint(const char *host, uint16_t port) {
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (host == NULL || strlen(host) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(g_root_config.mqtt_host, host, sizeof(g_root_config.mqtt_host) - 1);
    g_root_config.mqtt_port = port == 0 ? MQTT_BROKER_PORT : port;

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(nvs_handle, NVS_KEY_MQTT_HOST, g_root_config.mqtt_host);
    if (err == ESP_OK) {
        err = nvs_set_u16(nvs_handle, NVS_KEY_MQTT_PORT, g_root_config.mqtt_port);
    }
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✅ MQTT endpoint обновлён: %s:%u", g_root_config.mqtt_host, g_root_config.mqtt_port);
    }
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
    update_topic_prefix_for_mesh();

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

    root_config_t tmp = {0};
    memcpy(&tmp, config, sizeof(root_config_t));
    ensure_trailing_slash(tmp.mqtt_topic_prefix, sizeof(tmp.mqtt_topic_prefix));

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    // Сохранение всех полей
    nvs_set_str(nvs_handle, NVS_KEY_ROOT_ID, tmp.root_node_id);
    nvs_set_str(nvs_handle, NVS_KEY_MESH_ID, tmp.mesh_network_id);
    nvs_set_str(nvs_handle, NVS_KEY_MQTT_PREFIX, tmp.mqtt_topic_prefix);
    nvs_set_str(nvs_handle, NVS_KEY_MQTT_HOST, tmp.mqtt_host);
    nvs_set_u16(nvs_handle, NVS_KEY_MQTT_PORT, tmp.mqtt_port);
    nvs_set_str(nvs_handle, NVS_KEY_ZONE_NAME, tmp.zone_name);
    nvs_set_str(nvs_handle, NVS_KEY_ZONE_LOCATION, tmp.zone_location);
    nvs_set_u8(nvs_handle, NVS_KEY_ZONE_NUMBER, tmp.zone_number);
    nvs_set_u8(nvs_handle, NVS_KEY_IS_CONFIGURED, tmp.is_configured ? 1 : 0);
    nvs_set_u64(nvs_handle, NVS_KEY_CONFIGURED_AT, tmp.configured_at);

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        // Обновление кэша
        memcpy(&g_root_config, &tmp, sizeof(root_config_t));
        update_topic_prefix_for_mesh();
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
    ESP_LOGI(TAG, "║ MQTT Broker:       %-25s ║", g_root_config.mqtt_host[0] ? g_root_config.mqtt_host : MQTT_BROKER_HOST);
    ESP_LOGI(TAG, "║ MQTT Port:         %-25d ║", g_root_config.mqtt_port ? g_root_config.mqtt_port : MQTT_BROKER_PORT);
    ESP_LOGI(TAG, "║ Zone Number:       %-25d ║", g_root_config.zone_number);
    ESP_LOGI(TAG, "║ Zone Name:         %-25s ║", g_root_config.zone_name[0] ? g_root_config.zone_name : "-");
    ESP_LOGI(TAG, "║ Zone Location:     %-25s ║", g_root_config.zone_location[0] ? g_root_config.zone_location : "-");
    ESP_LOGI(TAG, "║ Configured:        %-25s ║", g_root_config.is_configured ? "YES" : "NO");
    ESP_LOGI(TAG, "║ Setup Requested:   %-25s ║", g_force_setup_requested ? "YES" : "NO");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════════╝");
}

esp_err_t root_config_request_setup(void)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (g_force_setup_requested) {
        return ESP_OK;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_u8(nvs_handle, NVS_KEY_FORCE_SETUP, 1);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        g_force_setup_requested = true;
    }
    return err;
}

bool root_config_is_setup_requested(void)
{
    if (!g_initialized) {
        return false;
    }
    return g_force_setup_requested;
}

bool root_config_take_setup_request(void)
{
    if (!g_initialized) {
        return false;
    }
    if (!g_force_setup_requested) {
        return false;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        esp_err_t set_err = nvs_set_u8(nvs_handle, NVS_KEY_FORCE_SETUP, 0);
        if (set_err == ESP_OK) {
            set_err = nvs_commit(nvs_handle);
        }
        if (set_err != ESP_OK) {
            ESP_LOGW(TAG, "Не удалось сбросить флаг force_setup: %s", esp_err_to_name(set_err));
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGW(TAG, "Не удалось открыть NVS для сброса force_setup: %s", esp_err_to_name(err));
    }

    g_force_setup_requested = false;
    return true;
}

esp_err_t root_config_request_node_pairing(void)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (g_force_pairing_requested) {
        return ESP_OK;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_u8(nvs_handle, NVS_KEY_FORCE_PAIR, 1);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        g_force_pairing_requested = true;
    }
    return err;
}

bool root_config_take_node_pairing_request(void)
{
    if (!g_initialized) {
        return false;
    }
    if (!g_force_pairing_requested) {
        return false;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        esp_err_t set_err = nvs_set_u8(nvs_handle, NVS_KEY_FORCE_PAIR, 0);
        if (set_err == ESP_OK) {
            set_err = nvs_commit(nvs_handle);
        }
        if (set_err != ESP_OK) {
            ESP_LOGW(TAG, "Не удалось сбросить флаг force_pair: %s", esp_err_to_name(set_err));
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGW(TAG, "Не удалось открыть NVS для сброса force_pair: %s", esp_err_to_name(err));
    }

    g_force_pairing_requested = false;
    return true;
}

