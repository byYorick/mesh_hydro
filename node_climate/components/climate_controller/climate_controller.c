/**
 * @file climate_controller.c
 * @brief Реализация контроллера климата
 */

#include "climate_controller.h"
#include "sht3x_driver.h"
#include "ccs811_driver.h"
#include "lux_sensor.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "mesh_config.h"  // Для HEARTBEAT_INTERVAL_MS
#include "zone_config.h"
#include "oled_display.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
// #include "esp_task_wdt.h"  // Закомментировано для совместимости с ESP-IDF v5.5
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

static const char *TAG = "climate_ctrl";

static climate_node_config_t *s_config = NULL;
static TaskHandle_t s_main_task = NULL;
static TaskHandle_t s_heartbeat_task = NULL;
static bool s_discovery_sent = false;
static uint32_t s_boot_time = 0;
static char s_root_node_id[32] = {0};
static char s_mesh_network_id[32] = {0};
static bool s_oled_ready = false;
static float s_last_temp = 0.0f;
static float s_last_humidity = 0.0f;
static uint16_t s_last_co2 = 0;
static uint16_t s_last_lux = 0;
static int8_t s_last_rssi = 0;
static bool s_last_mesh_connected = false;
static bool s_display_has_data = false;

// Forward declarations
static void climate_main_task(void *arg);
static void heartbeat_task(void *arg);
static void send_telemetry(float temp, float humidity, uint16_t co2, uint16_t lux);
static void send_discovery(void);
static void send_heartbeat(void);
static void send_event(mesh_event_level_t level, const char *message, float temp, float humidity, uint16_t co2);
static void check_sensor_thresholds(float temp, float humidity, uint16_t co2);
static esp_err_t read_all_sensors(float *temp, float *humidity, uint16_t *co2, uint16_t *lux);
static int8_t get_rssi_to_parent(void);
static void climate_display_init_once(void);
static void climate_display_update(float temp, float humidity, uint16_t co2, uint16_t lux, bool mesh_connected, int8_t rssi);
static void climate_display_show_state(bool mesh_connected);

#define CLIMATE_OLED_I2C_PORT  I2C_NUM_1
#define CLIMATE_OLED_SDA_PIN   GPIO_NUM_25
#define CLIMATE_OLED_SCL_PIN   GPIO_NUM_26

esp_err_t climate_controller_init(climate_node_config_t *config) {
    if (!config) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    s_config = config;
    s_boot_time = (uint32_t)time(NULL);
    s_discovery_sent = false;

    zone_config_init();

    const char *root_id = zone_config_get_root_id();
    if (root_id && root_id[0] != '\0' && zone_config_validate(root_id)) {
        strncpy(s_root_node_id, root_id, sizeof(s_root_node_id) - 1);
    } else if (s_config->base.root_node_id[0] != '\0') {
        strncpy(s_root_node_id, s_config->base.root_node_id, sizeof(s_root_node_id) - 1);
    } else {
        strncpy(s_root_node_id, "root_setup", sizeof(s_root_node_id) - 1);
    }

    const char *mesh_id = zone_config_get_mesh_id();
    if (mesh_id && mesh_id[0] != '\0' && zone_config_validate(mesh_id)) {
        strncpy(s_mesh_network_id, mesh_id, sizeof(s_mesh_network_id) - 1);
    } else if (node_config_get_mesh_network_id(s_mesh_network_id) != ESP_OK || s_mesh_network_id[0] == '\0') {
        strncpy(s_mesh_network_id, MESH_NETWORK_ID, sizeof(s_mesh_network_id) - 1);
    }

    ESP_LOGI(TAG, "Climate Controller initialized");
    ESP_LOGI(TAG, "Node ID: %s, Zone: %s", s_config->base.node_id, s_config->base.zone);
    ESP_LOGI(TAG, "Read interval: %d ms", s_config->read_interval_ms);
    ESP_LOGI(TAG, "Context: root_node_id=%s mesh_network_id=%s", s_root_node_id, s_mesh_network_id);

    climate_display_init_once();

    return ESP_OK;
}

