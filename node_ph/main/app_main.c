/**
 * @file app_main.c
 * @brief NODE pH - главный файл
 * 
 * ESP32-C3 - автономный узел управления pH
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
#include <time.h>

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "mesh_config.h"
#include "zone_config.h"

// Компоненты pH
#include "ph_sensor.h"
#include "pump_controller.h"
#include "ph_manager.h"

// Для JSON парсинга
#include "cJSON.h"

static const char *TAG = "ph_node";

// Глобальные переменные для pump_events
const char *g_node_id = NULL;
bool g_emergency_mode = false;
bool g_autonomous_mode = false;

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
static char s_mesh_network_id[ZONE_CONFIG_MAX_LEN] = {0};
static char s_root_node_id[ZONE_CONFIG_MAX_LEN] = {0};

// I2C конфигурация для ESP32 (стандартные пины)
#define I2C_MASTER_SCL_IO   9
#define I2C_MASTER_SDA_IO   8
#define I2C_MASTER_FREQ_HZ  100000
#define I2C_MASTER_NUM      I2C_NUM_0

// Конфигурация узла
static ph_node_config_t s_node_config;

// Forward declarations
static void run_setup_mode(void);
static void run_normal_mode(void);
static esp_err_t scan_for_setup_mesh(char *mesh_id_out, size_t mesh_id_len, uint8_t *channel_out);
static esp_err_t send_setup_message(const char *type, const char *pin, const char *mesh_id);
static void setup_heartbeat_task(void *arg);
static esp_err_t handle_write_config_command(cJSON *params);
static void send_setup_config_confirmation(void);
static esp_err_t i2c_master_init(void);
static void init_default_config(void);
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len);

void app_main(void)
{
    ESP_ERROR_CHECK(node_config_init());

    // Определяем режим работы узла
    bool is_configured = node_config_is_configured();
    if (!is_configured) {
        s_is_setup_mode = true;
        run_setup_mode();
        return;
    }

    s_is_setup_mode = false;
    run_normal_mode();
}

static void run_normal_mode(void)
{
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  NODE pH Starting...                   ║");
    ESP_LOGI(TAG, "║  ESP32-C3 - pH Control Node            ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    
    // [Step 1/8] Loading config from NVS
    ESP_LOGI(TAG, "[Step 1/8] Loading config...");
    esp_err_t ret = node_config_load(&s_node_config, sizeof(ph_node_config_t), "ph_ns");
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Config not found, using defaults");
        init_default_config();
        node_config_save(&s_node_config, sizeof(ph_node_config_t), "ph_ns");
    } else {
        // Валидация emergency limits - если они не инициализированы (равны 0), установить значения по умолчанию
        if (s_node_config.ph_emergency_low <= 0.0f || s_node_config.ph_emergency_high <= 0.0f ||
            s_node_config.ph_emergency_low >= s_node_config.ph_emergency_high) {
            ESP_LOGW(TAG, "Invalid emergency limits (%.2f-%.2f), setting defaults", 
                     s_node_config.ph_emergency_low, s_node_config.ph_emergency_high);
            s_node_config.ph_emergency_low = 4.0f;   // Критично низкий pH
            s_node_config.ph_emergency_high = 9.0f;   // Критично высокий pH
            node_config_save(&s_node_config, sizeof(ph_node_config_t), "ph_ns");
            ESP_LOGI(TAG, "Emergency limits set to: %.2f-%.2f", 
                     s_node_config.ph_emergency_low, s_node_config.ph_emergency_high);
        }
    }
    
    // Установка глобального node_id для pump_events
    g_node_id = s_node_config.base.node_id;

    if (zone_config_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize zone_config");
        strncpy(s_mesh_network_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_mesh_network_id) - 1);
        strncpy(s_root_node_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_root_node_id) - 1);
    } else if (zone_config_load(s_mesh_network_id, sizeof(s_mesh_network_id),
                                s_root_node_id, sizeof(s_root_node_id)) != ESP_OK) {
        ESP_LOGW(TAG, "Zone config not found, using UNCONFIGURED defaults");
        strncpy(s_mesh_network_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_mesh_network_id) - 1);
        strncpy(s_root_node_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_root_node_id) - 1);
    }
    s_mesh_network_id[sizeof(s_mesh_network_id) - 1] = '\0';
    s_root_node_id[sizeof(s_root_node_id) - 1] = '\0';
    
    ESP_LOGI(TAG, "  Node ID: %s", s_node_config.base.node_id);
    ESP_LOGI(TAG, "  pH target: %.2f (range: %.2f-%.2f)", 
             s_node_config.ph_target, s_node_config.ph_min, s_node_config.ph_max);
    ESP_LOGI(TAG, "  Zone context: mesh_id=%s, root_id=%s",
             s_mesh_network_id, s_root_node_id);
    
    // Установка режима работы датчика pH
    const char *sensor_mode_str;
    switch (s_node_config.sensor_mode) {
        case 0:
            sensor_mode_str = "REAL";
            break;
        case 1:
            sensor_mode_str = "MOCK REACTIVE";
            break;
        case 2:
            sensor_mode_str = "MOCK NON-REACTIVE";
            break;
        default:
            sensor_mode_str = "UNKNOWN";
            s_node_config.sensor_mode = PH_SENSOR_MODE_MOCK_REACTIVE;  // Установка по умолчанию
            break;
    }
    ph_sensor_set_mode((ph_sensor_mode_t)s_node_config.sensor_mode);
    ESP_LOGI(TAG, "  Sensor mode: %s (%d)", sensor_mode_str, s_node_config.sensor_mode);
    ESP_LOGI(TAG, "  Emergency limits: %.2f-%.2f", 
             s_node_config.ph_emergency_low, s_node_config.ph_emergency_high);
    
    // [Step 3/8] I2C init
    ESP_LOGI(TAG, "[Step 3/8] I2C init...");
    ESP_ERROR_CHECK(i2c_master_init());
    
    // [Step 4/8] Sensor init
    ESP_LOGI(TAG, "[Step 4/8] pH Sensor init...");
    esp_err_t ret_ph = ph_sensor_init(I2C_MASTER_NUM);
    if (ret_ph != ESP_OK) {
        ESP_LOGW(TAG, "  ⚠️ pH sensor not found - using mock values");
    } else {
        ESP_LOGI(TAG, "  ✅ pH sensor ready");
    }
    
    // [Step 5/8] Pumps init (2 насоса)
    ESP_LOGI(TAG, "[Step 5/8] Pumps init (2x PWM)...");
    ESP_ERROR_CHECK(pump_controller_init());
    ESP_LOGI(TAG, "  - 2 pumps ready (GPIO 12,13 - pH UP/DOWN)");
    
    // [Step 6/8] Mesh NODE mode init
    ESP_LOGI(TAG, "[Step 6/8] Mesh NODE mode init...");
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
    ESP_LOGI(TAG, "  Mesh ID: %s", mesh_config.mesh_id);
    
    // Регистрация callback для команд от ROOT
    mesh_manager_register_recv_cb(on_mesh_data_received);
    
    // [Step 7/8] pH Manager init
    ESP_LOGI(TAG, "[Step 7/8] pH Manager init...");
    ESP_ERROR_CHECK(ph_manager_init(&s_node_config));
    
    // [Step 8/8] Starting
    ESP_LOGI(TAG, "[Step 8/8] Starting...");
    ESP_ERROR_CHECK(mesh_manager_start());
    ESP_ERROR_CHECK(ph_manager_start());
    
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  NODE pH Running! ✓                    ║");
    ESP_LOGI(TAG, "║  Autonomous: YES                       ║");
    ESP_LOGI(TAG, "║  Emergency Protection: ACTIVE          ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    
    // Главный цикл - статистика
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000)); // Каждую минуту
        
        float ph;
        ph_manager_get_value(&ph);
        
        ESP_LOGI(TAG, "Status: pH=%.2f (target %.2f), Mesh=%s",
                 ph, s_node_config.ph_target,
                 mesh_manager_is_connected() ? "ONLINE" : "OFFLINE");
    }
}

static void run_setup_mode(void)
{
    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "=== NODE pH SETUP MODE ACTIVATED ===");
    ESP_LOGW(TAG, "========================================");

    memset(s_setup_pin, 0, sizeof(s_setup_pin));
    memset(s_setup_mesh_id, 0, sizeof(s_setup_mesh_id));
    node_config_generate_setup_pin(s_setup_pin, sizeof(s_setup_pin));

    ESP_LOGI(TAG, "Setup PIN: %s", s_setup_pin);
    ESP_LOGI(TAG, "Ищем временную mesh сеть с префиксом %s", SETUP_MESH_PREFIX);

    esp_err_t scan_err;
    uint32_t attempt = 0;
    do {
        attempt++;
        scan_err = scan_for_setup_mesh(s_setup_mesh_id, sizeof(s_setup_mesh_id), &s_setup_mesh_channel);
        if (scan_err != ESP_OK) {
            ESP_LOGW(TAG, "[%u] Сети с префиксом %s не найдены. Повтор через %d мс", attempt,
                     SETUP_MESH_PREFIX, SETUP_DISCOVERY_RETRY_MS);
            vTaskDelay(pdMS_TO_TICKS(SETUP_DISCOVERY_RETRY_MS));
        }
    } while (scan_err != ESP_OK);

    ESP_LOGI(TAG, "Найдена сеть: %s (channel=%u)", s_setup_mesh_id, s_setup_mesh_channel);

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
        ESP_LOGW(TAG, "Не удалось подключиться к mesh %s, продолжаем попытки отправки discovery", s_setup_mesh_id);
    }

    if (send_setup_message("discovery", s_setup_pin, s_setup_mesh_id) != ESP_OK) {
        ESP_LOGW(TAG, "Не удалось отправить discovery сообщение в mesh");
    }

    s_setup_active = true;
    if (xTaskCreate(setup_heartbeat_task, "setup_heartbeat", 4096, NULL, 5, &s_setup_heartbeat_task) != pdPASS) {
        ESP_LOGE(TAG, "Не удалось создать задачу heartbeat для setup режима");
        s_setup_heartbeat_task = NULL;
    }

    s_setup_config_sem = xSemaphoreCreateBinary();
    if (!s_setup_config_sem) {
        ESP_LOGE(TAG, "Не удалось создать семафор ожидания конфигурации");
    }

    ESP_LOGI(TAG, "Ожидаем команду write_config через mesh...");
    if (s_setup_config_sem) {
        xSemaphoreTake(s_setup_config_sem, portMAX_DELAY);
        vSemaphoreDelete(s_setup_config_sem);
        s_setup_config_sem = NULL;
    }

    s_setup_active = false;
    if (s_setup_heartbeat_task) {
        // Ждём завершения задачи heartbeat
        while (eTaskGetState(s_setup_heartbeat_task) != eDeleted) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        s_setup_heartbeat_task = NULL;
    }

    ESP_LOGI(TAG, "Конфигурация получена. Перезапуск устройства...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    mesh_manager_stop();
    esp_restart();
}

static void setup_heartbeat_task(void *arg)
{
    while (s_setup_active) {
        vTaskDelay(pdMS_TO_TICKS(SETUP_HEARTBEAT_INTERVAL_MS));
        if (!s_setup_active) {
            break;
        }
        if (send_setup_message("heartbeat", s_setup_pin, s_setup_mesh_id) != ESP_OK) {
            ESP_LOGW(TAG, "Не удалось отправить heartbeat в setup режиме");
        }
    }

    s_setup_heartbeat_task = NULL;
    vTaskDelete(NULL);
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

    ESP_LOGI(TAG, "Лучшая сеть: %s (RSSI=%d, channel=%u)", mesh_id_out, best_rssi, best_channel);
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
    cJSON_AddStringToObject(root, "node_type", "ph");
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
#if defined(ESP_CHIP_MODEL_ESP32) && (ESP_CHIP_MODEL_ESP32 >= 0)
    if (chip_info.model == ESP_CHIP_MODEL_ESP32) {
        chip_model_str = "ESP32";
    } else
#endif
#if defined(ESP_CHIP_MODEL_ESP32S2) && (ESP_CHIP_MODEL_ESP32S2 >= 0)
    if (chip_info.model == ESP_CHIP_MODEL_ESP32S2) {
        chip_model_str = "ESP32-S2";
    } else
#endif
#if defined(ESP_CHIP_MODEL_ESP32S3) && (ESP_CHIP_MODEL_ESP32S3 >= 0)
    if (chip_info.model == ESP_CHIP_MODEL_ESP32S3) {
        chip_model_str = "ESP32-S3";
    } else
#endif
#if defined(ESP_CHIP_MODEL_ESP32C3) && (ESP_CHIP_MODEL_ESP32C3 >= 0)
    if (chip_info.model == ESP_CHIP_MODEL_ESP32C3) {
        chip_model_str = "ESP32-C3";
    } else
#endif
    {
        chip_model_str = "ESP32";
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

    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh не подключён, отправка %s может не пройти", type);
    }

    esp_err_t err = mesh_manager_send_to_root((const uint8_t *)payload, strlen(payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mesh_manager_send_to_root(%s) failed: %s", type, esp_err_to_name(err));
    }

    free(payload);
    cJSON_Delete(root);
    return err;
}

static esp_err_t handle_write_config_command(cJSON *params)
{
    if (params == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *pin_item = cJSON_GetObjectItem(params, "pin");
    if (!cJSON_IsString(pin_item)) {
        ESP_LOGE(TAG, "write_config без PIN");
        return ESP_ERR_INVALID_ARG;
    }
    if (strncmp(pin_item->valuestring, s_setup_pin, sizeof(s_setup_pin) - 1) != 0) {
        ESP_LOGE(TAG, "PIN не совпадает (ожидается %s, получено %s)", s_setup_pin, pin_item->valuestring);
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *node_id_item = cJSON_GetObjectItem(params, "node_id");
    if (!cJSON_IsString(node_id_item)) {
        ESP_LOGE(TAG, "write_config без node_id");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *mesh_id_item = cJSON_GetObjectItem(params, "mesh_network_id");
    if (!cJSON_IsString(mesh_id_item)) {
        mesh_id_item = cJSON_GetObjectItem(params, "mesh_id");
    }
    if (!cJSON_IsString(mesh_id_item)) {
        ESP_LOGE(TAG, "write_config без mesh_network_id");
        return ESP_ERR_INVALID_ARG;
    }

    const char *node_id = node_id_item->valuestring;
    const char *mesh_id = mesh_id_item->valuestring;

    strncpy(s_node_config.base.node_id, node_id, sizeof(s_node_config.base.node_id) - 1);
    g_node_id = s_node_config.base.node_id;
    strncpy(s_setup_mesh_id, mesh_id, sizeof(s_setup_mesh_id) - 1);

    cJSON *root_node_id_item = cJSON_GetObjectItem(params, "root_node_id");
    const char *root_id_str = (cJSON_IsString(root_node_id_item) && root_node_id_item->valuestring[0] != '\0')
                                  ? root_node_id_item->valuestring
                                  : (s_root_node_id[0] != '\0' ? s_root_node_id : node_id);

    strncpy(s_node_config.base.root_node_id, root_id_str, sizeof(s_node_config.base.root_node_id) - 1);
    strncpy(s_root_node_id, root_id_str, sizeof(s_root_node_id) - 1);

    esp_err_t zone_err = zone_config_save(mesh_id, root_id_str);
    if (zone_err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось сохранить zone_config: %s", esp_err_to_name(zone_err));
        return zone_err;
    }
    node_config_set_root_node_id(root_id_str);

    cJSON *zone_item = cJSON_GetObjectItem(params, "zone");
    if (cJSON_IsString(zone_item)) {
        strncpy(s_node_config.base.zone, zone_item->valuestring, sizeof(s_node_config.base.zone) - 1);
    }

    s_node_config.base.config_valid = true;
    s_node_config.base.config_version++;
    s_node_config.base.last_updated = (uint64_t)time(NULL);

    strncpy(s_mesh_network_id, mesh_id, sizeof(s_mesh_network_id) - 1);

    esp_err_t err = node_config_set_mesh_network_id(mesh_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось записать mesh_id в NVS: %s", esp_err_to_name(err));
        return err;
    }

    bool config_updated = false;
    cJSON *config_obj = cJSON_GetObjectItem(params, "config");
    if (cJSON_IsObject(config_obj)) {
        if (node_config_update_from_json(&s_node_config, config_obj, "ph") != ESP_OK) {
            ESP_LOGW(TAG, "Не удалось обновить специализированную конфигурацию, используются дефолтные значения");
        } else {
            config_updated = true;
        }
    }

    if (!config_updated) {
        s_node_config.base.config_version++;
        s_node_config.base.last_updated = (uint64_t)time(NULL);
        s_node_config.base.config_valid = true;
    }

    err = node_config_save(&s_node_config, sizeof(ph_node_config_t), "ph_ns");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Сохранение конфигурации пH узла завершилось ошибкой: %s", esp_err_to_name(err));
        return err;
    }

    err = node_config_mark_configured(true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось пометить узел как настроенный: %s", esp_err_to_name(err));
        return err;
    }

    s_is_setup_mode = false;
    send_setup_config_confirmation();
    return ESP_OK;
}

static void send_setup_config_confirmation(void)
{
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh не подключён, config_confirmation не будет отправлен");
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
    const char *mesh_confirm = zone_config_validate(s_mesh_network_id) ? s_mesh_network_id : s_setup_mesh_id;
    cJSON_AddStringToObject(root, "mesh_id", mesh_confirm);
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_confirm);
    cJSON_AddStringToObject(root, "status", "success");
    if (s_setup_pin[0] != '\0') {
        cJSON_AddStringToObject(root, "pin", s_setup_pin);
    }
    const char *root_confirm = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "root_node_id", root_confirm);
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
        ESP_LOGE(TAG, "Не удалось отправить config_confirmation: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Config confirmation отправлен через mesh");
    }

    free(payload);
    cJSON_Delete(root);
}

// Инициализация I2C
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        return ret;
    }
    
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// Инициализация дефолтной конфигурации
static void init_default_config(void) {
    memset(&s_node_config, 0, sizeof(ph_node_config_t));
    
    // Базовая конфигурация
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(s_node_config.base.node_id, sizeof(s_node_config.base.node_id),
             "ph_%02x%02x%02x", mac[3], mac[4], mac[5]);
    strncpy(s_node_config.base.node_type, "ph", sizeof(s_node_config.base.node_type));
    strncpy(s_node_config.base.zone, "Auto-discovered", sizeof(s_node_config.base.zone));
    s_node_config.base.config_valid = true;
    s_node_config.base.config_version = 1;
    
    // pH целевое значение
    s_node_config.ph_target = 6.5f;
    
    // Диапазоны
    s_node_config.ph_min = 5.5f;
    s_node_config.ph_max = 7.5f;
    
    // Аварийные лимиты (критические границы)
    s_node_config.ph_emergency_low = 4.0f;   // Критично низкий pH
    s_node_config.ph_emergency_high = 9.0f;  // Критично высокий pH
    
    // Лимиты безопасности насосов
    s_node_config.max_pump_time_ms = 60000;      // Максимум 60 секунд
    s_node_config.cooldown_ms = 5000;            // 5 секунд между запусками
    s_node_config.max_daily_volume_ml = 1000;    // Максимум 1 литр в день
    
    // PID параметры для 2 насосов (консервативные настройки)
    for (int i = 0; i < 2; i++) {
        s_node_config.pump_pid[i].kp = 1.0f;   // Консервативный Kp
        s_node_config.pump_pid[i].ki = 0.05f;  // Консервативный Ki
        s_node_config.pump_pid[i].kd = 0.3f;   // Консервативный Kd
        s_node_config.pump_pid[i].output_min = 0.0f;
        s_node_config.pump_pid[i].output_max = 5.0f;
        s_node_config.pump_pid[i].enabled = true;
    }
    
    // Калибровка насосов (по умолчанию 1 мл/сек)
    for (int i = 0; i < 2; i++) {
        s_node_config.pump_calibration[i].ml_per_second = 1.0f;
        s_node_config.pump_calibration[i].calibration_time_ms = 10000;  // 10 сек
        s_node_config.pump_calibration[i].calibration_volume_ml = 10.0f;  // 10 мл
        s_node_config.pump_calibration[i].is_calibrated = false;
        s_node_config.pump_calibration[i].last_calibrated = 0;
    }
    
    // Калибровка датчика pH (дефолт)
    s_node_config.ph_cal_offset = 0.0f;
    
    // Режим работы датчика pH (0=реальный, 1=mock с реакцией, 2=mock без реакции)
    s_node_config.sensor_mode = PH_SENSOR_MODE_MOCK_REACTIVE;  // По умолчанию mock с реакцией
    
    ESP_LOGI(TAG, "Default config initialized: %s", s_node_config.base.node_id);
}

/**
 * @brief Callback при получении данных от ROOT
 */
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    // Создаём NULL-terminated копию
    char *data_copy = malloc(len + 1);
    if (data_copy == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for data copy");
        return;
    }
    memcpy(data_copy, data, len);
    data_copy[len] = '\0';
    
    mesh_message_t msg;
    
    ESP_LOGI(TAG, "=== JSON PARSING DEBUG ===");
    ESP_LOGI(TAG, "Data length: %d", (int)len);
    ESP_LOGI(TAG, "Data: %s", data_copy);
    
    if (!mesh_protocol_parse(data_copy, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        free(data_copy);
        return;
    }
    
    ESP_LOGI(TAG, "JSON parsed successfully");
    ESP_LOGI(TAG, "Message type: %d", msg.type);
    ESP_LOGI(TAG, "Node ID: %s", msg.node_id);

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

    ESP_LOGI(TAG, "Message from ROOT: type=%d", msg.type);

    switch (msg.type) {
        case MESH_MSG_COMMAND: {
            ESP_LOGI(TAG, "=== PROCESSING COMMAND ===");
            ESP_LOGI(TAG, "msg.data: %s", msg.data ? "found" : "NULL");
            
            if (msg.data) {
                char *data_str = cJSON_PrintUnformatted(msg.data);
                ESP_LOGI(TAG, "msg.data content: %s", data_str ? data_str : "NULL");
                if (data_str) free(data_str);
            }
            
            if (is_write_config) {
                ESP_LOGI(TAG, "Получена команда write_config");
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
            
            cJSON *effective_params = params ? params : msg.data;
            if (cmd && cJSON_IsString(cmd)) {
                ESP_LOGI(TAG, "Calling ph_manager_handle_command...");
                ph_manager_handle_command(cmd->valuestring, effective_params);
                ESP_LOGI(TAG, "ph_manager_handle_command returned");
            } else {
                ESP_LOGW(TAG, "Invalid command format");
            }
            break;
        }

        case MESH_MSG_CONFIG:
            ph_manager_handle_config_update(msg.data);
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }

    mesh_protocol_free_message(&msg);
    free(data_copy);
}

