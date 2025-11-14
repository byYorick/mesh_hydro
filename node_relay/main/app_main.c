/**
 * @file app_main.c
 * @brief NODE Relay - управление светом, вентиляцией и форточками
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
#include "nvs_flash.h"
#include <time.h>

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "mesh_config.h"
#include "zone_config.h"
#include "cJSON.h"

// Relay компоненты (актуаторы, логика)
#include "relay_manager.h"

static const char *TAG = "relay_node";

#define SETUP_MESH_PREFIX "ROOT_PAIR_"
#define SETUP_HEARTBEAT_INTERVAL_MS 10000
#define SETUP_DISCOVERY_RETRY_MS 5000
#define SETUP_DISCOVERY_INTERVAL_MS 3000

static relay_node_config_t s_node_config = {0};
static bool s_is_setup_mode = false;
static bool s_setup_active = false;
static SemaphoreHandle_t s_setup_config_sem = NULL;
static TaskHandle_t s_setup_heartbeat_task = NULL;
static char s_setup_pin[7] = {0};
static char s_setup_mesh_id[32] = {0};
static uint8_t s_setup_mesh_channel = 0;
static uint8_t s_setup_mesh_id_bytes[6] = {0};
static char s_setup_mesh_tag[13] = {0};
static char s_setup_root_pin[8] = {0};
static char s_mesh_network_id[ZONE_CONFIG_MAX_LEN] = {0};
static char s_root_node_id[ZONE_CONFIG_MAX_LEN] = {0};
static bool s_setup_config_received = false;

static void run_setup_mode(void);
static void run_normal_mode(void);
static esp_err_t scan_for_setup_mesh(char *mesh_id_out, size_t mesh_id_len, uint8_t *channel_out);
static esp_err_t send_setup_message(const char *type, const char *pin, const char *mesh_id);
static void setup_heartbeat_task(void *arg);
static esp_err_t handle_write_config_command(cJSON *params);
static void send_setup_config_confirmation(void);
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len);
static int hex_digit(char c);
static bool hex_to_bytes(const char *hex, size_t hex_len, uint8_t *out, size_t out_len);

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

static bool hex_to_bytes(const char *hex, size_t hex_len, uint8_t *out, size_t out_len)
{
    if (!hex || !out || hex_len != out_len * 2) {
        return false;
    }

    for (size_t i = 0; i < out_len; ++i) {
        int high = hex_digit(hex[2 * i]);
        int low = hex_digit(hex[2 * i + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        out[i] = (uint8_t)((high << 4) | low);
    }
    return true;
}

void app_main(void)
{
    ESP_ERROR_CHECK(node_config_init());

    bool is_configured = node_config_is_configured();
    if (!is_configured) {
        s_is_setup_mode = true;
        run_setup_mode();
        return;
    }

    s_setup_config_received = true;
    s_is_setup_mode = false;
    run_normal_mode();
}

static void run_normal_mode(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== NODE Relay Starting ===");
    ESP_LOGI(TAG, "========================================");

    memset(&s_node_config, 0, sizeof(s_node_config));
    if (node_config_load(&s_node_config, sizeof(s_node_config), "relay_ns") != ESP_OK) {
        ESP_LOGW(TAG, "Config not found, using defaults");
        node_config_reset_to_default(&s_node_config, "relay");
        strcpy(s_node_config.base.node_id, "relay_001");
        strcpy(s_node_config.base.zone, "Zone 1");
        node_config_save(&s_node_config, sizeof(s_node_config), "relay_ns");
    }

    ESP_LOGI(TAG, "Node ID: %s", s_node_config.base.node_id);

    if (zone_config_init() != ESP_OK) {
        ESP_LOGE(TAG, "zone_config_init failed");
        strncpy(s_mesh_network_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_mesh_network_id) - 1);
        strncpy(s_root_node_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_root_node_id) - 1);
    } else if (zone_config_load(s_mesh_network_id, sizeof(s_mesh_network_id),
                                s_root_node_id, sizeof(s_root_node_id)) != ESP_OK) {
        ESP_LOGW(TAG, "Zone config missing, using UNCONFIGURED defaults");
        strncpy(s_mesh_network_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_mesh_network_id) - 1);
        strncpy(s_root_node_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_root_node_id) - 1);
    }
    s_mesh_network_id[sizeof(s_mesh_network_id) - 1] = '\0';
    s_root_node_id[sizeof(s_root_node_id) - 1] = '\0';

    ESP_LOGI(TAG, "Zone context: mesh_id=%s, root_id=%s",
             s_mesh_network_id, s_root_node_id);

    const char *mesh_ssid = NULL;
    if (zone_config_validate(s_mesh_network_id) && s_mesh_network_id[0] != '\0') {
        mesh_ssid = s_mesh_network_id;
    } else {
        if (node_config_get_mesh_network_id(s_mesh_network_id) != ESP_OK || s_mesh_network_id[0] == '\0') {
            strncpy(s_mesh_network_id, MESH_NETWORK_ID, sizeof(s_mesh_network_id) - 1);
            s_mesh_network_id[sizeof(s_mesh_network_id) - 1] = '\0';
        }
        mesh_ssid = s_mesh_network_id;
    }

    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = 6,
        .router_ssid = NULL,
        .router_password = NULL,
        .router_bssid = NULL,
    };
    mesh_manager_string_to_mesh_id(mesh_ssid, mesh_config.mesh_id);
    mesh_config.mesh_id_str = mesh_ssid;

    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());

    ESP_LOGI(TAG, "Mesh started (%s)", mesh_config.mesh_id_str ? mesh_config.mesh_id_str : "(null)");

    ESP_ERROR_CHECK(relay_manager_init(&s_node_config));
    ESP_ERROR_CHECK(relay_manager_start());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000));
        ESP_LOGI(TAG, "Status: Mesh=%s", mesh_manager_is_connected() ? "ONLINE" : "OFFLINE");
    }
}

static void run_setup_mode(void)
{
    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "=== NODE Relay SETUP MODE ===");
    ESP_LOGW(TAG, "========================================");

    memset(&s_node_config, 0, sizeof(s_node_config));
    memset(s_setup_pin, 0, sizeof(s_setup_pin));
    node_config_generate_setup_pin(s_setup_pin, sizeof(s_setup_pin));
    strncpy(s_mesh_network_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_mesh_network_id) - 1);
    strncpy(s_root_node_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_root_node_id) - 1);
    s_setup_config_received = false;

    ESP_LOGI(TAG, "Setup PIN: %s", s_setup_pin);

    esp_err_t scan_err;
    uint32_t attempt = 0;
    do {
        attempt++;
        scan_err = scan_for_setup_mesh(s_setup_mesh_id, sizeof(s_setup_mesh_id), &s_setup_mesh_channel);
        if (scan_err != ESP_OK) {
            ESP_LOGW(TAG, "[%u] Setup mesh not found, retrying in %d ms", attempt, SETUP_DISCOVERY_RETRY_MS);
            vTaskDelay(pdMS_TO_TICKS(SETUP_DISCOVERY_RETRY_MS));
        }
    } while (scan_err != ESP_OK);

    ESP_LOGI(TAG, "Found mesh: %s (channel=%u)", s_setup_mesh_id, s_setup_mesh_channel);

    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = s_setup_mesh_channel,
        .max_connection = 6,
        .router_ssid = NULL,
        .router_password = NULL,
        .router_bssid = NULL,
    };
    memcpy(mesh_config.mesh_id, s_setup_mesh_id_bytes, sizeof(mesh_config.mesh_id));
    mesh_config.mesh_id_str = s_setup_mesh_id;

    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());

    const int max_wait_ms = 20000;
    int waited_ms = 0;
    while (!mesh_manager_is_connected() && waited_ms < max_wait_ms) {
        vTaskDelay(pdMS_TO_TICKS(500));
        waited_ms += 500;
    }

    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh not connected, discovery may fail but continuing");
    }

    if (send_setup_message("discovery", s_setup_pin, s_setup_mesh_id) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send discovery message");
    }

    s_setup_active = true;
    if (xTaskCreate(setup_heartbeat_task, "setup_heartbeat", 4096, NULL, 5, &s_setup_heartbeat_task) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create heartbeat task");
        s_setup_heartbeat_task = NULL;
    }

    s_setup_config_sem = xSemaphoreCreateBinary();
    if (!s_setup_config_sem) {
        ESP_LOGE(TAG, "Failed to create setup semaphore");
    }

    ESP_LOGI(TAG, "Waiting for write_config...");
    if (s_setup_config_sem) {
        xSemaphoreTake(s_setup_config_sem, portMAX_DELAY);
        vSemaphoreDelete(s_setup_config_sem);
        s_setup_config_sem = NULL;
    }

    s_setup_active = false;
    if (s_setup_heartbeat_task) {
        while (eTaskGetState(s_setup_heartbeat_task) != eDeleted) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        s_setup_heartbeat_task = NULL;
    }

    ESP_LOGI(TAG, "Configuration received. Restarting...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    mesh_manager_stop();
    esp_restart();
}

static esp_err_t scan_for_setup_mesh(char *mesh_id_out, size_t mesh_id_len, uint8_t *channel_out)
{
    if (mesh_id_out == NULL || mesh_id_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    mesh_id_out[0] = '\0';
    if (channel_out) {
        *channel_out = 0;
    }

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (!sta_netif) {
        return ESP_FAIL;
    }

    err = esp_wifi_init(&wifi_cfg);
    if (err != ESP_OK) {
        esp_netif_destroy(sta_netif);
        return err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        esp_wifi_deinit();
        esp_netif_destroy(sta_netif);
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        esp_wifi_deinit();
        esp_netif_destroy(sta_netif);
        return err;
    }

    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 300,
            },
        },
    };

    err = esp_wifi_scan_start(&scan_cfg, true);
    if (err != ESP_OK) {
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(sta_netif);
        return err;
    }

    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(sta_netif);
        return err;
    }
    if (ap_count == 0) {
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(sta_netif);
        return ESP_ERR_NOT_FOUND;
    }

    wifi_ap_record_t *ap_records = calloc(ap_count, sizeof(wifi_ap_record_t));
    if (!ap_records) {
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(sta_netif);
        return ESP_ERR_NO_MEM;
    }

    err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    if (err != ESP_OK) {
        free(ap_records);
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(sta_netif);
        return err;
    }

    int best_rssi = -127;
    bool found = false;
    uint8_t best_channel = 0;
    char best_ssid[33] = {0};
    char best_tag[13] = {0};
    char best_pin[8] = {0};
    uint8_t best_id[6] = {0};
    size_t prefix_len = strlen(SETUP_MESH_PREFIX);

    for (uint16_t i = 0; i < ap_count; ++i) {
        const char *ssid = (const char *)ap_records[i].ssid;
        if (ssid[0] == '\0') {
            continue;
        }
        if (strncmp(ssid, SETUP_MESH_PREFIX, prefix_len) != 0) {
            continue;
        }

        const char *payload = ssid + prefix_len;
        const char *sep = strchr(payload, '_');
        if (!sep) {
            continue;
        }
        size_t tag_len = (size_t)(sep - payload);
        if (tag_len != 12) {
            continue;
        }

        char tag_buf[13] = {0};
        memcpy(tag_buf, payload, tag_len);
        const char *pin_part = sep + 1;
        if (pin_part[0] == '\0') {
            continue;
        }

        uint8_t candidate_id[6] = {0};
        if (!hex_to_bytes(tag_buf, tag_len, candidate_id, sizeof(candidate_id))) {
            continue;
        }

        if (!found || ap_records[i].rssi > best_rssi) {
            best_rssi = ap_records[i].rssi;
            best_channel = ap_records[i].primary;
            strncpy(best_ssid, ssid, sizeof(best_ssid) - 1);
            strncpy(best_tag, tag_buf, sizeof(best_tag) - 1);
            strncpy(best_pin, pin_part, sizeof(best_pin) - 1);
            memcpy(best_id, candidate_id, sizeof(best_id));
            found = true;
        }
    }

    free(ap_records);
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_destroy(sta_netif);

    if (!found) {
        return ESP_ERR_NOT_FOUND;
    }

    strncpy(mesh_id_out, best_ssid, mesh_id_len - 1);
    mesh_id_out[mesh_id_len - 1] = '\0';
    strncpy(s_setup_mesh_tag, best_tag, sizeof(s_setup_mesh_tag) - 1);
    strncpy(s_setup_root_pin, best_pin, sizeof(s_setup_root_pin) - 1);
    memcpy(s_setup_mesh_id_bytes, best_id, sizeof(s_setup_mesh_id_bytes));
    if (channel_out) {
        *channel_out = best_channel;
    }

    ESP_LOGI(TAG, "Best setup mesh: %s (RSSI=%d, channel=%u)", mesh_id_out, best_rssi, best_channel);
    return ESP_OK;
}

static esp_err_t send_setup_message(const char *type, const char *pin, const char *mesh_id)
{
    if (type == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }

    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    cJSON_AddStringToObject(root, "type", type);
    cJSON_AddStringToObject(root, "node_type", "relay");
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    if (pin && pin[0] != '\0') {
        cJSON_AddStringToObject(root, "pin", pin);
    }
    if (mesh_id && mesh_id[0] != '\0') {
        cJSON_AddStringToObject(root, "temp_mesh_id", mesh_id);
    }
    if (s_setup_mesh_tag[0] != '\0') {
        cJSON_AddStringToObject(root, "pairing_mesh_tag", s_setup_mesh_tag);
    }

    const char *mesh_field = zone_config_validate(s_mesh_network_id) && s_mesh_network_id[0] != '\0'
                                 ? s_mesh_network_id
                                 : ZONE_CONFIG_UNCONFIGURED;
    const char *root_field = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_field);
    cJSON_AddStringToObject(root, "root_node_id", root_field);

    cJSON_AddStringToObject(root, "firmware_version", esp_app_get_description()->version);

    char *payload = cJSON_PrintUnformatted(root);
    if (!payload) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = mesh_manager_send_to_root((const uint8_t *)payload, strlen(payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send %s: %s", type, esp_err_to_name(err));
    }

    free(payload);
    cJSON_Delete(root);
    return err;
}

static void setup_heartbeat_task(void *arg)
{
    while (s_setup_active) {
        TickType_t delay_ticks = s_setup_config_received
                                     ? pdMS_TO_TICKS(SETUP_HEARTBEAT_INTERVAL_MS)
                                     : pdMS_TO_TICKS(SETUP_DISCOVERY_INTERVAL_MS);
        vTaskDelay(delay_ticks);
        if (!s_setup_active) {
            break;
        }
        const char *msg_type = s_setup_config_received ? "heartbeat" : "discovery";
        if (send_setup_message(msg_type, s_setup_pin, s_setup_mesh_id) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send %s", msg_type);
        }
    }

    s_setup_heartbeat_task = NULL;
    vTaskDelete(NULL);
}

static esp_err_t handle_write_config_command(cJSON *params)
{
    if (!params) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *pin_item = cJSON_GetObjectItem(params, "pin");
    if (!cJSON_IsString(pin_item) || strncmp(pin_item->valuestring, s_setup_pin, sizeof(s_setup_pin) - 1) != 0) {
        ESP_LOGE(TAG, "PIN mismatch");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *node_id_item = cJSON_GetObjectItem(params, "node_id");
    if (!cJSON_IsString(node_id_item)) {
        ESP_LOGE(TAG, "Missing node_id");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *mesh_id_item = cJSON_GetObjectItem(params, "mesh_network_id");
    if (!mesh_id_item) {
        mesh_id_item = cJSON_GetObjectItem(params, "mesh_id");
    }
    if (!cJSON_IsString(mesh_id_item)) {
        ESP_LOGE(TAG, "Missing mesh_network_id");
        return ESP_ERR_INVALID_ARG;
    }

    const char *mesh_id = mesh_id_item->valuestring;
    cJSON *root_node_id_item = cJSON_GetObjectItem(params, "root_node_id");
    const char *root_id = (cJSON_IsString(root_node_id_item) && root_node_id_item->valuestring[0] != '\0')
                              ? root_node_id_item->valuestring
                              : node_id_item->valuestring;

    if (!zone_config_validate(mesh_id) || !zone_config_validate(root_id)) {
        ESP_LOGE(TAG, "Invalid mesh_id (%s) or root_id (%s)", mesh_id, root_id);
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_node_config, 0, sizeof(s_node_config));
    strncpy(s_node_config.base.node_id, node_id_item->valuestring, sizeof(s_node_config.base.node_id) - 1);
    s_node_config.base.node_id[sizeof(s_node_config.base.node_id) - 1] = '\0';
    strncpy(s_setup_mesh_id, mesh_id, sizeof(s_setup_mesh_id) - 1);
    s_setup_mesh_id[sizeof(s_setup_mesh_id) - 1] = '\0';

    strncpy(s_node_config.base.root_node_id, root_id, sizeof(s_node_config.base.root_node_id) - 1);
    s_node_config.base.root_node_id[sizeof(s_node_config.base.root_node_id) - 1] = '\0';
    strncpy(s_root_node_id, root_id, sizeof(s_root_node_id) - 1);
    s_root_node_id[sizeof(s_root_node_id) - 1] = '\0';
    esp_err_t zone_err = zone_config_save(mesh_id, root_id);
    if (zone_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save zone config: %s", esp_err_to_name(zone_err));
        return zone_err;
    }
    node_config_set_root_node_id(root_id);

    cJSON *zone_item = cJSON_GetObjectItem(params, "zone");
    if (cJSON_IsString(zone_item)) {
        strncpy(s_node_config.base.zone, zone_item->valuestring, sizeof(s_node_config.base.zone) - 1);
        s_node_config.base.zone[sizeof(s_node_config.base.zone) - 1] = '\0';
    }

    s_node_config.base.config_valid = true;
    s_node_config.base.config_version++;
    s_node_config.base.last_updated = (uint64_t)time(NULL);

    strncpy(s_mesh_network_id, mesh_id, sizeof(s_mesh_network_id) - 1);
    s_mesh_network_id[sizeof(s_mesh_network_id) - 1] = '\0';

    esp_err_t err = node_config_set_mesh_network_id(mesh_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store mesh_id: %s", esp_err_to_name(err));
        return err;
    }

    cJSON *config_obj = cJSON_GetObjectItem(params, "config");
    if (cJSON_IsObject(config_obj)) {
        if (node_config_update_from_json(&s_node_config, config_obj, "relay") != ESP_OK) {
            ESP_LOGW(TAG, "Unable to update relay-specific config");
        }
    }

    err = node_config_save(&s_node_config, sizeof(s_node_config), "relay_ns");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save relay config: %s", esp_err_to_name(err));
        return err;
    }

    err = node_config_mark_configured(true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mark node configured: %s", esp_err_to_name(err));
        return err;
    }

    s_is_setup_mode = false;
    send_setup_config_confirmation();
    return ESP_OK;
}

static void send_setup_config_confirmation(void)
{
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh not connected, skipping config_confirmation");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return;
    }

    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    cJSON_AddStringToObject(root, "type", "config_confirmation");
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    cJSON_AddStringToObject(root, "node_id", s_node_config.base.node_id);
    const char *mesh_field = zone_config_validate(s_mesh_network_id) ? s_mesh_network_id : s_setup_mesh_id;
    cJSON_AddStringToObject(root, "mesh_id", mesh_field);
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_field);
    cJSON_AddStringToObject(root, "status", "success");
    if (s_setup_pin[0] != '\0') {
        cJSON_AddStringToObject(root, "pin", s_setup_pin);
    }
    const char *root_field = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "root_node_id", root_field);
    if (s_node_config.base.zone[0] != '\0') {
        cJSON_AddStringToObject(root, "zone", s_node_config.base.zone);
    }

    char *payload = cJSON_PrintUnformatted(root);
    if (!payload) {
        cJSON_Delete(root);
        return;
    }

    esp_err_t err = mesh_manager_send_to_root((const uint8_t *)payload, strlen(payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send config_confirmation: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Config confirmation sent");
    }

    free(payload);
    cJSON_Delete(root);
}

static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len)
{
    char *data_copy = malloc(len + 1);
    if (!data_copy) {
        ESP_LOGE(TAG, "Failed to allocate buffer for mesh data");
        return;
    }
    memcpy(data_copy, data, len);
    data_copy[len] = '\0';

    mesh_message_t msg;
    if (!mesh_protocol_parse(data_copy, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        free(data_copy);
        return;
    }

    bool is_write_config = false;
    cJSON *cmd = NULL;
    cJSON *params = NULL;
    if (msg.type == MESH_MSG_COMMAND && msg.data) {
        cmd = cJSON_GetObjectItem(msg.data, "command");
        params = cJSON_GetObjectItem(msg.data, "params");
        if (cmd && cJSON_IsString(cmd) && strcmp(cmd->valuestring, "write_config") == 0) {
            is_write_config = true;
        }
    }

    if (!is_write_config && strcmp(msg.node_id, s_node_config.base.node_id) != 0) {
        mesh_protocol_free_message(&msg);
        free(data_copy);
        return;
    }

    switch (msg.type) {
        case MESH_MSG_COMMAND:
            if (is_write_config) {
                ESP_LOGI(TAG, "Received write_config command");
                if (!params) {
                    params = msg.data;
                }
                if (handle_write_config_command(params) == ESP_OK) {
                    if (s_setup_config_sem) {
                        xSemaphoreGive(s_setup_config_sem);
                    }
                }
                mesh_protocol_free_message(&msg);
                free(data_copy);
                return;
            }

            if (cmd && cJSON_IsString(cmd)) {
                relay_manager_handle_command(cmd->valuestring, params ? params : msg.data);
            }
            break;

        case MESH_MSG_CONFIG:
            relay_manager_handle_config_update(msg.data);
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }

    mesh_protocol_free_message(&msg);
    free(data_copy);
}

