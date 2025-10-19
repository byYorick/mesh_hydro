/**
 * @file app_main.c
 * @brief NODE pH/EC - главный файл
 * 
 * ESP32-S3 #3 - критичный узел управления pH и EC
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/i2c.h"

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "mesh_config.h"  // Централизованная конфигурация

// Компоненты pH/EC
#include "ph_sensor.h"
#include "ec_sensor.h"
#include "pump_controller.h"
#include "ph_ec_manager.h"

static const char *TAG = "ph_ec_node";

// I2C конфигурация
#define I2C_MASTER_SCL_IO   17
#define I2C_MASTER_SDA_IO   18
#define I2C_MASTER_FREQ_HZ  100000
#define I2C_MASTER_NUM      I2C_NUM_0

// Конфигурация узла
static ph_ec_node_config_t s_node_config;

// Forward declarations
static esp_err_t i2c_master_init(void);
static void init_default_config(void);

void app_main(void)
{
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  NODE pH/EC Starting...                ║");
    ESP_LOGI(TAG, "║  ESP32-S3 #3 - Critical Node           ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    
    // [Step 1/9] NVS init
    ESP_LOGI(TAG, "[Step 1/9] NVS init...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // [Step 2/9] Загрузка конфигурации
    ESP_LOGI(TAG, "[Step 2/9] Loading config...");
    ret = node_config_load(&s_node_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Config not found, using defaults");
        init_default_config();
        node_config_save(&s_node_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
    }
    ESP_LOGI(TAG, "  Node ID: %s", s_node_config.base.node_id);
    ESP_LOGI(TAG, "  pH target: %.2f (%.2f-%.2f)", 
             s_node_config.ph_target, s_node_config.ph_min, s_node_config.ph_max);
    ESP_LOGI(TAG, "  EC target: %.2f (%.2f-%.2f)",
             s_node_config.ec_target, s_node_config.ec_min, s_node_config.ec_max);
    
    // [Step 3/9] I2C init
    ESP_LOGI(TAG, "[Step 3/9] I2C init (SCL=%d, SDA=%d)...",
             I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO);
    ESP_ERROR_CHECK(i2c_master_init());
    
    // [Step 4/9] Sensors init
    ESP_LOGI(TAG, "[Step 4/9] Sensors init...");
    
    ESP_LOGI(TAG, "  - pH sensor (0x%02X)...", PH_SENSOR_ADDR);
    esp_err_t ret_ph = ph_sensor_init(I2C_MASTER_NUM);
    if (ret_ph != ESP_OK) {
        ESP_LOGW(TAG, "    WARNING: pH sensor init failed. Using default values.");
    } else {
        ESP_LOGI(TAG, "    OK");
    }
    
    ESP_LOGI(TAG, "  - EC sensor (0x%02X)...", EC_SENSOR_ADDR);
    esp_err_t ret_ec = ec_sensor_init(I2C_MASTER_NUM);
    if (ret_ec != ESP_OK) {
        ESP_LOGW(TAG, "    WARNING: EC sensor init failed. Using default values.");
    } else {
        ESP_LOGI(TAG, "    OK");
    }
    
    if (ret_ph == ESP_OK || ret_ec == ESP_OK) {
        ESP_LOGI(TAG, "At least one sensor initialized successfully");
    } else {
        ESP_LOGW(TAG, "WARNING: All sensors failed. Running in simulation mode.");
    }
    
    // [Step 5/9] Pumps init
    ESP_LOGI(TAG, "[Step 5/9] Pumps init (5x PWM)...");
    ESP_ERROR_CHECK(pump_controller_init());
    ESP_LOGI(TAG, "  - 5 pumps ready (GPIO 4,5,6,7,15)");
    
    // [Step 6/9] Mesh NODE mode init
    ESP_LOGI(TAG, "[Step 6/9] Mesh NODE mode init...");
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = MESH_NETWORK_ID,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = 6,          // Макс подключений для NODE AP
        // ВАЖНО: NODE нужны настройки роутера для совместимости с ESP-MESH API
        // NODE не будет подключаться к роутеру напрямую, но настройки должны совпадать с ROOT
        .router_ssid = MESH_ROUTER_SSID,
        .router_password = MESH_ROUTER_PASSWORD,
        .router_bssid = NULL
    };
    
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    ESP_LOGI(TAG, "  Mesh ID: %s", mesh_config.mesh_id);
    
    // [Step 7/9] pH/EC Manager init
    ESP_LOGI(TAG, "[Step 7/9] pH/EC Manager init...");
    ESP_ERROR_CHECK(ph_ec_manager_init(&s_node_config));
    
    // [Step 8/9] Starting mesh network
    ESP_LOGI(TAG, "[Step 8/9] Starting mesh network...");
    ESP_ERROR_CHECK(mesh_manager_start());
    
    // [Step 9/9] Starting pH/EC Manager
    ESP_LOGI(TAG, "[Step 9/9] Starting pH/EC Manager...");
    ESP_ERROR_CHECK(ph_ec_manager_start());
    
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  NODE pH/EC Running! ✓                 ║");
    ESP_LOGI(TAG, "║  Autonomous: YES                       ║");
    ESP_LOGI(TAG, "║  Emergency Protection: ACTIVE          ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    
    // Главный цикл - статистика
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000)); // Каждую минуту
        
        float ph, ec;
        ph_ec_manager_get_values(&ph, &ec);
        
        ESP_LOGI(TAG, "Status: pH=%.2f (target %.2f), EC=%.2f (target %.2f), Mesh=%s",
                 ph, s_node_config.ph_target,
                 ec, s_node_config.ec_target,
                 mesh_manager_is_connected() ? "ONLINE" : "OFFLINE");
    }
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
    memset(&s_node_config, 0, sizeof(ph_ec_node_config_t));
    
    // Базовая конфигурация
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(s_node_config.base.node_id, sizeof(s_node_config.base.node_id),
             "ph_ec_%02x%02x%02x", mac[3], mac[4], mac[5]);
    strncpy(s_node_config.base.zone, "Auto-discovered", sizeof(s_node_config.base.zone));
    s_node_config.base.config_valid = true;
    s_node_config.base.config_version = 1;
    
    // pH/EC целевые значения
    s_node_config.ph_target = 6.5f;
    s_node_config.ec_target = 2.5f;
    
    // Диапазоны
    s_node_config.ph_min = 5.5f;
    s_node_config.ph_max = 7.5f;
    s_node_config.ec_min = 1.5f;
    s_node_config.ec_max = 4.0f;
    
    // PID параметры для насосов
    for (int i = 0; i < 5; i++) {
        s_node_config.pump_pid[i].kp = (i < 2) ? 2.0f : 1.5f;  // pH: 2.0, EC: 1.5
        s_node_config.pump_pid[i].ki = (i < 2) ? 0.1f : 0.05f;
        s_node_config.pump_pid[i].kd = (i < 2) ? 0.5f : 0.3f;
        s_node_config.pump_pid[i].output_min = 0.0f;
        s_node_config.pump_pid[i].output_max = (i < 2) ? 5.0f : 10.0f;
        s_node_config.pump_pid[i].enabled = true;
    }
    
    // Калибровка (дефолт)
    s_node_config.ph_calibration_offset = 0.0f;
    s_node_config.ec_calibration_multiplier = 1.0f;
    
    ESP_LOGI(TAG, "Default config initialized: %s", s_node_config.base.node_id);
}