esp_err_t climate_controller_start(void) {
    if (s_main_task != NULL) {
        ESP_LOGW(TAG, "Main task already running");
        return ESP_OK;
    }

    // Запуск главной задачи (telemetry каждые 30 сек)
    // Stack увеличен в 2 раза для безопасности: 4096 → 8192
    BaseType_t ret = xTaskCreate(climate_main_task,
                                  "climate_main",
                                  8192,
                                  NULL,
                                  5,
                                  &s_main_task);
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main task");
        return ESP_FAIL;
    }

    // Запуск задачи heartbeat (каждые 60 сек)
    // Stack увеличен в 2 раза для безопасности: 3072 → 6144
    ret = xTaskCreate(heartbeat_task,
                      "heartbeat",
                      6144,
                      NULL,
                      4,
                      &s_heartbeat_task);
    
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Failed to create heartbeat task");
        // Не критично - продолжаем работу
    }

    ESP_LOGI(TAG, "Climate Controller started");
    return ESP_OK;
}

esp_err_t climate_controller_stop(void) {
    if (s_main_task != NULL) {
        vTaskDelete(s_main_task);
        s_main_task = NULL;
    }
    if (s_heartbeat_task != NULL) {
        vTaskDelete(s_heartbeat_task);
        s_heartbeat_task = NULL;
    }
    ESP_LOGI(TAG, "Climate Controller stopped");
    return ESP_OK;
}

// Главная задача чтения датчиков и отправки telemetry
static void climate_main_task(void *arg) {
    ESP_LOGI(TAG, "Main task running (telemetry every 30 sec)");
    
    // Регистрация в watchdog (закомментировано для ESP-IDF v5.5)
    // esp_task_wdt_add(NULL);

    float temp, humidity;
    uint16_t co2, lux;

    // Отправка discovery сообщения при старте
    // Ожидаем подключения к mesh (до 30 секунд)
    int wait_count = 0;
    while (!mesh_manager_is_connected() && wait_count < 30) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        wait_count++;
    }
    
    // Дополнительная задержка после подключения для стабилизации mesh
    if (mesh_manager_is_connected()) {
        ESP_LOGI(TAG, "Mesh connected, waiting 3 seconds for stabilization...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        send_discovery();
        s_discovery_sent = true;
    }

    while (1) {
        // Сброс watchdog (закомментировано для ESP-IDF v5.5)
        // esp_task_wdt_reset();

        // Если mesh подключен, но discovery не отправлен - отправить
        if (mesh_manager_is_connected() && !s_discovery_sent) {
            send_discovery();
            s_discovery_sent = true;
        }

        // Чтение всех датчиков (или моковых значений в MOCK MODE)
        esp_err_t ret = read_all_sensors(&temp, &humidity, &co2, &lux);

        // ⚠️ MOCK MODE: Всегда отправляем телеметрию (даже с моковыми данными)
        if (ret == ESP_OK) {
            // Отправка телеметрии на ROOT
            send_telemetry(temp, humidity, co2, lux);

            // Проверка пороговых значений и отправка events
            check_sensor_thresholds(temp, humidity, co2);
        }
        // Убрана проверка "else" которая пропускала телеметрию - теперь всегда отправляется!

        // Интервал telemetry - 5 секунд (DEBUG режим!)
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Задача heartbeat - использует единый интервал из mesh_config.h
static void heartbeat_task(void *arg) {
    ESP_LOGI(TAG, "Heartbeat task running (interval: %d ms)", HEARTBEAT_INTERVAL_MS);
    
    // Начальная задержка перед первым heartbeat
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1) {
        if (mesh_manager_is_connected()) {
            send_heartbeat();
        }
        
        // Используем единый интервал из mesh_config.h (10 сек)
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS));
    }
}

