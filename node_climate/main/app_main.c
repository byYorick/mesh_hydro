/**
 * @file app_main.c
 * @brief NODE Climate - датчики климата
 * 
 * ESP32 - Temperature, Humidity, CO2, Lux sensors
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/i2c.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
#include <time.h>

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "mesh_config.h"  // Централизованная конфигурация
#include "zone_config.h"

// NODE Climate компоненты
#include "climate_controller.h"
#include "sht3x_driver.h"
#include "ccs811_driver.h"
#include "lux_sensor.h"

static const char *TAG = "CLIMATE";

// Глобальная конфигурация
static climate_node_config_t g_config;
static char s_mesh_network_id[ZONE_CONFIG_MAX_LEN] = {0};
static char s_root_node_id[ZONE_CONFIG_MAX_LEN] = {0};

#define SETUP_MESH_PREFIX "HYDRO_SETUP_"
#define SETUP_HEARTBEAT_INTERVAL_MS 10000
#define SETUP_DISCOVERY_RETRY_MS 5000

static bool s_is_setup_mode = false;
static bool s_setup_active = false;
static SemaphoreHandle_t s_setup_config_sem = NULL;
static TaskHandle_t s_setup_heartbeat_task = NULL;
static char s_setup_pin[7] = {0};
static char s_setup_mesh_id[32] = {0};
static uint8_t s_setup_mesh_channel = 0;

// Forward declarations
static void run_setup_mode(void);
static void run_normal_mode(void);
static esp_err_t scan_for_setup_mesh(char *mesh_id_out, size_t mesh_id_len, uint8_t *channel_out);
static esp_err_t send_setup_message(const char *type, const char *pin, const char *mesh_id);
static void setup_heartbeat_task(void *arg);
static esp_err_t handle_write_config_command(cJSON *params);
static void send_setup_config_confirmation(void);

// I2C пины (из MESH_PINOUT_ALL_NODES.md)
#define GPIO_I2C_SCL    17
#define GPIO_I2C_SDA    18
#define I2C_FREQ_HZ     100000

/**
 * @brief Callback при получении данных от ROOT
 */
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
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

    if (!is_write_config && strcmp(msg.node_id, g_config.base.node_id) != 0) {
        mesh_protocol_free_message(&msg);
        free(data_copy);
        return;
    }

    ESP_LOGI(TAG, "Message from ROOT: type=%d", msg.type);

    switch (msg.type) {
        case MESH_MSG_COMMAND: {
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
                climate_controller_handle_command(cmd->valuestring, params ? params : msg.data);
            }
            break;
        }

        case MESH_MSG_CONFIG:
            climate_controller_handle_config_update(msg.data);
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }

    mesh_protocol_free_message(&msg);
    free(data_copy);
}

/**
 * @brief Главная функция приложения
 */
void app_main(void) {
    ESP_ERROR_CHECK(node_config_init());

    bool is_configured = node_config_is_configured();
    if (!is_configured) {
        s_is_setup_mode = true;
        run_setup_mode();
        return;
    }

    s_is_setup_mode = false;
    run_normal_mode();
}

static void run_normal_mode(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== NODE Climate Starting ===");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32 - Temperature, Humidity, CO2, Lux");
    ESP_LOGI(TAG, "Build: %s %s", __DATE__, __TIME__);

    ESP_LOGI(TAG, "[Step 1/7] Loading configuration...");
    if (node_config_load(&g_config, sizeof(g_config), "climate_ns") != ESP_OK) {
        ESP_LOGW(TAG, "Config not found, using defaults");
        node_config_reset_to_default(&g_config, "climate");
        strcpy(g_config.base.node_id, "climate_001");
        strcpy(g_config.base.zone, "Greenhouse Zone 1");
        node_config_save(&g_config, sizeof(g_config), "climate_ns");
    }

    if (zone_config_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize zone_config");
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

    ESP_LOGI(TAG, "Loaded: %s (%s)", g_config.base.node_id, g_config.base.zone);
    ESP_LOGI(TAG, "Zone: mesh_id=%s, root_id=%s", s_mesh_network_id, s_root_node_id);
    ESP_LOGI(TAG, "Targets: Temp=%.1f°C, Hum=%.0f%%, CO2=%dppm, Lux=%d",
             g_config.temp_target, g_config.humidity_target, 
             g_config.co2_max, g_config.lux_min);

    ESP_LOGI(TAG, "[Step 2/7] Initializing I2C...");
    ESP_LOGW(TAG, "I2C DISABLED - Running in MOCK mode");

    ESP_LOGI(TAG, "[Step 3/7] Initializing Sensors...");
    ESP_LOGW(TAG, "SENSORS DISABLED - Running in MOCK mode");
    
    ESP_LOGI(TAG, "[Step 4/7] Initializing Mesh (NODE mode)...");
    char mesh_network_id[ZONE_CONFIG_MAX_LEN] = {0};
    if (zone_config_validate(s_mesh_network_id)) {
        strncpy(mesh_network_id, s_mesh_network_id, sizeof(mesh_network_id) - 1);
    } else if (node_config_get_mesh_network_id(mesh_network_id) != ESP_OK || mesh_network_id[0] == '\0') {
        strncpy(mesh_network_id, MESH_NETWORK_ID, sizeof(mesh_network_id) - 1);
    }
    mesh_network_id[sizeof(mesh_network_id) - 1] = '\0';
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = mesh_network_id,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = 6,
        .router_ssid = NULL,
        .router_password = NULL,
        .router_bssid = NULL
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());
    ESP_LOGI(TAG, "Mesh started with ID %s", mesh_config.mesh_id);

    ESP_LOGI(TAG, "[Step 5/7] Initializing Climate Controller...");
    ESP_ERROR_CHECK(climate_controller_init(&g_config));

    ESP_LOGI(TAG, "[Step 6/7] Starting Climate Controller...");
    ESP_ERROR_CHECK(climate_controller_start());

    ESP_LOGI(TAG, "[Step 7/7] System Ready");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== NODE Climate Running ===");
    ESP_LOGI(TAG, "Node ID: %s", g_config.base.node_id);
    ESP_LOGI(TAG, "Read interval: %d ms", g_config.read_interval_ms);
    ESP_LOGI(TAG, "========================================");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000));
        ESP_LOGI(TAG, "Status: Mesh=%s", mesh_manager_is_connected() ? "ONLINE" : "OFFLINE");
    }
}

