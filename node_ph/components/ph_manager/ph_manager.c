/**
 * @file ph_manager.c
 * @brief pH Manager implementation
 */

#include "ph_manager.h"
#include "ph_sensor.h"
#include "pump_controller.h"
#include "adaptive_pid.h"
#include "pump_events.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "mesh_config.h"  // Для HEARTBEAT_INTERVAL_MS
#include "zone_config.h"
#include "../../../common/oled_display/oled_display.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include "cJSON.h"
#include <string.h>
#include <time.h>

static const char *TAG = "ph_mgr";

// Состояние менеджера
static ph_node_config_t *s_config = NULL;
static TaskHandle_t s_main_task = NULL;
static TaskHandle_t s_heartbeat_task = NULL;
static bool s_discovery_sent = false;
static uint32_t s_boot_time = 0;
static bool s_emergency_mode = false;
static bool s_autonomous_mode = false;
static char s_root_node_id[ZONE_CONFIG_MAX_LEN] = {0};
static char s_mesh_network_id[ZONE_CONFIG_MAX_LEN] = {0};
static bool s_display_ready = false;
static float s_display_last_ph = 0.0f;
static float s_display_last_target = 0.0f;
static uint32_t s_display_pump_up_ml = 0;
static uint32_t s_display_pump_down_ml = 0;
static bool s_display_last_mesh = false;
static int8_t s_display_last_rssi = 0;
static bool s_display_has_data = false;

// Адаптивные PID контроллеры
static adaptive_pid_t s_pid_ph_up;
static adaptive_pid_t s_pid_ph_down;

// Текущее значение
static float s_current_ph = 7.0f;

// Forward declarations
static void main_task(void *arg);
static void heartbeat_task(void *arg);
static void send_discovery(void);
static void send_telemetry(void);
static void send_heartbeat(void);
static void send_event(mesh_event_level_t level, const char *message, float value);
static void send_event_with_metadata(mesh_event_level_t level, const char *message, float ph, float output, float error, pid_zone_t zone, pump_id_t pump_id);
static int8_t get_rssi_to_parent(void);
static void read_sensor(void);
static void control_ph(void);
static void check_emergency_conditions(void);
static void ph_display_init_once(void);
static void ph_display_update(float ph_value, bool mesh_connected, int8_t rssi);
static void ph_display_show_state(bool mesh_connected);
static void ph_display_show_heartbeat(void);

#define PH_OLED_I2C_PORT   I2C_NUM_0
#define PH_OLED_SDA_PIN    GPIO_NUM_8
#define PH_OLED_SCL_PIN    GPIO_NUM_9
#define PH_OLED_I2C_ADDR   0x3C

esp_err_t ph_manager_init(ph_node_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    s_config = config;
    s_boot_time = (uint32_t)time(NULL);
    s_discovery_sent = false;
    s_emergency_mode = false;
    s_autonomous_mode = false;

    zone_config_init();
    const char *mesh_id = zone_config_get_mesh_id();
    const char *root_id = zone_config_get_root_id();

    if (mesh_id && mesh_id[0] != '\0' && zone_config_validate(mesh_id)) {
        strncpy(s_mesh_network_id, mesh_id, sizeof(s_mesh_network_id) - 1);
    } else if (config->base.zone[0] != '\0') {
        strncpy(s_mesh_network_id, config->base.zone, sizeof(s_mesh_network_id) - 1);
    } else {
        strncpy(s_mesh_network_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_mesh_network_id) - 1);
    }
    s_mesh_network_id[sizeof(s_mesh_network_id) - 1] = '\0';

    if (root_id && root_id[0] != '\0' && zone_config_validate(root_id)) {
        strncpy(s_root_node_id, root_id, sizeof(s_root_node_id) - 1);
    } else if (config->base.root_node_id[0] != '\0') {
        strncpy(s_root_node_id, config->base.root_node_id, sizeof(s_root_node_id) - 1);
    } else {
        strncpy(s_root_node_id, ZONE_CONFIG_UNCONFIGURED, sizeof(s_root_node_id) - 1);
    }
    s_root_node_id[sizeof(s_root_node_id) - 1] = '\0';
    
    // Инициализация адаптивных PID контроллеров
    ESP_LOGI(TAG, "[INIT] Initializing pH UP PID: target=%.2f, Kp=%.2f, Ki=%.2f, Kd=%.2f",
             s_config->ph_target,
             s_config->pump_pid[PUMP_PH_UP].kp,
             s_config->pump_pid[PUMP_PH_UP].ki,
             s_config->pump_pid[PUMP_PH_UP].kd);
    adaptive_pid_init(&s_pid_ph_up, s_config->ph_target,
                     s_config->pump_pid[PUMP_PH_UP].kp, 
                     s_config->pump_pid[PUMP_PH_UP].ki, 
                     s_config->pump_pid[PUMP_PH_UP].kd);
    
    // Настройка зон для pH (dead=0.1, close=0.3, far=1.0)
    adaptive_pid_set_zones(&s_pid_ph_up, 0.1f, 0.3f, 1.0f);
    
    // Safety: макс 5 мл за раз, минимум 5 сек между дозами (из конфигурации)
    uint32_t min_interval = s_config->cooldown_ms > 0 ? s_config->cooldown_ms : 5000;
    adaptive_pid_set_safety(&s_pid_ph_up, 5.0f, min_interval);
    
    // Лимиты выхода
    adaptive_pid_set_output_limits(&s_pid_ph_up, 0.0f, 5.0f);
    ESP_LOGI(TAG, "[INIT] pH UP PID: zones=(0.1, 0.3, 1.0), safety=(5ml, %lums), output_limits=(0-5ml)", min_interval);
    
    // pH DOWN контроллер
    ESP_LOGI(TAG, "[INIT] Initializing pH DOWN PID: target=%.2f, Kp=%.2f, Ki=%.2f, Kd=%.2f",
             s_config->ph_target,
             s_config->pump_pid[PUMP_PH_DOWN].kp,
             s_config->pump_pid[PUMP_PH_DOWN].ki,
             s_config->pump_pid[PUMP_PH_DOWN].kd);
    adaptive_pid_init(&s_pid_ph_down, s_config->ph_target,
                     s_config->pump_pid[PUMP_PH_DOWN].kp,
                     s_config->pump_pid[PUMP_PH_DOWN].ki,
                     s_config->pump_pid[PUMP_PH_DOWN].kd);
    
    adaptive_pid_set_zones(&s_pid_ph_down, 0.1f, 0.3f, 1.0f);
    adaptive_pid_set_safety(&s_pid_ph_down, 5.0f, min_interval);
    adaptive_pid_set_output_limits(&s_pid_ph_down, 0.0f, 5.0f);
    ESP_LOGI(TAG, "[INIT] pH DOWN PID: zones=(0.1, 0.3, 1.0), safety=(5ml, %lums), output_limits=(0-5ml)", min_interval);
    
    ESP_LOGI(TAG, "pH Manager initialized");
    ESP_LOGI(TAG, "Node ID: %s, pH target: %.2f", 
             s_config->base.node_id, s_config->ph_target);
    ESP_LOGI(TAG, "Zone context: mesh_id=%s, root_id=%s",
             s_mesh_network_id, s_root_node_id);

    ESP_LOGI(TAG, "🔵 [PH_OLED] Calling ph_display_init_once() from ph_manager_init()...");
    ph_display_init_once();
    ESP_LOGI(TAG, "🔵 [PH_OLED] ph_display_init_once() returned, s_display_ready = %d", s_display_ready);
    
    ESP_LOGI(TAG, "🔵 [PH_OLED] Calling ph_display_update() from ph_manager_init()...");
    ph_display_update(s_current_ph, mesh_manager_is_connected(), mesh_manager_is_connected() ? get_rssi_to_parent() : 0);
    ESP_LOGI(TAG, "🔵 [PH_OLED] ph_display_update() returned");
    
    return ESP_OK;
}

esp_err_t ph_manager_start(void) {
    if (s_main_task != NULL) {
        ESP_LOGW(TAG, "Already running");
        return ESP_OK;
    }
    
    // Запуск главной задачи
    BaseType_t ret = xTaskCreate(main_task, "ph_main", 6144, NULL, 5, &s_main_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main task");
        return ESP_FAIL;
    }
    
    // Запуск heartbeat задачи
    ret = xTaskCreate(heartbeat_task, "heartbeat", 3072, NULL, 4, &s_heartbeat_task);
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Failed to create heartbeat task");
    }
    
    ESP_LOGI(TAG, "pH Manager started");
    return ESP_OK;
}