// Чтение всех датчиков с retry логикой
static esp_err_t read_all_sensors(float *temp, float *humidity, uint16_t *co2, uint16_t *lux) {
    const int max_retries = 3;
    esp_err_t ret_temp = ESP_FAIL;
    esp_err_t ret_co2 = ESP_FAIL;
    esp_err_t ret_lux = ESP_FAIL;

    // Retry для каждого датчика
    for (int i = 0; i < max_retries; i++) {
        // SHT3x (температура + влажность)
        if (ret_temp != ESP_OK) {
            ret_temp = sht3x_read(temp, humidity);
        }

        // CCS811 (CO2)
        if (ret_co2 != ESP_OK) {
            ret_co2 = ccs811_read(co2, NULL);
            
            // Компенсация температуры/влажности для CCS811
            if (ret_temp == ESP_OK) {
                ccs811_set_environment(*temp, *humidity);
            }
        }

        // Lux sensor
        if (ret_lux != ESP_OK) {
            ret_lux = lux_sensor_read(lux);
        }

        // Все датчики успешно?
        if (ret_temp == ESP_OK && ret_co2 == ESP_OK && ret_lux == ESP_OK) {
            break;
        }

        // Ожидание перед повтором
        if (i < max_retries - 1) {
            ESP_LOGW(TAG, "Sensor read failed, retry %d/%d", i + 1, max_retries);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    // Установка моковых значений для неудачных чтений (MOCK MODE)
    if (ret_temp != ESP_OK) {
        ESP_LOGW(TAG, "⚠️ SHT3x read failed - using MOCK values");
        *temp = 22.5f;     // Моковая температура (вместо -127.0f)
        *humidity = 65.0f; // Моковая влажность (вместо 0.0f)
    }
    if (ret_co2 != ESP_OK) {
        ESP_LOGW(TAG, "⚠️ CCS811 read failed - using MOCK value");
        *co2 = 800;  // Моковое CO2 (вместо 0)
    }
    if (ret_lux != ESP_OK) {
        ESP_LOGW(TAG, "⚠️ Lux sensor read failed - using MOCK value");
        *lux = 500;  // Моковое освещение (вместо 0)
    }

    // ⚠️ MOCK MODE: Всегда возвращаем ESP_OK (даже если все датчики не работают)
    // Телеметрия будет отправлена с моковыми значениями
    bool at_least_one_ok = (ret_temp == ESP_OK || ret_co2 == ESP_OK || ret_lux == ESP_OK);
    
    if (!at_least_one_ok) {
        ESP_LOGW(TAG, "⚠️ All sensors failed - using MOCK data (temp=22.5°C, hum=65%, co2=800ppm, lux=500)");
    }
    
    ESP_LOGD(TAG, "📊 Sensors: %.1f°C, %.0f%%, %dppm, %dlux", *temp, *humidity, *co2, *lux);
    return ESP_OK;  // Всегда успех для отправки телеметрии с моковыми данными
}

// Получение RSSI к родительскому узлу (ROOT)
static int8_t get_rssi_to_parent(void) {
    return mesh_manager_get_parent_rssi();
}

// Отправка discovery сообщения
static void send_discovery(void) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, discovery skipped");
        return;
    }

    // Получение MAC адреса
    uint8_t mac[6];
    mesh_manager_get_mac(mac);

    // Получение данных о системе
    uint32_t heap_free = esp_get_free_heap_size();
    int8_t rssi = get_rssi_to_parent();

    // Создание discovery JSON
    char discovery_msg[512];
    snprintf(discovery_msg, sizeof(discovery_msg),
            "{\"type\":\"discovery\","
            "\"node_id\":\"%s\","
            "\"node_type\":\"climate\","
            "\"root_node_id\":\"%s\","
            "\"mesh_network_id\":\"%s\","
            "\"zone\":\"%s\","
            "\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
            "\"firmware\":\"1.0.0\","
            "\"hardware\":\"ESP32\","
            "\"sensors\":[\"sht3x\",\"ccs811\",\"lux\"],"
            "\"heap_free\":%lu,"
            "\"wifi_rssi\":%d}",
            s_config->base.node_id,
            s_root_node_id,
            s_mesh_network_id,
            s_config->base.zone,
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
            (unsigned long)heap_free,
            rssi);

    esp_err_t err = mesh_manager_send_to_root((uint8_t *)discovery_msg, strlen(discovery_msg));
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "🔍 Discovery sent: %s (RSSI=%d)", s_config->base.node_id, rssi);
    } else {
        ESP_LOGW(TAG, "Failed to send discovery: %s", esp_err_to_name(err));
    }
}