static void run_setup_mode(void) {
    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "=== NODE Climate SETUP MODE ===");
    ESP_LOGW(TAG, "========================================");

    memset(&g_config, 0, sizeof(g_config));
    memset(s_setup_pin, 0, sizeof(s_setup_pin));
    memset(s_setup_mesh_id, 0, sizeof(s_setup_mesh_id));
    node_config_generate_setup_pin(s_setup_pin, sizeof(s_setup_pin));
    ESP_LOGI(TAG, "Setup PIN: %s", s_setup_pin);

    ESP_LOGI(TAG, "Scanning for setup mesh...");
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
        .mesh_id = s_setup_mesh_id,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = s_setup_mesh_channel,
        .max_connection = 6,
        .router_ssid = NULL,
        .router_password = NULL,
        .router_bssid = NULL,
    };
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
        ESP_LOGE(TAG, "Failed to create semaphore for setup");
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
    size_t prefix_len = strlen(SETUP_MESH_PREFIX);

    for (uint16_t i = 0; i < ap_count; ++i) {
        const char *ssid = (const char *)ap_records[i].ssid;
        if (ssid[0] == '\0') {
            continue;
        }
        if (strncmp(ssid, SETUP_MESH_PREFIX, prefix_len) == 0) {
            if (!found || ap_records[i].rssi > best_rssi) {
                best_rssi = ap_records[i].rssi;
                best_channel = ap_records[i].primary;
                strncpy(best_ssid, ssid, sizeof(best_ssid) - 1);
                found = true;
            }
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
    cJSON_AddStringToObject(root, "node_type", "climate");
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    if (pin && pin[0] != '\0') {
        cJSON_AddStringToObject(root, "pin", pin);
    }
    if (mesh_id && mesh_id[0] != '\0') {
        cJSON_AddStringToObject(root, "temp_mesh_id", mesh_id);
    }
    const char *mesh_field = zone_config_validate(s_mesh_network_id) && s_mesh_network_id[0] != '\0'
                                 ? s_mesh_network_id
                                 : ZONE_CONFIG_UNCONFIGURED;
    const char *root_field = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_field);
    cJSON_AddStringToObject(root, "root_node_id", root_field);

    const esp_app_desc_t *app_desc = esp_app_get_description();
    if (app_desc && app_desc->version[0] != '\0') {
        cJSON_AddStringToObject(root, "firmware_version", app_desc->version);
    }

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    const char *chip_model_str = "ESP32";
    switch (chip_info.model) {
        case ESP_CHIP_MODEL_ESP32:
            chip_model_str = "ESP32";
            break;
        case ESP_CHIP_MODEL_ESP32S2:
            chip_model_str = "ESP32-S2";
            break;
        case ESP_CHIP_MODEL_ESP32S3:
            chip_model_str = "ESP32-S3";
            break;
        case ESP_CHIP_MODEL_ESP32C3:
            chip_model_str = "ESP32-C3";
            break;
        default:
            chip_model_str = "ESP32";
            break;
    }
    cJSON_AddStringToObject(root, "chip_model", chip_model_str);

    if (strcmp(type, "heartbeat") == 0) {
        uint32_t uptime_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
        cJSON_AddNumberToObject(root, "uptime", uptime_ms);
        cJSON_AddNumberToObject(root, "free_heap", (double)esp_get_free_heap_size());
        int8_t rssi = mesh_manager_get_parent_rssi();
        if (rssi != 0) {
            cJSON_AddNumberToObject(root, "rssi", rssi);
        }
    }

    char *payload = cJSON_PrintUnformatted(root);
    if (!payload) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = mesh_manager_send_to_root((const uint8_t *)payload, strlen(payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mesh_manager_send_to_root(%s) failed: %s", type, esp_err_to_name(err));
    }

    free(payload);
    cJSON_Delete(root);
    return err;
}

static void setup_heartbeat_task(void *arg)
{
    while (s_setup_active) {
        vTaskDelay(pdMS_TO_TICKS(SETUP_HEARTBEAT_INTERVAL_MS));
        if (!s_setup_active) {
            break;
        }
        if (send_setup_message("heartbeat", s_setup_pin, s_setup_mesh_id) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send heartbeat in setup mode");
        }
    }

    s_setup_heartbeat_task = NULL;
    vTaskDelete(NULL);
}

static esp_err_t handle_write_config_command(cJSON *params)
{
    if (params == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *pin_item = cJSON_GetObjectItem(params, "pin");
    if (!cJSON_IsString(pin_item)) {
        ESP_LOGE(TAG, "write_config without PIN");
        return ESP_ERR_INVALID_ARG;
    }
    if (strncmp(pin_item->valuestring, s_setup_pin, sizeof(s_setup_pin) - 1) != 0) {
        ESP_LOGE(TAG, "PIN mismatch (expected %s, got %s)", s_setup_pin, pin_item->valuestring);
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *node_id_item = cJSON_GetObjectItem(params, "node_id");
    if (!cJSON_IsString(node_id_item)) {
        ESP_LOGE(TAG, "write_config missing node_id");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *mesh_id_item = cJSON_GetObjectItem(params, "mesh_network_id");
    if (!cJSON_IsString(mesh_id_item)) {
        mesh_id_item = cJSON_GetObjectItem(params, "mesh_id");
    }
    if (!cJSON_IsString(mesh_id_item)) {
        ESP_LOGE(TAG, "write_config missing mesh_network_id");
        return ESP_ERR_INVALID_ARG;
    }

    const char *node_id = node_id_item->valuestring;
    const char *mesh_id = mesh_id_item->valuestring;

    strncpy(g_config.base.node_id, node_id, sizeof(g_config.base.node_id) - 1);
    strncpy(s_setup_mesh_id, mesh_id, sizeof(s_setup_mesh_id) - 1);

    cJSON *root_node_id_item = cJSON_GetObjectItem(params, "root_node_id");
    const char *root_id_str = (cJSON_IsString(root_node_id_item) && root_node_id_item->valuestring[0] != '\0')
                                  ? root_node_id_item->valuestring
                                  : (s_root_node_id[0] != '\0' ? s_root_node_id : node_id);

    strncpy(g_config.base.root_node_id, root_id_str, sizeof(g_config.base.root_node_id) - 1);
    strncpy(s_root_node_id, root_id_str, sizeof(s_root_node_id) - 1);

    esp_err_t zone_err = zone_config_save(mesh_id, root_id_str);
    if (zone_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save zone config: %s", esp_err_to_name(zone_err));
        return zone_err;
    }
    node_config_set_root_node_id(root_id_str);

    cJSON *zone_item = cJSON_GetObjectItem(params, "zone");
    if (cJSON_IsString(zone_item)) {
        strncpy(g_config.base.zone, zone_item->valuestring, sizeof(g_config.base.zone) - 1);
    }

    g_config.base.config_valid = true;
    g_config.base.last_updated = (uint64_t)time(NULL);

    strncpy(s_mesh_network_id, mesh_id, sizeof(s_mesh_network_id) - 1);

    esp_err_t err = node_config_set_mesh_network_id(mesh_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save mesh_id: %s", esp_err_to_name(err));
        return err;
    }

    bool config_updated = false;
    cJSON *config_obj = cJSON_GetObjectItem(params, "config");
    if (cJSON_IsObject(config_obj)) {
        if (node_config_update_from_json(&g_config, config_obj, "climate") != ESP_OK) {
            ESP_LOGW(TAG, "Unable to update climate config from JSON");
        } else {
            config_updated = true;
        }
    }

    if (!config_updated) {
        g_config.base.config_version++;
        g_config.base.last_updated = (uint64_t)time(NULL);
        g_config.base.config_valid = true;
    }

    err = node_config_save(&g_config, sizeof(g_config), "climate_ns");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save climate config: %s", esp_err_to_name(err));
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
    cJSON_AddStringToObject(root, "node_id", g_config.base.node_id);
    const char *mesh_confirm = zone_config_validate(s_mesh_network_id) ? s_mesh_network_id : s_setup_mesh_id;
    cJSON_AddStringToObject(root, "mesh_id", mesh_confirm);
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_confirm);
    cJSON_AddStringToObject(root, "status", "success");
    if (s_setup_pin[0] != '\0') {
        cJSON_AddStringToObject(root, "pin", s_setup_pin);
    }
    const char *root_confirm = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "root_node_id", root_confirm);
    if (g_config.base.zone[0] != '\0') {
        cJSON_AddStringToObject(root, "zone", g_config.base.zone);
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