esp_err_t ph_manager_stop(void) {
    if (s_main_task != NULL) {
        vTaskDelete(s_main_task);
        s_main_task = NULL;
    }
    if (s_heartbeat_task != NULL) {
        vTaskDelete(s_heartbeat_task);
        s_heartbeat_task = NULL;
    }
    
    // Остановка всех насосов
    pump_controller_emergency_stop();
    
    if (s_display_ready) {
        oled_display_shutdown();
        s_display_ready = false;
        s_display_last_ph = 0.0f;
        s_display_last_target = 0.0f;
        s_display_pump_up_ml = 0;
        s_display_pump_down_ml = 0;
        s_display_last_mesh = false;
        s_display_last_rssi = 0;
        s_display_has_data = false;
    }

    ESP_LOGI(TAG, "pH Manager stopped");
    return ESP_OK;
}

esp_err_t ph_manager_get_value(float *ph) {
    if (ph) *ph = s_current_ph;
    return ESP_OK;
}

esp_err_t ph_manager_set_emergency(bool enable) {
    s_emergency_mode = enable;
    
    if (enable) {
        ESP_LOGW(TAG, "EMERGENCY MODE ACTIVATED");
        pump_controller_emergency_stop();
    } else {
        ESP_LOGI(TAG, "Emergency mode deactivated");
    }
    
    ph_display_show_state(mesh_manager_is_connected());

    return ESP_OK;
}

// Главная задача управления
static void main_task(void *arg) {
    ESP_LOGI(TAG, "Main task started");
    
    // Ждём подключения к mesh
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Отправка discovery при старте
    send_discovery();
    s_discovery_sent = true;
    
    TickType_t last_telemetry = 0;
    TickType_t last_control = 0;
    
    while (1) {
        TickType_t now = xTaskGetTickCount();
        
        // Чтение датчика каждые 10 секунд
        if (now - last_control >= pdMS_TO_TICKS(10000)) {
            read_sensor();
            check_emergency_conditions();
            
            if (!s_emergency_mode) {
                control_ph();
            }
            
            last_control = now;
        }
        
        // Отправка telemetry каждые 30 секунд
        if (now - last_telemetry >= pdMS_TO_TICKS(30000)) {
            send_telemetry();
            last_telemetry = now;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Heartbeat задача
static void heartbeat_task(void *arg) {
    ESP_LOGI(TAG, "Heartbeat task started (interval: %d ms)", HEARTBEAT_INTERVAL_MS);
    
    // Начальная задержка перед первым heartbeat
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    while (1) {
        if (mesh_manager_is_connected()) {
            send_heartbeat();
        }
        
        // Используем единый интервал из mesh_config.h
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS));
    }
}

// Отправка discovery сообщения
static void send_discovery(void) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Cannot send discovery - mesh not connected");
        return;
    }
    
    // Создание JSON
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON");
        return;
    }
    
    cJSON_AddStringToObject(root, "type", "discovery");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "ph");
    const char *mesh_id = zone_config_validate(s_mesh_network_id) ? s_mesh_network_id : ZONE_CONFIG_UNCONFIGURED;
    const char *root_id = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_id);
    cJSON_AddStringToObject(root, "root_node_id", root_id);
    
    // Sensors
    cJSON *sensors = cJSON_CreateArray();
    if (sensors) {
        cJSON_AddItemToArray(sensors, cJSON_CreateString("ph"));
        cJSON_AddItemToObject(root, "sensors", sensors);
    }
    
    // Actuators
    cJSON *actuators = cJSON_CreateArray();
    if (actuators) {
        cJSON_AddItemToArray(actuators, cJSON_CreateString("pump_ph_up"));
        cJSON_AddItemToArray(actuators, cJSON_CreateString("pump_ph_down"));
        cJSON_AddItemToObject(root, "actuators", actuators);
    }
    
    // System info
    cJSON_AddNumberToObject(root, "heap_free", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "wifi_rssi", get_rssi_to_parent());
    
    // Получение MAC адреса
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(root, "mac_address", mac_str);  // Изменено с "mac" на "mac_address"
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        ESP_LOGI(TAG, "Discovery sent: %s", json_str);
        free(json_str);
    }
    
    cJSON_Delete(root);
}

// Отправка telemetry
static void send_telemetry(void) {
    bool mesh_connected = mesh_manager_is_connected();
    int8_t rssi = 0;

    if (!mesh_connected) {
        s_autonomous_mode = true;
        ph_display_update(s_current_ph, false, 0);
        return;
    }

    s_autonomous_mode = false;
    rssi = get_rssi_to_parent();
    ph_display_update(s_current_ph, true, rssi);
    
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }
    
    // Получение MAC адреса
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    cJSON_AddStringToObject(root, "type", "telemetry");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "ph");  // ВАЖНО: тип узла для backend
    cJSON_AddStringToObject(root, "mac_address", mac_str);  // Добавляем MAC адрес
    const char *mesh_id = zone_config_validate(s_mesh_network_id) ? s_mesh_network_id : ZONE_CONFIG_UNCONFIGURED;
    const char *root_id = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_id);
    cJSON_AddStringToObject(root, "root_node_id", root_id);
    
    cJSON *data = cJSON_CreateObject();
    if (!data) {
        ESP_LOGE(TAG, "Failed to create data JSON object");
        cJSON_Delete(root);
        return;
    }
    
    cJSON_AddNumberToObject(data, "ph", s_current_ph);
    cJSON_AddNumberToObject(data, "ph_target", s_config->ph_target);
    cJSON_AddNumberToObject(data, "pump_ph_up_ml", pump_controller_get_total_ml(PUMP_PH_UP));
    cJSON_AddNumberToObject(data, "pump_ph_down_ml", pump_controller_get_total_ml(PUMP_PH_DOWN));
    cJSON_AddNumberToObject(data, "rssi_to_parent", rssi);
    cJSON_AddBoolToObject(data, "emergency", s_emergency_mode);
    cJSON_AddBoolToObject(data, "autonomous", s_autonomous_mode);
    cJSON_AddItemToObject(root, "data", data);
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        ESP_LOGD(TAG, "Telemetry sent");
        free(json_str);
    }
    
    cJSON_Delete(root);
}

// Отправка heartbeat
static void send_heartbeat(void) {
    if (!mesh_manager_is_connected()) {
        ph_display_show_state(false);
        return;
    }
    
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }
    
    // Получение MAC адреса
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    cJSON_AddStringToObject(root, "type", "heartbeat");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "ph");  // ВАЖНО: тип узла для backend
    cJSON_AddStringToObject(root, "mac_address", mac_str);  // Добавляем MAC адрес
    cJSON_AddNumberToObject(root, "uptime", (uint32_t)time(NULL) - s_boot_time);
    cJSON_AddNumberToObject(root, "heap_free", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "rssi_to_parent", get_rssi_to_parent());
    cJSON_AddBoolToObject(root, "autonomous", s_autonomous_mode);
    const char *mesh_id = zone_config_validate(s_mesh_network_id) ? s_mesh_network_id : ZONE_CONFIG_UNCONFIGURED;
    const char *root_id = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_id);
    cJSON_AddStringToObject(root, "root_node_id", root_id);
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        if (err == ESP_OK) {
            ph_display_show_state(true);
            ph_display_show_heartbeat();
        }
        free(json_str);
    }
    
    cJSON_Delete(root);
}

// Чтение датчика
static void read_sensor(void) {
    float ph;
    esp_err_t ret = ph_sensor_read(&ph);
    
    if (ret == ESP_OK) {
        // Дополнительная валидация в менеджере
        if (ph < 0.0f || ph > 14.0f) {
            ESP_LOGE(TAG, "CRITICAL: Invalid pH from sensor: %.2f - using last value", ph);
            ESP_LOGW(TAG, "Current pH: %.2f (last valid)", s_current_ph);
        } else {
            s_current_ph = ph + s_config->ph_cal_offset;
            ESP_LOGI(TAG, "pH: %.2f (target: %.2f, mock=%s)", 
                     s_current_ph, s_config->ph_target, 
                     ph_sensor_is_mock_mode() ? "YES" : "NO");
        }
    } else {
        ESP_LOGW(TAG, "Failed to read pH sensor, using last value: %.2f", s_current_ph);
    }
}