// Отправка heartbeat
static void send_heartbeat(void) {
    if (!mesh_manager_is_connected()) {
        return;
    }

    // Получение MAC адреса
    uint8_t mac[6];
    mesh_manager_get_mac(mac);

    uint32_t uptime = (uint32_t)time(NULL) - s_boot_time;
    uint32_t heap_free = esp_get_free_heap_size();
    int8_t rssi = get_rssi_to_parent();

    // Создание heartbeat JSON с node_type, MAC и RSSI
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "heartbeat");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "climate");
    cJSON_AddStringToObject(root, "root_node_id", s_root_node_id);
    cJSON_AddStringToObject(root, "mesh_network_id", s_mesh_network_id);
    
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    
    cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
    cJSON_AddNumberToObject(root, "uptime", uptime);
    cJSON_AddNumberToObject(root, "heap_free", heap_free);
    cJSON_AddNumberToObject(root, "rssi_to_parent", rssi);
    
    char *heartbeat_msg = cJSON_PrintUnformatted(root);
    esp_err_t err = ESP_FAIL;
    if (heartbeat_msg) {
        err = mesh_manager_send_to_root((uint8_t *)heartbeat_msg, strlen(heartbeat_msg));
    }
    cJSON_Delete(root);
    
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "💓 Heartbeat sent (uptime=%lus, heap=%luB, RSSI=%d)", 
                 (unsigned long)uptime, (unsigned long)heap_free, rssi);
        climate_display_show_state(true);
        if (s_oled_ready) {
            oled_display_show_heartbeat(true);
        }
    }
    
    if (heartbeat_msg) {
        free(heartbeat_msg);
    }
}

// Отправка телеметрии на ROOT с RSSI
static void send_telemetry(float temp, float humidity, uint16_t co2, uint16_t lux) {
    bool mesh_connected = mesh_manager_is_connected();
    int8_t rssi = mesh_connected ? get_rssi_to_parent() : 0;

    climate_display_update(temp, humidity, co2, lux, mesh_connected, rssi);

    if (!mesh_connected) {
        ESP_LOGW(TAG, "Mesh offline, telemetry skipped");
        return;
    }

    // Создание JSON
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "temperature", temp);
    cJSON_AddNumberToObject(data, "humidity", humidity);
    cJSON_AddNumberToObject(data, "co2", co2);
    cJSON_AddNumberToObject(data, "lux", lux);
    cJSON_AddNumberToObject(data, "rssi_to_parent", rssi);

    char json_buf[512];
    if (mesh_protocol_create_telemetry(s_config->base.node_id, s_root_node_id, s_mesh_network_id, "climate", data,
                                        json_buf, sizeof(json_buf))) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "📊 Telemetry: %.1f°C, %.0f%%, %dppm, %dlux, RSSI=%d", 
                     temp, humidity, co2, lux, rssi);
        } else {
            ESP_LOGW(TAG, "Failed to send telemetry: %s", esp_err_to_name(err));
        }
    }

    cJSON_Delete(data);
}

