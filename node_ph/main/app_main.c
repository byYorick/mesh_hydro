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
#include "nvs_flash.h"
#include "driver/i2c.h"

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "mesh_config.h"

// Компоненты pH
#include "ph_sensor.h"
#include "pump_controller.h"
#include "ph_manager.h"

// Для JSON парсинга
#include "cJSON.h"

static const char *TAG = "ph_node";

// I2C конфигурация для ESP32-C3
#define I2C_MASTER_SCL_IO   9
#define I2C_MASTER_SDA_IO   8
#define I2C_MASTER_FREQ_HZ  100000
#define I2C_MASTER_NUM      I2C_NUM_0

// Конфигурация узла
static ph_node_config_t s_node_config;

// Forward declarations
static esp_err_t i2c_master_init(void);
static void init_default_config(void);
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len);

void app_main(void)
{
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  NODE pH Starting...                   ║");
    ESP_LOGI(TAG, "║  ESP32-C3 - pH Control Node            ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    
    // [Step 1/8] NVS init
    ESP_LOGI(TAG, "[Step 1/8] NVS init...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // [Step 2/8] Загрузка конфигурации
    ESP_LOGI(TAG, "[Step 2/8] Loading config...");
    ret = node_config_load(&s_node_config, sizeof(ph_node_config_t), "ph_ns");
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Config not found, using defaults");
        init_default_config();
        node_config_save(&s_node_config, sizeof(ph_node_config_t), "ph_ns");
    }
    ESP_LOGI(TAG, "  Node ID: %s", s_node_config.base.node_id);
    ESP_LOGI(TAG, "  pH target: %.2f (%.2f-%.2f)", 
             s_node_config.ph_target, s_node_config.ph_min, s_node_config.ph_max);
    
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
    ESP_LOGI(TAG, "  - 2 pumps ready (GPIO 2,3)");
    
    // [Step 6/8] Mesh NODE mode init
    ESP_LOGI(TAG, "[Step 6/8] Mesh NODE mode init...");
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = MESH_NETWORK_ID,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = 6,
        .router_ssid = MESH_ROUTER_SSID,
        .router_password = MESH_ROUTER_PASSWORD,
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
    
    if (!mesh_protocol_parse(data_copy, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        free(data_copy);
        return;
    }

    // Проверка что сообщение для нас
    if (strcmp(msg.node_id, s_node_config.base.node_id) != 0) {
        mesh_protocol_free_message(&msg);
        free(data_copy);
        return;
    }

    ESP_LOGI(TAG, "Message from ROOT: type=%d", msg.type);

    // Обработка по типу сообщения
    switch (msg.type) {
        case MESH_MSG_COMMAND: {
            cJSON *cmd = cJSON_GetObjectItem(msg.data, "command");
            cJSON *params = cJSON_GetObjectItem(msg.data, "params");
            
            if (cmd && cJSON_IsString(cmd)) {
                // Передаем params (или msg.data если params нет)
                ph_manager_handle_command(cmd->valuestring, params ? params : msg.data);
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