// Управление pH
static void control_ph(void) {
    if (s_config == NULL) {
        ESP_LOGW(TAG, "[CONTROL] Config not initialized, skipping");
        return;
    }
    
    float output = 0.0f;
    esp_err_t err;
    int8_t rssi = get_rssi_to_parent();
    float error = s_current_ph - s_config->ph_target;
    
    ESP_LOGI(TAG, "[CONTROL] pH=%.2f, target=%.2f, error=%.2f, emergency=%d", 
             s_current_ph, s_config->ph_target, error, s_emergency_mode);
    
    // Если pH < target - нужно повысить (pH UP)
    if (s_current_ph < s_config->ph_target) {
        ESP_LOGI(TAG, "[CONTROL] pH < target, computing pH UP correction");
        err = adaptive_pid_compute(&s_pid_ph_up, s_current_ph, 10.0f, &output);
        ESP_LOGI(TAG, "[CONTROL] pH UP PID result: err=%d, output=%.3f", err, output);
        
        if (err == ESP_OK && output > 0.0f) {
            pid_zone_t zone = adaptive_pid_get_zone(&s_pid_ph_up);
            ESP_LOGI(TAG, "pH UP: %.2f ml [%s zone] (current=%.2f, target=%.2f)", 
                     output, adaptive_pid_zone_to_str(zone), s_current_ph, s_config->ph_target);
            
            // Получение данных PID для события
            pump_event_pid_data_t pid_data;
            // Заполняем данные PID вручную из adaptive_pid_t
            pid_data.kp = (zone == ZONE_DEAD) ? s_pid_ph_up.coeffs_dead.kp : 
                          (zone == ZONE_CLOSE) ? s_pid_ph_up.coeffs_close.kp : 
                          s_pid_ph_up.coeffs_far.kp;
            pid_data.ki = (zone == ZONE_DEAD) ? s_pid_ph_up.coeffs_dead.ki : 
                          (zone == ZONE_CLOSE) ? s_pid_ph_up.coeffs_close.ki : 
                          s_pid_ph_up.coeffs_far.ki;
            pid_data.kd = (zone == ZONE_DEAD) ? s_pid_ph_up.coeffs_dead.kd : 
                          (zone == ZONE_CLOSE) ? s_pid_ph_up.coeffs_close.kd : 
                          s_pid_ph_up.coeffs_far.kd;
            pid_data.setpoint = s_pid_ph_up.setpoint;
            pid_data.current_value = s_current_ph;
            pid_data.error = s_pid_ph_up.setpoint - s_current_ph;
            pid_data.output = output;
            pid_data.integral = s_pid_ph_up.integral;
            pid_data.derivative = s_pid_ph_up.prev_error;
            pid_data.enabled = true;
            
            // Отправка события включения насоса pH UP
            pump_events_send_start_event(
                PUMP_PH_UP,
                (uint32_t)((output / 2.0f) * 1000.0f), // Примерная длительность
                output,
                &pid_data,
                s_current_ph,
                s_config->ph_target,
                s_emergency_mode, s_autonomous_mode,
                rssi
            );
            
            pump_controller_run_dose(PUMP_PH_UP, output);
            
            // Обновление mock значения pH при дозировании
            ph_sensor_update_mock_for_dosing(PUMP_PH_UP, output);
            
            // Отправка события при коррекции в FAR зоне
            if (zone == ZONE_FAR) {
                float error = s_config->ph_target - s_current_ph;
                send_event_with_metadata(MESH_EVENT_WARNING, "pH far from target, aggressive correction", 
                                        s_current_ph, output, error, zone, PUMP_PH_UP);
            }
        } else {
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "[CONTROL] pH UP PID error: %s", esp_err_to_name(err));
            } else if (output <= 0.0f) {
                ESP_LOGI(TAG, "[CONTROL] pH UP output <= 0 (%.3f), no correction needed", output);
            }
        }
    }
    // Если pH > target - нужно понизить (pH DOWN)
    // ВАЖНО: для pH DOWN ошибка будет отрицательной (setpoint - current < 0)
    // adaptive_pid вычисляет error = setpoint - current = 6.50 - 6.86 = -0.36
    // Это даст отрицательный output, который обрежется до output_min = 0
    // Решение: инвертируем setpoint и current местами для получения положительной ошибки
    else if (s_current_ph > s_config->ph_target) {
        ESP_LOGI(TAG, "[CONTROL] pH > target, computing pH DOWN correction");
        // Временно изменяем setpoint на current для получения положительной ошибки
        // error = new_setpoint - new_current = current - target = 6.86 - 6.50 = +0.36
        float orig_setpoint = s_pid_ph_down.setpoint;
        s_pid_ph_down.setpoint = s_current_ph;  // Временно устанавливаем setpoint = current
        err = adaptive_pid_compute(&s_pid_ph_down, s_config->ph_target, 10.0f, &output);
        s_pid_ph_down.setpoint = orig_setpoint;  // Восстанавливаем исходный setpoint
        ESP_LOGI(TAG, "[CONTROL] pH DOWN PID result: err=%d, output=%.3f", err, output);
        
        if (err == ESP_OK && output > 0.0f) {
            pid_zone_t zone = adaptive_pid_get_zone(&s_pid_ph_down);
            ESP_LOGI(TAG, "pH DOWN: %.2f ml [%s zone] (current=%.2f, target=%.2f)", 
                     output, adaptive_pid_zone_to_str(zone), s_current_ph, s_config->ph_target);
            
            // Получение данных PID для события
            pump_event_pid_data_t pid_data;
            // Заполняем данные PID вручную из adaptive_pid_t
            pid_data.kp = (zone == ZONE_DEAD) ? s_pid_ph_down.coeffs_dead.kp : 
                          (zone == ZONE_CLOSE) ? s_pid_ph_down.coeffs_close.kp : 
                          s_pid_ph_down.coeffs_far.kp;
            pid_data.ki = (zone == ZONE_DEAD) ? s_pid_ph_down.coeffs_dead.ki : 
                          (zone == ZONE_CLOSE) ? s_pid_ph_down.coeffs_close.ki : 
                          s_pid_ph_down.coeffs_far.ki;
            pid_data.kd = (zone == ZONE_DEAD) ? s_pid_ph_down.coeffs_dead.kd : 
                          (zone == ZONE_CLOSE) ? s_pid_ph_down.coeffs_close.kd : 
                          s_pid_ph_down.coeffs_far.kd;
            pid_data.setpoint = s_pid_ph_down.setpoint;
            pid_data.current_value = s_current_ph;
            pid_data.error = s_pid_ph_down.setpoint - s_current_ph;
            pid_data.output = output;
            pid_data.integral = s_pid_ph_down.integral;
            pid_data.derivative = s_pid_ph_down.prev_error;
            pid_data.enabled = true;
            
            // Отправка события включения насоса pH DOWN
            pump_events_send_start_event(
                PUMP_PH_DOWN,
                (uint32_t)((output / 2.0f) * 1000.0f), // Примерная длительность
                output,
                &pid_data,
                s_current_ph,
                s_config->ph_target,
                s_emergency_mode, s_autonomous_mode,
                rssi
            );
            
            pump_controller_run_dose(PUMP_PH_DOWN, output);
            
            // Обновление mock значения pH при дозировании
            ph_sensor_update_mock_for_dosing(PUMP_PH_DOWN, output);
            
            // Отправка события при коррекции в FAR зоне
            if (zone == ZONE_FAR) {
                float error = s_current_ph - s_config->ph_target;
                send_event_with_metadata(MESH_EVENT_WARNING, "pH far from target, aggressive correction", 
                                        s_current_ph, output, error, zone, PUMP_PH_DOWN);
            }
        } else {
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "[CONTROL] pH DOWN PID error: %s", esp_err_to_name(err));
            } else if (output <= 0.0f) {
                ESP_LOGI(TAG, "[CONTROL] pH DOWN output <= 0 (%.3f), no correction needed", output);
            }
        }
    } else {
        ESP_LOGI(TAG, "[CONTROL] pH in range (%.2f = %.2f), no correction needed", s_current_ph, s_config->ph_target);
    }

    bool mesh_connected = mesh_manager_is_connected();
    ph_display_update(s_current_ph, mesh_connected, mesh_connected ? get_rssi_to_parent() : 0);
}

