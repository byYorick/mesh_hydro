/**
 * @file app_main.c
 * @brief NODE Climate - датчики климата
 * 
 * ESP32 - Temperature, Humidity, CO2, Lux sensors
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/i2c.h"

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "mesh_config.h"  // Централизованная конфигурация

// NODE Climate компоненты
#include "climate_controller.h"
#include "sht3x_driver.h"
#include "ccs811_driver.h"
#include "lux_sensor.h"

static const char *TAG = "CLIMATE";

// Глобальная конфигурация
static climate_node_config_t g_config;

// I2C пины (из MESH_PINOUT_ALL_NODES.md)
#define GPIO_I2C_SCL    17
#define GPIO_I2C_SDA    18
#define I2C_FREQ_HZ     100000

/**
 * @brief Callback при получении данных от ROOT
 */
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    
    if (!mesh_protocol_parse((const char *)data, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        return;
    }

    // Проверка что сообщение для нас
    if (strcmp(msg.node_id, g_config.base.node_id) != 0) {
        mesh_protocol_free_message(&msg);
        return;
    }

    ESP_LOGI(TAG, "Message from ROOT: type=%d", msg.type);

    // Обработка по типу сообщения
    switch (msg.type) {
        case MESH_MSG_COMMAND: {
            cJSON *cmd = cJSON_GetObjectItem(msg.data, "command");
            if (cmd && cJSON_IsString(cmd)) {
                climate_controller_handle_command(cmd->valuestring, msg.data);
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
}

/**
 * @brief Главная функция приложения
 */
void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== NODE Climate Starting ===");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32 - Temperature, Humidity, CO2, Lux");
    ESP_LOGI(TAG, "Build: %s %s", __DATE__, __TIME__);

    // === Шаг 1: Инициализация NVS ===
    ESP_LOGI(TAG, "[Step 1/7] Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs erase");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // === Шаг 2: Загрузка конфигурации ===
    ESP_LOGI(TAG, "[Step 2/7] Loading configuration...");
    if (node_config_load(&g_config, sizeof(g_config), "climate_ns") != ESP_OK) {
        ESP_LOGW(TAG, "Config not found, using defaults");
        node_config_reset_to_default(&g_config, "climate");
        strcpy(g_config.base.node_id, "climate_001");
        strcpy(g_config.base.zone, "Greenhouse Zone 1");
        node_config_save(&g_config, sizeof(g_config), "climate_ns");
    }
    ESP_LOGI(TAG, "Loaded: %s (%s)", g_config.base.node_id, g_config.base.zone);
    ESP_LOGI(TAG, "Targets: Temp=%.1f°C, Hum=%.0f%%, CO2=%dppm, Lux=%d",
             g_config.temp_target, g_config.humidity_target, 
             g_config.co2_max, g_config.lux_min);

    // === Шаг 3: Инициализация I2C ===
    ESP_LOGI(TAG, "[Step 3/7] Initializing I2C...");
    ESP_LOGW(TAG, "I2C DISABLED - Running in MOCK mode");
    // i2c_config_t i2c_conf = {
    //     .mode = I2C_MODE_MASTER,
    //     .sda_io_num = GPIO_I2C_SDA,
    //     .scl_io_num = GPIO_I2C_SCL,
    //     .sda_pullup_en = GPIO_PULLUP_ENABLE,
    //     .scl_pullup_en = GPIO_PULLUP_ENABLE,
    //     .master.clk_speed = I2C_FREQ_HZ,
    // };
    // ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_conf));
    // ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
    ESP_LOGI(TAG, "I2C MOCK: Skipping hardware init");

    // === Шаг 4: Инициализация датчиков ===
    ESP_LOGI(TAG, "[Step 4/7] Initializing Sensors...");
    ESP_LOGW(TAG, "SENSORS DISABLED - Running in MOCK mode");
    
    ESP_LOGI(TAG, "  - SHT3x (temp + humidity)... MOCK");
    // MOCK: всегда успешно
    // esp_err_t ret_sht = sht3x_init(I2C_NUM_0);
    // if (ret_sht != ESP_OK) {
    //     ESP_LOGW(TAG, "    WARNING: SHT3x init failed (0x%x). Sensor may not be connected.", ret_sht);
    // } else {
    //     ESP_LOGI(TAG, "    OK");
    // }
    
    ESP_LOGI(TAG, "  - CCS811 (CO2)... MOCK");
    // MOCK: всегда успешно
    // esp_err_t ret_ccs = ccs811_init(I2C_NUM_0);
    // if (ret_ccs != ESP_OK) {
    //     ESP_LOGW(TAG, "    WARNING: CCS811 init failed (0x%x). Sensor may not be connected.", ret_ccs);
    // } else {
    //     ESP_LOGI(TAG, "    OK");
    // }
    
    ESP_LOGI(TAG, "  - Lux sensor... MOCK");
    // MOCK: всегда успешно
    // esp_err_t ret_lux = lux_sensor_init(I2C_NUM_0);
    // if (ret_lux != ESP_OK) {
    //     ESP_LOGW(TAG, "    WARNING: Lux sensor init failed (0x%x). Sensor may not be connected.", ret_lux);
    // } else {
    //     ESP_LOGI(TAG, "    OK");
    // }
    
    ESP_LOGI(TAG, "All sensors initialized in MOCK mode");
    // if (ret_sht == ESP_OK || ret_ccs == ESP_OK || ret_lux == ESP_OK) {
    //     ESP_LOGI(TAG, "At least one sensor initialized successfully");
    // } else {
    //     ESP_LOGW(TAG, "WARNING: All sensors failed to initialize. Running in degraded mode.");
    // }

    // === Шаг 5: Инициализация Mesh (NODE режим) ===
    ESP_LOGI(TAG, "[Step 5/7] Initializing Mesh (NODE mode)...");
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = MESH_NETWORK_ID,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = 6,          // Макс подключений для NODE AP
        // ВАЖНО: NODE нужны router settings для ESP-MESH API
        // Но NODE НЕ будет голосовать за ROOT (vote percentage = 0%)
        .router_ssid = MESH_ROUTER_SSID,
        .router_password = MESH_ROUTER_PASSWORD,
        .router_bssid = NULL
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());
    ESP_LOGI(TAG, "Mesh started");

    // === Шаг 6: Инициализация Climate Controller ===
    ESP_LOGI(TAG, "[Step 6/7] Initializing Climate Controller...");
    ESP_ERROR_CHECK(climate_controller_init(&g_config));

    // === Шаг 7: Запуск главной задачи ===
    ESP_LOGI(TAG, "[Step 7/7] Starting Climate Controller...");
    ESP_ERROR_CHECK(climate_controller_start());

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== NODE Climate Running ===");
    ESP_LOGI(TAG, "Node ID: %s", g_config.base.node_id);
    ESP_LOGI(TAG, "Read interval: %d ms", g_config.read_interval_ms);
    ESP_LOGI(TAG, "========================================");

    ESP_LOGI(TAG, "All systems operational. Monitoring climate...");
}