void climate_controller_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "Command received: %s", command);

    if (strcmp(command, "set_read_interval") == 0) {
        cJSON *interval = cJSON_GetObjectItem(params, "interval_ms");
        if (interval && cJSON_IsNumber(interval)) {
            s_config->read_interval_ms = (uint32_t)interval->valueint;
            node_config_save(s_config, sizeof(climate_node_config_t), "climate_ns");
            ESP_LOGI(TAG, "Read interval updated: %d ms", s_config->read_interval_ms);
        }
    }
    // Другие команды по необходимости
}

void climate_controller_handle_config_update(cJSON *config_json) {
    ESP_LOGI(TAG, "Config update received");

    // Обновление конфигурации
    node_config_update_from_json(s_config, config_json, "climate");
    
    // Сохранение в NVS
    node_config_save(s_config, sizeof(climate_node_config_t), "climate_ns");
    
    ESP_LOGI(TAG, "Config updated and saved");
}

// Отправка event сообщения
static void send_event(mesh_event_level_t level, const char *message, float temp, float humidity, uint16_t co2) {
    if (!mesh_manager_is_connected()) {
        return;
    }
    
    cJSON *data = cJSON_CreateObject();
    if (temp > -100.0f) {
        cJSON_AddNumberToObject(data, "temperature", temp);
    }
    if (humidity >= 0.0f) {
        cJSON_AddNumberToObject(data, "humidity", humidity);
    }
    if (co2 > 0) {
        cJSON_AddNumberToObject(data, "co2", co2);
    }
    
    char json_buf[512];
    if (mesh_protocol_create_event(s_config->base.node_id, s_root_node_id, s_mesh_network_id, level, message, data,
                                    json_buf, sizeof(json_buf))) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Event sent: %s - %s", mesh_protocol_event_level_to_str(level), message);
        }
    }
    
    cJSON_Delete(data);
}

// Проверка пороговых значений
static void check_sensor_thresholds(float temp, float humidity, uint16_t co2) {
    static bool temp_warning_sent = false;
    static bool co2_warning_sent = false;
    static bool humidity_warning_sent = false;
    
    // Критичная температура
    if (temp > -100.0f) {  // Валидное значение
        if (temp < 5.0f || temp > 45.0f) {
            if (!temp_warning_sent) {
                send_event(MESH_EVENT_CRITICAL, "Temperature critical", temp, humidity, co2);
                temp_warning_sent = true;
            }
        } else if (temp < 10.0f || temp > 40.0f) {
            if (!temp_warning_sent) {
                send_event(MESH_EVENT_WARNING, "Temperature out of optimal range", temp, humidity, co2);
                temp_warning_sent = true;
            }
        } else {
            temp_warning_sent = false;  // Сброс флага при нормализации
        }
    }
    
    // Критичный CO2
    if (co2 > 0) {
        if (co2 > 3000) {
            if (!co2_warning_sent) {
                send_event(MESH_EVENT_CRITICAL, "CO2 critical level", temp, humidity, co2);
                co2_warning_sent = true;
            }
        } else if (co2 > 2000) {
            if (!co2_warning_sent) {
                send_event(MESH_EVENT_WARNING, "CO2 high level", temp, humidity, co2);
                co2_warning_sent = true;
            }
        } else {
            co2_warning_sent = false;
        }
    }
    
    // Критичная влажность
    if (humidity >= 0.0f) {
        if (humidity < 20.0f || humidity > 90.0f) {
            if (!humidity_warning_sent) {
                send_event(MESH_EVENT_WARNING, "Humidity out of range", temp, humidity, co2);
                humidity_warning_sent = true;
            }
        } else {
            humidity_warning_sent = false;
        }
    }
}