static void ph_display_init_once(void)
{
    ESP_LOGI(TAG, "🔵 [PH_OLED] ph_display_init_once() called");
    ESP_LOGI(TAG, "   s_display_ready = %d", s_display_ready);
    
    if (s_display_ready) {
        ESP_LOGI(TAG, "   OLED already ready in ph_manager, skipping");
        return;
    }

    ESP_LOGI(TAG, "   Configuring OLED: I2C port=%d, SDA=%d, SCL=%d, addr=0x%02X",
             PH_OLED_I2C_PORT, PH_OLED_SDA_PIN, PH_OLED_SCL_PIN, PH_OLED_I2C_ADDR);
    
    const oled_display_config_t cfg = {
        .i2c_port = PH_OLED_I2C_PORT,
        .sda_pin = PH_OLED_SDA_PIN,
        .scl_pin = PH_OLED_SCL_PIN,
        .clk_speed_hz = 100000,
        .i2c_address = PH_OLED_I2C_ADDR,
        .width = 128,
        .height = 64,
        .line_count = 4,
    };

    // Пытаемся инициализировать OLED (может быть уже инициализирован в app_main)
    // oled_display_init() вернет ESP_OK если OLED уже инициализирован
    ESP_LOGI(TAG, "   Calling oled_display_init()...");
    esp_err_t init_err = oled_display_init(&cfg);
    ESP_LOGI(TAG, "   oled_display_init() returned: %s (0x%x)", esp_err_to_name(init_err), init_err);
    if (init_err != ESP_OK) {
        ESP_LOGE(TAG, "   ❌ OLED init failed: %s (0x%x)", esp_err_to_name(init_err), init_err);
        return;
    }
    ESP_LOGI(TAG, "   ✅ OLED init OK (may be already initialized)");

    // Проверяем, запущена ли уже задача (OLED мог быть инициализирован в app_main)
    const oled_display_task_config_t task_cfg = {
        .stack_size = 4096,
        .priority = 4,
        .queue_depth = 8,
        .heartbeat_timeout_ticks = pdMS_TO_TICKS(1500),
    };

    // Пытаемся запустить задачу (если уже запущена, вернет ESP_OK)
    ESP_LOGI(TAG, "   Calling oled_display_start_task()...");
    esp_err_t task_err = oled_display_start_task(&task_cfg);
    ESP_LOGI(TAG, "   oled_display_start_task() returned: %s (0x%x)", esp_err_to_name(task_err), task_err);
    if (task_err != ESP_OK) {
        // Если задача не запустилась, но OLED инициализирован, все равно продолжаем
        // (задача могла быть запущена в app_main)
        ESP_LOGI(TAG, "   ⚠️ Task start returned error (may be already running), continuing...");
    } else {
        ESP_LOGI(TAG, "   ✅ Task start OK");
    }

    // Устанавливаем шаблоны для нормального режима работы
    // Это должно работать даже если OLED был инициализирован в app_main
    ESP_LOGI(TAG, "   Setting templates for normal mode...");
    oled_display_set_template(0, "{node} {zone}");
    oled_display_set_template(1, "pH {ph} -> {target}");
    oled_display_set_template(2, "Pump {pump_up}/{pump_down}ml");
    oled_display_set_template(3, "{mode} Mesh {mesh} {rssi}");
    ESP_LOGI(TAG, "   ✅ Templates set");

    // Помечаем дисплей как готовый (OLED инициализирован и шаблоны установлены)
    s_display_ready = true;
    ESP_LOGI(TAG, "   ✅ s_display_ready = true");
    ESP_LOGI(TAG, "   ✅ OLED display ready for ph_manager");
    
    // Обновляем дисплей с текущими данными
    ESP_LOGI(TAG, "   Calling ph_display_update() with ph=%.2f, mesh=%d",
             s_current_ph, mesh_manager_is_connected());
    ph_display_update(s_current_ph, mesh_manager_is_connected(), mesh_manager_is_connected() ? get_rssi_to_parent() : 0);
    ESP_LOGI(TAG, "   ✅ ph_display_update() completed");
    ESP_LOGI(TAG, "🔵 [PH_OLED] ph_display_init_once() completed successfully");
}

static void ph_display_update(float ph_value, bool mesh_connected, int8_t rssi)
{
    if (!s_display_ready) {
        ESP_LOGW(TAG, "🔵 [PH_OLED] ph_display_update() called but s_display_ready = false");
        ESP_LOGW(TAG, "   ph=%.2f, mesh=%d, rssi=%d", ph_value, mesh_connected, rssi);
        return;
    }

    char ph_str[16];
    char target_str[16];
    char pump_up_str[16];
    char pump_down_str[16];
    char mode_str[8];
    char mesh_str[12];
    char rssi_str[16];

    snprintf(ph_str, sizeof(ph_str), "%.2f", ph_value);
    float target = s_config ? s_config->ph_target : 0.0f;
    snprintf(target_str, sizeof(target_str), "%.2f", target);

    uint32_t pump_up = pump_controller_get_total_ml(PUMP_PH_UP);
    uint32_t pump_down = pump_controller_get_total_ml(PUMP_PH_DOWN);
    snprintf(pump_up_str, sizeof(pump_up_str), "%lu", (unsigned long)pump_up);
    snprintf(pump_down_str, sizeof(pump_down_str), "%lu", (unsigned long)pump_down);

    const char *mode = s_emergency_mode ? "EMR" : (s_autonomous_mode ? "AUTO" : "RUN");
    strlcpy(mode_str, mode, sizeof(mode_str));
    strlcpy(mesh_str, mesh_connected ? "ONLINE" : "OFFLINE", sizeof(mesh_str));
    if (mesh_connected && rssi != 0) {
        snprintf(rssi_str, sizeof(rssi_str), "%ddBm", rssi);
    } else {
        strlcpy(rssi_str, "--", sizeof(rssi_str));
    }

    const char *node_id = (s_config && s_config->base.node_id[0]) ? s_config->base.node_id : "ph";
    const char *zone = (s_config && s_config->base.zone[0]) ? s_config->base.zone : "Zone";

    oled_display_kv_t values[] = {
        {.key = "node", .value = node_id},
        {.key = "zone", .value = zone},
        {.key = "ph", .value = ph_str},
        {.key = "target", .value = target_str},
        {.key = "pump_up", .value = pump_up_str},
        {.key = "pump_down", .value = pump_down_str},
        {.key = "mode", .value = mode_str},
        {.key = "mesh", .value = mesh_str},
        {.key = "rssi", .value = rssi_str},
    };

    // Логируем только первые несколько раз и при ошибках, чтобы не засорять логи
    static int update_count = 0;
    update_count++;
    if (update_count <= 3) {
        ESP_LOGI(TAG, "🔵 [PH_OLED] ph_display_update() #%d: ph=%.2f, target=%.2f, mesh=%s, mode=%s",
                 update_count, ph_value, target, mesh_str, mode_str);
    }
    
    esp_err_t err = oled_display_queue_render(values,
                                              sizeof(values) / sizeof(values[0]),
                                              0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "🔵 [PH_OLED] ph_display_update() #%d: ❌ OLED render failed: %s (0x%x)",
                 update_count, esp_err_to_name(err), err);
        return;
    }
    if (update_count <= 3) {
        ESP_LOGI(TAG, "   ✅ OLED render queued successfully");
    }

    s_display_last_ph = ph_value;
    s_display_last_target = target;
    s_display_pump_up_ml = pump_up;
    s_display_pump_down_ml = pump_down;
    s_display_last_mesh = mesh_connected;
    s_display_last_rssi = rssi;
    s_display_has_data = true;
}

static void ph_display_show_state(bool mesh_connected)
{
    ESP_LOGI(TAG, "🔵 [PH_OLED] ph_display_show_state() called: mesh=%d, s_display_ready=%d",
             mesh_connected, s_display_ready);
    
    if (!s_display_ready) {
        ESP_LOGW(TAG, "   ⚠️ Display not ready, skipping");
        return;
    }

    float ph_value = s_display_has_data ? s_display_last_ph : s_current_ph;
    int8_t rssi = mesh_connected ? get_rssi_to_parent() : s_display_last_rssi;
    ESP_LOGI(TAG, "   Calling ph_display_update(ph=%.2f, mesh=%d, rssi=%d)", ph_value, mesh_connected, rssi);

    ph_display_update(ph_value, mesh_connected, rssi);
}

static void ph_display_show_heartbeat(void)
{
    if (!s_display_ready) {
        return;
    }
    esp_err_t err = oled_display_show_heartbeat(true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "OLED heartbeat failed: %s", esp_err_to_name(err));
    }
}

// Отправка event сообщения с полными метаданными
static void send_event(mesh_event_level_t level, const char *message, float value) {
    // Логирование события всегда (даже если mesh не подключен)
    if (s_config == NULL) {
        ESP_LOGE(TAG, "[EVENT] %s (pH=%.2f) - config not initialized!", message, value);
        return;
    }
    
    const char *level_str = mesh_protocol_event_level_to_str(level);
    if (level_str == NULL) {
        level_str = "UNKNOWN";
    }
    
    if (level == MESH_EVENT_CRITICAL || level == MESH_EVENT_EMERGENCY) {
        ESP_LOGE(TAG, "[EVENT %s] %s (pH=%.2f, target=%.2f)", level_str, message, value, s_config->ph_target);
    } else if (level == MESH_EVENT_WARNING) {
        ESP_LOGW(TAG, "[EVENT %s] %s (pH=%.2f, target=%.2f)", level_str, message, value, s_config->ph_target);
    } else {
        ESP_LOGI(TAG, "[EVENT %s] %s (pH=%.2f, target=%.2f)", level_str, message, value, s_config->ph_target);
    }
    
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "   [WARNING] Event not sent - mesh offline");
        return;  // Нельзя отправить если offline
    }
    
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "   [ERROR] Failed to create JSON root");
        return;
    }
    
    cJSON_AddStringToObject(root, "type", "event");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "ph");
    const char *mesh_id = zone_config_validate(s_mesh_network_id) ? s_mesh_network_id : ZONE_CONFIG_UNCONFIGURED;
    const char *root_id = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_id);
    cJSON_AddStringToObject(root, "root_node_id", root_id);
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_id);
    cJSON_AddStringToObject(root, "root_node_id", root_id);
    cJSON_AddStringToObject(root, "level", mesh_protocol_event_level_to_str(level));
    cJSON_AddStringToObject(root, "message", message);
    cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
    
    // Полные метаданные для любого события
    cJSON *data = cJSON_CreateObject();
    
    // Основные параметры pH
    cJSON_AddNumberToObject(data, "ph", value);
    cJSON_AddNumberToObject(data, "ph_target", s_config->ph_target);
    cJSON_AddNumberToObject(data, "ph_min", s_config->ph_min);
    cJSON_AddNumberToObject(data, "ph_max", s_config->ph_max);
    cJSON_AddNumberToObject(data, "ph_emergency_low", s_config->ph_emergency_low);
    cJSON_AddNumberToObject(data, "ph_emergency_high", s_config->ph_emergency_high);
    
    // Текущее состояние PID контроллеров
    cJSON *pid_up = cJSON_CreateObject();
    cJSON_AddNumberToObject(pid_up, "setpoint", s_pid_ph_up.setpoint);
    cJSON_AddNumberToObject(pid_up, "integral", s_pid_ph_up.integral);
    cJSON_AddNumberToObject(pid_up, "prev_error", s_pid_ph_up.prev_error);
    cJSON_AddStringToObject(pid_up, "zone", adaptive_pid_zone_to_str(s_pid_ph_up.current_zone));
    cJSON_AddItemToObject(data, "pid_up", pid_up);
    
    cJSON *pid_down = cJSON_CreateObject();
    cJSON_AddNumberToObject(pid_down, "setpoint", s_pid_ph_down.setpoint);
    cJSON_AddNumberToObject(pid_down, "integral", s_pid_ph_down.integral);
    cJSON_AddNumberToObject(pid_down, "prev_error", s_pid_ph_down.prev_error);
    cJSON_AddStringToObject(pid_down, "zone", adaptive_pid_zone_to_str(s_pid_ph_down.current_zone));
    cJSON_AddItemToObject(data, "pid_down", pid_down);
    
    // Состояние системы
    cJSON_AddBoolToObject(data, "autonomous_mode", s_autonomous_mode);
    cJSON_AddBoolToObject(data, "emergency_mode", s_emergency_mode);
    
    // Конфигурация безопасности
    cJSON_AddNumberToObject(data, "max_pump_time_ms", s_config->max_pump_time_ms);
    cJSON_AddNumberToObject(data, "cooldown_ms", s_config->cooldown_ms);
    cJSON_AddNumberToObject(data, "max_daily_volume_ml", s_config->max_daily_volume_ml);
    
    // Калибровка насосов
    cJSON *pump_cal = cJSON_CreateArray();
    for (int i = 0; i < 2; i++) {
        cJSON *cal = cJSON_CreateObject();
        cJSON_AddNumberToObject(cal, "pump_id", i);
        cJSON_AddNumberToObject(cal, "ml_per_second", s_config->pump_calibration[i].ml_per_second);
        cJSON_AddBoolToObject(cal, "is_calibrated", s_config->pump_calibration[i].is_calibrated);
        cJSON_AddItemToArray(pump_cal, cal);
    }
    cJSON_AddItemToObject(data, "pump_calibration", pump_cal);
    
    // Метаданные узла
    cJSON_AddNumberToObject(data, "uptime_sec", (uint32_t)((time(NULL) - s_boot_time)));
    cJSON_AddNumberToObject(data, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(data, "rssi", get_rssi_to_parent());
    
    // Информация о mesh
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        cJSON_AddNumberToObject(data, "wifi_rssi", ap_info.rssi);
        char bssid_str[18];
        snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                 ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
        cJSON_AddStringToObject(data, "parent_bssid", bssid_str);
    }
    
    cJSON_AddItemToObject(root, "data", data);
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "   [OK] Event sent to ROOT (%d bytes)", (int)strlen(json_str));
        } else {
            ESP_LOGW(TAG, "   [ERROR] Failed to send event: %s", esp_err_to_name(err));
        }
        free(json_str);
    } else {
        ESP_LOGE(TAG, "   [ERROR] Failed to serialize JSON");
    }
    
    cJSON_Delete(root);
}

// Отправка события с расширенными метаданными (для агрессивных коррекций)
static void send_event_with_metadata(mesh_event_level_t level, const char *message, 
                                     float ph, float output, float error, 
                                     pid_zone_t zone, pump_id_t pump_id) {
    // Логирование события всегда (даже если mesh не подключен)
    if (s_config == NULL) {
        ESP_LOGE(TAG, "[EVENT] %s (pH=%.2f) - config not initialized!", message, ph);
        return;
    }
    
    const char *level_str = mesh_protocol_event_level_to_str(level);
    if (level_str == NULL) {
        level_str = "UNKNOWN";
    }
    const char *pump_name = (pump_id == PUMP_PH_UP) ? "pH UP" : "pH DOWN";
    ESP_LOGW(TAG, "[EVENT %s] %s [pH=%.2f, error=%.2f, zone=%s, pump=%s, output=%.2f ml]", 
             level_str, message, ph, error, adaptive_pid_zone_to_str(zone), pump_name, output);
    
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "   [WARNING] Event not sent - mesh offline");
        return;
    }
    
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "   [ERROR] Failed to create JSON root");
        return;
    }
    
    cJSON_AddStringToObject(root, "type", "event");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(root, "node_type", "ph");
    cJSON_AddStringToObject(root, "level", mesh_protocol_event_level_to_str(level));
    cJSON_AddStringToObject(root, "message", message);
    cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
    
    // Расширенные данные события
    cJSON *data = cJSON_CreateObject();
    
    // Основные параметры pH
    cJSON_AddNumberToObject(data, "ph", ph);
    cJSON_AddNumberToObject(data, "ph_target", s_config->ph_target);
    cJSON_AddNumberToObject(data, "ph_min", s_config->ph_min);
    cJSON_AddNumberToObject(data, "ph_max", s_config->ph_max);
    cJSON_AddNumberToObject(data, "ph_emergency_low", s_config->ph_emergency_low);
    cJSON_AddNumberToObject(data, "ph_emergency_high", s_config->ph_emergency_high);
    
    // Параметры коррекции
    cJSON_AddNumberToObject(data, "error", error);
    cJSON_AddNumberToObject(data, "correction_ml", output);
    cJSON_AddStringToObject(data, "zone", adaptive_pid_zone_to_str(zone));
    cJSON_AddNumberToObject(data, "pump_id", pump_id);
    cJSON_AddStringToObject(data, "pump_name", pump_id == PUMP_PH_UP ? "pH UP" : "pH DOWN");
    
    // PID коэффициенты активной зоны
    adaptive_pid_t *pid = (pump_id == PUMP_PH_UP) ? &s_pid_ph_up : &s_pid_ph_down;
    pid_coeffs_t *coeffs = (zone == ZONE_DEAD) ? &pid->coeffs_dead :
                           (zone == ZONE_CLOSE) ? &pid->coeffs_close :
                           &pid->coeffs_far;
    
    cJSON *pid_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(pid_obj, "kp", coeffs->kp);
    cJSON_AddNumberToObject(pid_obj, "ki", coeffs->ki);
    cJSON_AddNumberToObject(pid_obj, "kd", coeffs->kd);
    cJSON_AddNumberToObject(pid_obj, "integral", pid->integral);
    cJSON_AddNumberToObject(pid_obj, "prev_error", pid->prev_error);
    cJSON_AddItemToObject(data, "pid", pid_obj);
    
    // Состояние системы
    cJSON_AddBoolToObject(data, "autonomous_mode", s_autonomous_mode);
    cJSON_AddBoolToObject(data, "emergency_mode", s_emergency_mode);
    
    // Метаданные узла
    cJSON_AddNumberToObject(data, "uptime_sec", (uint32_t)((time(NULL) - s_boot_time)));
    cJSON_AddNumberToObject(data, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(data, "rssi", get_rssi_to_parent());
    
    cJSON_AddItemToObject(root, "data", data);
    
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "   [OK] Event sent to ROOT (%d bytes)", (int)strlen(json_str));
        } else {
            ESP_LOGW(TAG, "   [ERROR] Failed to send event: %s", esp_err_to_name(err));
        }
        free(json_str);
    } else {
        ESP_LOGE(TAG, "   [ERROR] Failed to serialize JSON");
    }
    
    cJSON_Delete(root);
}