static void climate_display_init_once(void) {
    if (s_oled_ready) {
        return;
    }

    oled_display_config_t cfg = {
        .i2c_port = CLIMATE_OLED_I2C_PORT,
        .sda_pin = CLIMATE_OLED_SDA_PIN,
        .scl_pin = CLIMATE_OLED_SCL_PIN,
        .clk_speed_hz = 400000,
        .i2c_address = 0x3C,
        .width = 128,
        .height = 64,
        .line_count = 4,
    };

    if (oled_display_init(&cfg) != ESP_OK) {
        ESP_LOGW(TAG, "OLED init failed");
        return;
    }

    oled_display_task_config_t task_cfg = {
        .stack_size = 4096,
        .priority = 4,
        .queue_depth = 6,
        .heartbeat_timeout_ticks = pdMS_TO_TICKS(1500),
    };

    if (oled_display_start_task(&task_cfg) != ESP_OK) {
        ESP_LOGW(TAG, "OLED task start failed");
        return;
    }

    oled_display_set_template(0, "{node} {zone}");
    oled_display_set_template(1, "T {temp}  H {hum}");
    oled_display_set_template(2, "CO2 {co2}  Lux {lux}");
    oled_display_set_template(3, "Mesh {mesh} RSSI {rssi}");

    oled_display_kv_t init_values[] = {
        {.key = "node", .value = s_config ? s_config->base.node_id : "climate"},
        {.key = "zone", .value = (s_config && s_config->base.zone[0]) ? s_config->base.zone : "Zone"},
        {.key = "temp", .value = "--"},
        {.key = "hum", .value = "--"},
        {.key = "co2", .value = "--"},
        {.key = "lux", .value = "--"},
        {.key = "mesh", .value = "INIT"},
        {.key = "rssi", .value = "--"},
    };
    oled_display_queue_render(init_values, sizeof(init_values) / sizeof(init_values[0]), 0);

    s_oled_ready = true;
}

static void climate_display_update(float temp, float humidity, uint16_t co2, uint16_t lux, bool mesh_connected, int8_t rssi) {
    if (!s_oled_ready) {
        return;
    }

    char temp_str[16];
    char hum_str[16];
    char co2_str[16];
    char lux_str[16];
    char mesh_str[16];
    char rssi_str[16];

    snprintf(temp_str, sizeof(temp_str), "%.1fC", temp);
    snprintf(hum_str, sizeof(hum_str), "%.0f%%", humidity);
    snprintf(co2_str, sizeof(co2_str), "%uppm", (unsigned)co2);
    snprintf(lux_str, sizeof(lux_str), "%ulx", (unsigned)lux);
    strlcpy(mesh_str, mesh_connected ? "ONLINE" : "OFFLINE", sizeof(mesh_str));

    if (mesh_connected && rssi != 0) {
        snprintf(rssi_str, sizeof(rssi_str), "%ddBm", rssi);
    } else {
        strlcpy(rssi_str, "--", sizeof(rssi_str));
    }

    const char *zone = (s_config && s_config->base.zone[0]) ? s_config->base.zone : "Zone";
    const char *node_id = s_config ? s_config->base.node_id : "climate";

    oled_display_kv_t values[] = {
        {.key = "node", .value = node_id},
        {.key = "zone", .value = zone},
        {.key = "temp", .value = temp_str},
        {.key = "hum", .value = hum_str},
        {.key = "co2", .value = co2_str},
        {.key = "lux", .value = lux_str},
        {.key = "mesh", .value = mesh_str},
        {.key = "rssi", .value = rssi_str},
    };

    oled_display_queue_render(values, sizeof(values) / sizeof(values[0]), 0);

    s_last_temp = temp;
    s_last_humidity = humidity;
    s_last_co2 = co2;
    s_last_lux = lux;
    s_last_rssi = rssi;
    s_last_mesh_connected = mesh_connected;
    s_display_has_data = true;
}

static void climate_display_show_state(bool mesh_connected) {
    if (!s_oled_ready) {
        return;
    }

    if (!s_display_has_data) {
        climate_display_update(0.0f, 0.0f, 0, 0, mesh_connected, 0);
        return;
    }

    climate_display_update(s_last_temp, s_last_humidity, s_last_co2, s_last_lux, mesh_connected, s_last_rssi);
}