// Проверка аварийных условий
static void check_emergency_conditions(void) {
    // Emergency если pH слишком низкий или высокий (используем настраиваемые пороги)
    if (s_current_ph < s_config->ph_emergency_low || s_current_ph > s_config->ph_emergency_high) {
        if (!s_emergency_mode) {
            ESP_LOGE(TAG, "EMERGENCY: pH out of range (%.2f) - limits: %.2f-%.2f", 
                     s_current_ph, s_config->ph_emergency_low, s_config->ph_emergency_high);
            ph_manager_set_emergency(true);
            
            // Отправка критичного события
            if (s_current_ph < s_config->ph_emergency_low) {
                send_event(MESH_EVENT_CRITICAL, "pH too low", s_current_ph);
            } else {
                send_event(MESH_EVENT_CRITICAL, "pH too high", s_current_ph);
            }
        }
    } else {
        if (s_emergency_mode) {
            ESP_LOGI(TAG, "pH back to normal range (%.2f)", s_current_ph);
            ph_manager_set_emergency(false);
            
            // Отправка события восстановления
            send_event(MESH_EVENT_INFO, "pH back to normal", s_current_ph);
        }
    }
}

// Получение RSSI
static int8_t get_rssi_to_parent(void) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return -100;
}

// Обработка команд
void ph_manager_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "=== PH_MANAGER_HANDLE_COMMAND ===");
    ESP_LOGI(TAG, "Command received: %s", command ? command : "NULL");
    ESP_LOGI(TAG, "Params: %s", params ? "found" : "NULL");
    
    if (strcmp(command, "set_ph_target") == 0) {
        ph_manager_handle_ph_target_command(params);
    }
    else if (strcmp(command, "emergency_stop") == 0) {
        ph_manager_set_emergency(true);
    }
    else if (strcmp(command, "reset_emergency") == 0) {
        ph_manager_set_emergency(false);
    }
    else if (strcmp(command, "run_pump") == 0) {
        ph_manager_handle_pump_command(params);
    }
    else if (strcmp(command, "reset_stats") == 0) {
        ph_manager_handle_reset_stats_command();
    }
    else if (strcmp(command, "run_pump_manual") == 0) {
        ph_manager_handle_manual_pump_command(params);
    }
    else if (strcmp(command, "calibrate_pump") == 0) {
        cJSON *pump_id = cJSON_GetObjectItem(params, "pump_id");
        cJSON *duration_sec = cJSON_GetObjectItem(params, "duration_sec");
        cJSON *volume_ml = cJSON_GetObjectItem(params, "volume_ml");
        
        if (cJSON_IsNumber(pump_id) && cJSON_IsNumber(duration_sec) && cJSON_IsNumber(volume_ml)) {
            int pump = pump_id->valueint;
            float duration = (float)duration_sec->valuedouble;
            float volume = (float)volume_ml->valuedouble;
            
            // Валидация
            if (pump < 0 || pump > 1) {
                ESP_LOGW(TAG, "Invalid pump_id: %d", pump);
                return;
            }
            if (duration <= 0.0f || volume <= 0.0f) {
                ESP_LOGW(TAG, "Invalid calibration params");
                return;
            }
            
            // Сохранение калибровки
            s_config->pump_calibration[pump].calibration_time_ms = (uint32_t)(duration * 1000.0f);
            s_config->pump_calibration[pump].calibration_volume_ml = volume;
            s_config->pump_calibration[pump].ml_per_second = volume / duration;
            s_config->pump_calibration[pump].is_calibrated = true;
            s_config->pump_calibration[pump].last_calibrated = (uint64_t)time(NULL);
            
            // Сохранение в NVS
            esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Pump %d calibrated: %.2f ml/sec (%.1f ml in %.1f sec)", 
                         pump, s_config->pump_calibration[pump].ml_per_second, volume, duration);
            } else {
                ESP_LOGE(TAG, "Failed to save calibration to NVS");
            }
        }
    }
    else if (strcmp(command, "force_mock_mode") == 0) {
        cJSON *enable = cJSON_GetObjectItem(params, "enable");
        if (cJSON_IsBool(enable)) {
            bool mock_enable = cJSON_IsTrue(enable);
            ph_sensor_force_mock_mode(mock_enable);
            ESP_LOGI(TAG, "Mock mode %s", mock_enable ? "enabled" : "disabled");
        }
    }
    else if (strcmp(command, "set_sensor_mode") == 0) {
        cJSON *mode = cJSON_GetObjectItem(params, "mode");
        if (cJSON_IsNumber(mode)) {
            uint8_t new_mode = (uint8_t)mode->valueint;
            if (new_mode <= 2) {
                s_config->sensor_mode = new_mode;
                ph_sensor_set_mode((ph_sensor_mode_t)new_mode);
                
                // Сохраняем в NVS
                esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
                if (err == ESP_OK) {
                    const char *mode_str = (new_mode == 0) ? "REAL" : 
                                          (new_mode == 1) ? "MOCK REACTIVE" : "MOCK NON-REACTIVE";
                    ESP_LOGI(TAG, "Sensor mode set to %s (%d) and saved to NVS", mode_str, new_mode);
                } else {
                    ESP_LOGE(TAG, "Failed to save sensor mode to NVS");
                }
            } else {
                ESP_LOGW(TAG, "Invalid sensor mode: %d (must be 0-2)", new_mode);
            }
        }
    }
    else if (strcmp(command, "get_sensor_status") == 0) {
        // Проверка подключения к mesh
        if (!mesh_manager_is_connected()) {
            ESP_LOGW(TAG, "Cannot send status: mesh offline");
            return;
        }
        
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            ESP_LOGE(TAG, "Failed to create root JSON object");
            return;
        }
        
        cJSON_AddStringToObject(root, "type", "sensor_status");
        cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
        cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
        
        cJSON *status = cJSON_CreateObject();
        if (status) {
            cJSON_AddBoolToObject(status, "connected", ph_sensor_is_connected());
            cJSON_AddBoolToObject(status, "mock_mode", ph_sensor_is_mock_mode());
            cJSON_AddBoolToObject(status, "stable", ph_sensor_is_stable());
            cJSON_AddNumberToObject(status, "current_ph", s_current_ph);
            cJSON_AddNumberToObject(status, "ph_target", s_config->ph_target);
            cJSON_AddItemToObject(root, "status", status);
        }
        
        char *json_str = cJSON_PrintUnformatted(root);
        if (json_str) {
            mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
            ESP_LOGI(TAG, "Sensor status sent");
            free(json_str);
        }
        
        cJSON_Delete(root);
    }
    else if (strcmp(command, "set_ph_target") == 0) {
        cJSON *ph_target = cJSON_GetObjectItem(params, "ph_target");
        if (cJSON_IsNumber(ph_target)) {
            float new_target = (float)ph_target->valuedouble;
            if (new_target >= 5.0f && new_target <= 8.0f) {
                s_config->ph_target = new_target;
                // Обновляем PID контроллеры
                adaptive_pid_set_target(&s_pid_ph_up, new_target);
                adaptive_pid_set_target(&s_pid_ph_down, new_target);
                
                // Сохраняем в NVS
                esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "pH target set to %.2f and saved to NVS", new_target);
                } else {
                    ESP_LOGE(TAG, "Failed to save pH target to NVS: %s", esp_err_to_name(err));
                }
            } else {
                ESP_LOGW(TAG, "Invalid pH target: %.2f (must be 5.0-8.0)", new_target);
            }
        }
    }
    else if (strcmp(command, "set_ec_target") == 0) {
        cJSON *ec_target = cJSON_GetObjectItem(params, "ec_target");
        if (cJSON_IsNumber(ec_target)) {
            float new_target = (float)ec_target->valuedouble;
            if (new_target >= 0.5f && new_target <= 3.0f) {
                // Для pH узла EC target не используется, но сохраняем для совместимости
                ESP_LOGI(TAG, "EC target set to %.2f (not used in pH node)", new_target);
            } else {
                ESP_LOGW(TAG, "Invalid EC target: %.2f (must be 0.5-3.0)", new_target);
            }
        }
    }
    else if (strcmp(command, "set_autonomous_mode") == 0) {
        cJSON *enable = cJSON_GetObjectItem(params, "enable");
        if (cJSON_IsBool(enable)) {
            bool autonomous = cJSON_IsTrue(enable);
            s_config->autonomous_enabled = autonomous;
            s_autonomous_mode = autonomous;
            
            // Сохраняем в NVS
            esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Autonomous mode %s", autonomous ? "enabled" : "disabled");
            } else {
                ESP_LOGE(TAG, "Failed to save autonomous mode to NVS");
            }
        }
    }
    else if (strcmp(command, "set_safety_settings") == 0 || strcmp(command, "update_safety_config") == 0) {
        ph_manager_handle_safety_command(params);
    }
    else if (strcmp(command, "emergency_stop") == 0) {
        ph_manager_set_emergency(true);
        ESP_LOGW(TAG, "EMERGENCY STOP activated via command");
    }
    else if (strcmp(command, "reset_emergency") == 0) {
        ph_manager_set_emergency(false);
        ESP_LOGI(TAG, "Emergency mode reset via command");
    }
    else if (strcmp(command, "get_config") == 0) {
        ph_manager_send_config_response();
    }
    else {
        ESP_LOGW(TAG, "Unknown command: %s", command);
    }
}

// Обновление конфигурации
void ph_manager_handle_config_update(cJSON *config_json) {
    ESP_LOGI(TAG, "Config update received");
    
    bool config_changed = false;
    
    cJSON *ph_target = cJSON_GetObjectItem(config_json, "ph_target");
    if (cJSON_IsNumber(ph_target)) {
        float new_target = (float)ph_target->valuedouble;
        if (new_target >= 5.0f && new_target <= 9.0f) {
            s_config->ph_target = new_target;
            adaptive_pid_set_setpoint(&s_pid_ph_up, s_config->ph_target);
            adaptive_pid_set_setpoint(&s_pid_ph_down, s_config->ph_target);
            config_changed = true;
            ESP_LOGI(TAG, "pH target updated: %.2f", s_config->ph_target);
        }
    }
    
    cJSON *ph_min = cJSON_GetObjectItem(config_json, "ph_min");
    if (cJSON_IsNumber(ph_min)) {
        float new_min = (float)ph_min->valuedouble;
        if (new_min >= 4.0f && new_min <= 8.0f) {
            s_config->ph_min = new_min;
            config_changed = true;
            ESP_LOGI(TAG, "pH min updated: %.2f", s_config->ph_min);
        }
    }
    
    cJSON *ph_max = cJSON_GetObjectItem(config_json, "ph_max");
    if (cJSON_IsNumber(ph_max)) {
        float new_max = (float)ph_max->valuedouble;
        if (new_max >= 6.0f && new_max <= 10.0f) {
            s_config->ph_max = new_max;
            config_changed = true;
            ESP_LOGI(TAG, "pH max updated: %.2f", s_config->ph_max);
        }
    }
    
    // PID параметры
    cJSON *pid_params = cJSON_GetObjectItem(config_json, "pid_params");
    if (cJSON_IsObject(pid_params)) {
        cJSON *kp = cJSON_GetObjectItem(pid_params, "kp");
        cJSON *ki = cJSON_GetObjectItem(pid_params, "ki");
        cJSON *kd = cJSON_GetObjectItem(pid_params, "kd");
        
        if (cJSON_IsNumber(kp) && cJSON_IsNumber(ki) && cJSON_IsNumber(kd)) {
            s_config->pump_pid[0].kp = (float)kp->valuedouble;
            s_config->pump_pid[0].ki = (float)ki->valuedouble;
            s_config->pump_pid[0].kd = (float)kd->valuedouble;
            s_config->pump_pid[1].kp = (float)kp->valuedouble;
            s_config->pump_pid[1].ki = (float)ki->valuedouble;
            s_config->pump_pid[1].kd = (float)kd->valuedouble;
            
            // Переинициализация адаптивных PID
            adaptive_pid_init(&s_pid_ph_up, s_config->ph_target,
                             s_config->pump_pid[0].kp, 
                             s_config->pump_pid[0].ki, 
                             s_config->pump_pid[0].kd);
            adaptive_pid_init(&s_pid_ph_down, s_config->ph_target,
                             s_config->pump_pid[1].kp,
                             s_config->pump_pid[1].ki, 
                             s_config->pump_pid[1].kd);
            
            config_changed = true;
            ESP_LOGI(TAG, "PID params updated: Kp=%.2f Ki=%.2f Kd=%.2f", 
                     (float)kp->valuedouble, (float)ki->valuedouble, (float)kd->valuedouble);
        }
    }
    
    // ВАЖНО: Сохранение в NVS
    if (config_changed) {
        esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Configuration saved to NVS");
        } else {
            ESP_LOGE(TAG, "Failed to save config to NVS: %s", esp_err_to_name(err));
        }
    }
}

// ============================================================================
// Внутренние функции для обработки команд
// ============================================================================

/**
 * @brief Обработка команды set_ph_target
 */
void ph_manager_handle_ph_target_command(cJSON *params) {
    cJSON *target = cJSON_GetObjectItem(params, "target");
    if (cJSON_IsNumber(target)) {
        float new_target = (float)target->valuedouble;
        
        // Валидация диапазона
        if (new_target < 5.0f || new_target > 9.0f) {
            ESP_LOGW(TAG, "Invalid pH target: %.2f (must be 5.0-9.0)", new_target);
            return;
        }
        
        s_config->ph_target = new_target;
        adaptive_pid_set_setpoint(&s_pid_ph_up, s_config->ph_target);
        adaptive_pid_set_setpoint(&s_pid_ph_down, s_config->ph_target);
        
        // ВАЖНО: Сохранение в NVS!
        esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "pH target set to %.2f and saved to NVS", s_config->ph_target);
        } else {
            ESP_LOGE(TAG, "Failed to save pH target to NVS: %s", esp_err_to_name(err));
        }
    }
}

/**
 * @brief Обработка команд насосов
 */
void ph_manager_handle_pump_command(cJSON *params) {
    cJSON *pump_id = cJSON_GetObjectItem(params, "pump_id");
    cJSON *duration = cJSON_GetObjectItem(params, "duration_ms");
    
    if (cJSON_IsNumber(pump_id) && cJSON_IsNumber(duration)) {
        pump_id_t pump = (pump_id_t)pump_id->valueint;
        uint32_t dur = (uint32_t)duration->valueint;
        
        if (pump < PUMP_MAX && dur > 0 && dur <= 10000) {
            ESP_LOGI(TAG, "Manual pump run: %d for %lu ms", pump, dur);
            pump_controller_run(pump, dur);
        }
    }
}

/**
 * @brief Обработка команды reset_stats
 */
void ph_manager_handle_reset_stats_command(void) {
    pump_controller_reset_stats(PUMP_PH_UP);
    pump_controller_reset_stats(PUMP_PH_DOWN);
    ESP_LOGI(TAG, "Pump stats reset");
}

/**
 * @brief Обработка команды run_pump_manual
 */
void ph_manager_handle_manual_pump_command(cJSON *params) {
    ESP_LOGI(TAG, "=== RUN PUMP MANUAL COMMAND ===");
    
    // Диагностика блокировок перед запуском
    if (s_emergency_mode) {
        ESP_LOGW(TAG, "Manual pump blocked: EMERGENCY mode active");
        return;
    }
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Manual pump: mesh not connected (command will still be executed locally)");
    }
    
    // Отладочный вывод параметров
    if (params) {
        char *params_str = cJSON_PrintUnformatted(params);
        ESP_LOGI(TAG, "Command params: %s", params_str ? params_str : "NULL");
        if (params_str) free(params_str);
    } else {
        ESP_LOGW(TAG, "Command params is NULL!");
    }
    
    cJSON *pump_id = cJSON_GetObjectItem(params, "pump_id");
    cJSON *duration_sec = cJSON_GetObjectItem(params, "duration_sec");
    
    ESP_LOGI(TAG, "pump_id: %s, duration_sec: %s", 
             pump_id ? "found" : "NULL", 
             duration_sec ? "found" : "NULL");
    
    if (cJSON_IsNumber(pump_id) && cJSON_IsNumber(duration_sec)) {
        int pump = pump_id->valueint;
        float duration = (float)duration_sec->valuedouble;
        
        // Валидация
        if (pump < 0 || pump > 1) {
            ESP_LOGW(TAG, "Invalid pump_id: %d (must be 0-1)", pump);
            return;
        }
        if (duration <= 0.0f || duration > 30.0f) {
            ESP_LOGW(TAG, "Invalid duration: %.1f sec (must be 0.1-30.0)", duration);
            return;
        }
        
        // Запуск насоса вручную
        pump_id_t pid = (pump == 0) ? PUMP_PH_UP : PUMP_PH_DOWN;
        uint32_t duration_ms = (uint32_t)(duration * 1000.0f);
        
        ESP_LOGI(TAG, "Manual pump run: PUMP_%s (id=%d) for %.1f sec (%lu ms)", 
                 (pump == 0) ? "UP" : "DOWN", pump, duration, (unsigned long)duration_ms);
        // Информативный вывод текущей калибровки
        ESP_LOGI(TAG, "Calibration: pump %d = %.2f ml/s", pump, s_config->pump_calibration[pump].ml_per_second);
        pump_controller_run(pid, duration_ms);
    }
}

/**
 * @brief Обработка safety команд
 */
void ph_manager_handle_safety_command(cJSON *params) {
    // Safety параметры
    cJSON *max_pump_time = cJSON_GetObjectItem(params, "max_pump_time_ms");
    cJSON *cooldown = cJSON_GetObjectItem(params, "cooldown_ms");
    cJSON *max_daily_volume = cJSON_GetObjectItem(params, "max_daily_volume_ml");
    
    // Emergency пороги
    cJSON *ph_emergency_low = cJSON_GetObjectItem(params, "ph_emergency_low");
    cJSON *ph_emergency_high = cJSON_GetObjectItem(params, "ph_emergency_high");
    
    if (cJSON_IsNumber(max_pump_time)) {
        s_config->max_pump_time_ms = (uint32_t)max_pump_time->valuedouble;
    }
    if (cJSON_IsNumber(cooldown)) {
        s_config->cooldown_ms = (uint32_t)cooldown->valuedouble;
    }
    if (cJSON_IsNumber(max_daily_volume)) {
        s_config->max_daily_volume_ml = (uint32_t)max_daily_volume->valuedouble;
    }
    if (cJSON_IsNumber(ph_emergency_low)) {
        s_config->ph_emergency_low = (float)ph_emergency_low->valuedouble;
    }
    if (cJSON_IsNumber(ph_emergency_high)) {
        s_config->ph_emergency_high = (float)ph_emergency_high->valuedouble;
    }
    
    // Сохраняем в NVS
    esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Safety settings updated: max_pump=%lu ms, cooldown=%lu ms, daily_vol=%lu ml, ph_emergency=%.2f-%.2f", 
                 s_config->max_pump_time_ms, s_config->cooldown_ms, s_config->max_daily_volume_ml,
                 s_config->ph_emergency_low, s_config->ph_emergency_high);
    } else {
        ESP_LOGE(TAG, "Failed to save safety settings to NVS");
    }
}

/**
 * @brief Отправка config response
 */
void ph_manager_send_config_response(void) {
    // Проверка подключения к mesh
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Cannot send config: mesh offline");
        return;
    }
    
    // Создание корневого объекта
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create root JSON object");
        return;
    }
    
    cJSON_AddStringToObject(root, "type", "config_response");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
    cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
    const char *mesh_id = zone_config_validate(s_mesh_network_id) ? s_mesh_network_id : ZONE_CONFIG_UNCONFIGURED;
    const char *root_id = (s_root_node_id[0] != '\0') ? s_root_node_id : ZONE_CONFIG_UNCONFIGURED;
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_id);
    cJSON_AddStringToObject(root, "root_node_id", root_id);
    
    // Создание объекта конфигурации
    cJSON *config = cJSON_CreateObject();
    if (!config) {
        ESP_LOGE(TAG, "Failed to create config JSON object");
        cJSON_Delete(root);
        return;
    }
    
    cJSON_AddStringToObject(config, "node_id", s_config->base.node_id);
    cJSON_AddStringToObject(config, "node_type", "ph");
    cJSON_AddStringToObject(config, "zone", s_config->base.zone);
    
    // pH параметры
    cJSON_AddNumberToObject(config, "ph_target", s_config->ph_target);
    cJSON_AddNumberToObject(config, "ph_min", s_config->ph_min);
    cJSON_AddNumberToObject(config, "ph_max", s_config->ph_max);
    cJSON_AddNumberToObject(config, "ph_cal_offset", s_config->ph_cal_offset);
    
    // Калибровка насосов
    cJSON *pumps_cal = cJSON_CreateArray();
    if (pumps_cal) {
        for (int i = 0; i < 2; i++) {
            cJSON *pump = cJSON_CreateObject();
            if (pump) {
                cJSON_AddNumberToObject(pump, "pump_id", i);
                cJSON_AddNumberToObject(pump, "ml_per_second", s_config->pump_calibration[i].ml_per_second);
                cJSON_AddNumberToObject(pump, "calibration_volume_ml", s_config->pump_calibration[i].calibration_volume_ml);
                cJSON_AddNumberToObject(pump, "calibration_time_ms", s_config->pump_calibration[i].calibration_time_ms);
                cJSON_AddBoolToObject(pump, "is_calibrated", s_config->pump_calibration[i].is_calibrated);
                cJSON_AddNumberToObject(pump, "last_calibrated", (double)s_config->pump_calibration[i].last_calibrated);
                cJSON_AddItemToArray(pumps_cal, pump);
            }
        }
        cJSON_AddItemToObject(config, "pumps_calibration", pumps_cal);
    }
    
    // PID параметры
    cJSON *pumps_pid = cJSON_CreateArray();
    if (pumps_pid) {
        for (int i = 0; i < 2; i++) {
            cJSON *pid = cJSON_CreateObject();
            if (pid) {
                cJSON_AddNumberToObject(pid, "pump_id", i);
                cJSON_AddNumberToObject(pid, "kp", s_config->pump_pid[i].kp);
                cJSON_AddNumberToObject(pid, "ki", s_config->pump_pid[i].ki);
                cJSON_AddNumberToObject(pid, "kd", s_config->pump_pid[i].kd);
                cJSON_AddBoolToObject(pid, "enabled", s_config->pump_pid[i].enabled);
                cJSON_AddItemToArray(pumps_pid, pid);
            }
        }
        cJSON_AddItemToObject(config, "pumps_pid", pumps_pid);
    }
    
    // Safety параметры
    cJSON_AddNumberToObject(config, "max_pump_time_ms", s_config->max_pump_time_ms);
    cJSON_AddNumberToObject(config, "cooldown_ms", s_config->cooldown_ms);
    cJSON_AddNumberToObject(config, "max_daily_volume_ml", s_config->max_daily_volume_ml);
    
    // Emergency пороги
    cJSON_AddNumberToObject(config, "ph_emergency_low", s_config->ph_emergency_low);
    cJSON_AddNumberToObject(config, "ph_emergency_high", s_config->ph_emergency_high);
    
    // Автономия
    cJSON_AddBoolToObject(config, "autonomous_enabled", s_config->autonomous_enabled);
    cJSON_AddNumberToObject(config, "mesh_timeout_ms", s_config->mesh_timeout_ms);
    
    // Добавление config к root
    cJSON_AddItemToObject(root, "config", config);
    
    // Отправка конфигурации
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Config sent to ROOT (%d bytes)", strlen(json_str));
        } else {
            ESP_LOGE(TAG, "Failed to send config: %s", esp_err_to_name(err));
        }
        free(json_str);
    } else {
        ESP_LOGE(TAG, "Failed to serialize config JSON");
    }
    
    // Всегда освобождаем память
    cJSON_Delete(root);
}

