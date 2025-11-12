/**
 * @file ph_ec_manager.c
 * @brief pH/EC Manager implementation - упрощённая версия для демонстрации цепочки данных
 */

#include "ph_ec_manager.h"
#include "ph_sensor.h"
#include "ec_sensor.h"
#include "pump_controller.h"
#include "pid_controller.h"
#include "pump_events.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "mesh_config.h"  // Для HEARTBEAT_INTERVAL_MS
#include "zone_config.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>

static const char *TAG = "ph_ec_mgr";

// Состояние менеджера
static ph_ec_node_config_t *s_config = NULL;
static TaskHandle_t s_main_task = NULL;
static TaskHandle_t s_heartbeat_task = NULL;
static bool s_discovery_sent = false;
static uint32_t s_boot_time = 0;
static bool s_emergency_mode = false;
static bool s_autonomous_mode = false;
static char s_mesh_network_id[ZONE_CONFIG_MAX_LEN] = {0};
static char s_root_node_id[ZONE_CONFIG_MAX_LEN] = {0};

// PID контроллеры
static pid_controller_t s_pid_ph_up;
static pid_controller_t s_pid_ph_down;
static pid_controller_t s_pid_ec;

// Текущие значения
static float s_current_ph = 7.0f;
static float s_current_ec = 2.0f;

// Forward declarations
static void main_task(void *arg);
static void heartbeat_task(void *arg);
static void send_discovery(void);
static void send_telemetry(void);
static void send_heartbeat(void);
static int8_t get_rssi_to_parent(void);
static void read_sensors(void);
static void control_ph_ec(void);
static void check_emergency_conditions(void);

esp_err_t ph_ec_manager_init(ph_ec_node_config_t *config) {
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
    
    // Инициализация PID контроллеров с параметрами из конфигурации
    pid_init(&s_pid_ph_up, s_config->pump_pid[PUMP_PH_UP].kp, 
             s_config->pump_pid[PUMP_PH_UP].ki, 
             s_config->pump_pid[PUMP_PH_UP].kd);
    pid_set_setpoint(&s_pid_ph_up, s_config->ph_target);
    pid_set_output_limits(&s_pid_ph_up, 0.0f, 5.0f); // Макс 5 мл за раз
    
    pid_init(&s_pid_ph_down, s_config->pump_pid[PUMP_PH_DOWN].kp,
             s_config->pump_pid[PUMP_PH_DOWN].ki,
             s_config->pump_pid[PUMP_PH_DOWN].kd);
    pid_set_setpoint(&s_pid_ph_down, s_config->ph_target);
    pid_set_output_limits(&s_pid_ph_down, 0.0f, 5.0f);
    
    pid_init(&s_pid_ec, s_config->pump_pid[PUMP_EC_A].kp,
             s_config->pump_pid[PUMP_EC_A].ki,
             s_config->pump_pid[PUMP_EC_A].kd);
    pid_set_setpoint(&s_pid_ec, s_config->ec_target);
    pid_set_output_limits(&s_pid_ec, 0.0f, 10.0f);
    
    ESP_LOGI(TAG, "pH/EC Manager initialized");
    ESP_LOGI(TAG, "Node ID: %s, pH target: %.2f, EC target: %.2f", 
             s_config->base.node_id, s_config->ph_target, s_config->ec_target);
    ESP_LOGI(TAG, "Zone context: mesh_id=%s, root_id=%s",
             s_mesh_network_id, s_root_node_id);
    
    return ESP_OK;
}

esp_err_t ph_ec_manager_start(void) {
    if (s_main_task != NULL) {
        ESP_LOGW(TAG, "Already running");
        return ESP_OK;
    }
    
    // Запуск главной задачи
    BaseType_t ret = xTaskCreate(main_task, "ph_ec_main", 6144, NULL, 5, &s_main_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main task");
        return ESP_FAIL;
    }
    
    // Запуск heartbeat задачи
    ret = xTaskCreate(heartbeat_task, "heartbeat", 3072, NULL, 4, &s_heartbeat_task);
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Failed to create heartbeat task");
    }
    
    ESP_LOGI(TAG, "pH/EC Manager started");
    return ESP_OK;
}

esp_err_t ph_ec_manager_stop(void) {
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
    
    ESP_LOGI(TAG, "pH/EC Manager stopped");
    return ESP_OK;
}

esp_err_t ph_ec_manager_get_values(float *ph, float *ec) {
    if (ph) *ph = s_current_ph;
    if (ec) *ec = s_current_ec;
    return ESP_OK;
}

esp_err_t ph_ec_manager_set_emergency(bool enable) {
    s_emergency_mode = enable;
    
    if (enable) {
        ESP_LOGW(TAG, "EMERGENCY MODE ACTIVATED");
        pump_controller_emergency_stop();
    } else {
        ESP_LOGI(TAG, "Emergency mode deactivated");
    }
    
    return ESP_OK;
}

// Главная задача управления
static void main_task(void *arg) {
    ESP_LOGI(TAG, "Main task running");
    
    // Ожидание подключения к mesh
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
    
    uint32_t iteration = 0;
    
    while (1) {
        iteration++;
        
        // Проверка подключения mesh
        if (mesh_manager_is_connected() && !s_discovery_sent) {
            send_discovery();
            s_discovery_sent = true;
        }
        
        // Проверка автономного режима (если ROOT offline > 30 сек)
        // Упрощённо: считаем что всегда онлайн если mesh connected
        s_autonomous_mode = !mesh_manager_is_connected();
        
        // 1. Чтение датчиков
        read_sensors();
        
        // 2. Проверка аварийных условий
        check_emergency_conditions();
        
        // 3. Управление pH/EC (если не Emergency)
        if (!s_emergency_mode) {
            control_ph_ec();
        }
        
        // 4. Отправка telemetry каждые 5 секунд (DEBUG режим)
        if (iteration % 5 == 0 && mesh_manager_is_connected()) {
            send_telemetry();
        }
        
        // Цикл каждую секунду
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Задача heartbeat (каждые 5 секунд для DEBUG)
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

// Чтение датчиков - MOCK MODE (без реального I2C)
static void read_sensors(void) {
    // ⚠️ МОКОВЫЕ ЗНАЧЕНИЯ ДЛЯ ТЕСТИРОВАНИЯ БЕЗ ДАТЧИКОВ!
    
    // Генерация случайных отклонений для реалистичности
    static float ph_variation = 0.0f;
    static float ec_variation = 0.0f;
    
    // Случайное изменение ±0.05 каждую секунду
    ph_variation += ((float)(esp_random() % 100) - 50.0f) / 1000.0f;
    ec_variation += ((float)(esp_random() % 100) - 50.0f) / 1000.0f;
    
    // Ограничение вариаций
    if (ph_variation > 0.3f) ph_variation = 0.3f;
    if (ph_variation < -0.3f) ph_variation = -0.3f;
    if (ec_variation > 0.2f) ec_variation = 0.2f;
    if (ec_variation < -0.2f) ec_variation = -0.2f;
    
    // Моковые значения: pH=6.5±0.3, EC=2.5±0.2
    s_current_ph = 6.5f + ph_variation;
    s_current_ec = 2.5f + ec_variation;
    
    ESP_LOGD(TAG, "📊 Mock sensors: pH=%.2f, EC=%.2f", s_current_ph, s_current_ec);
    
    // ═══════════════════════════════════════════════════════
    // РЕАЛЬНЫЙ КОД (для production с настоящими датчиками):
    // ═══════════════════════════════════════════════════════
    // esp_err_t ret;
    // 
    // // Чтение pH
    // ret = ph_sensor_read(&s_current_ph);
    // if (ret != ESP_OK) {
    //     ESP_LOGW(TAG, "pH sensor read failed");
    // }
    // 
    // // Чтение EC
    // ret = ec_sensor_read(&s_current_ec);
    // if (ret != ESP_OK) {
    //     ESP_LOGW(TAG, "EC sensor read failed");
    // }
    // 
    // // Компенсация температуры для EC (упрощённо 25°C)
    // ec_sensor_set_temperature(25.0f);
    // 
    // ESP_LOGD(TAG, "Sensors: pH=%.2f, EC=%.2f", s_current_ph, s_current_ec);
}

// Проверка аварийных условий
static void check_emergency_conditions(void) {
    bool emergency = false;
    
    // Проверка pH emergency порогов (настраиваемые)
    if (s_current_ph < s_config->ph_emergency_low || s_current_ph > s_config->ph_emergency_high) {
        ESP_LOGW(TAG, "pH emergency: %.2f (limits: %.2f-%.2f)", 
                 s_current_ph, s_config->ph_emergency_low, s_config->ph_emergency_high);
        emergency = true;
    }
    
    // Проверка EC emergency порога (настраиваемый)
    if (s_current_ec > s_config->ec_emergency_high) {
        ESP_LOGW(TAG, "EC emergency: %.2f (limit: %.2f)",
                 s_current_ec, s_config->ec_emergency_high);
        emergency = true;
    }
    
    if (emergency && !s_emergency_mode) {
        ESP_LOGE(TAG, "EMERGENCY MODE ACTIVATED - pH: %.2f, EC: %.2f", s_current_ph, s_current_ec);
        ph_ec_manager_set_emergency(true);
    } else if (!emergency && s_emergency_mode) {
        ESP_LOGI(TAG, "Emergency mode deactivated - pH: %.2f, EC: %.2f", s_current_ph, s_current_ec);
        ph_ec_manager_set_emergency(false);
    }
}

// Управление pH/EC через PID
static void control_ph_ec(void) {
    float dt = 1.0f; // Цикл 1 секунда
    int8_t rssi = get_rssi_to_parent();
    
    // Управление pH
    if (s_current_ph < s_config->ph_target - 0.1f) {
        // pH низкий - нужен pH UP
        float dose = pid_compute(&s_pid_ph_up, s_current_ph, dt);
        if (dose > 0.1f) {
            // Получение данных PID для события
            pump_event_pid_data_t pid_data;
            pump_events_get_pid_data(&s_pid_ph_up, s_current_ph, &pid_data);
            pid_data.output = dose; // Устанавливаем реальный выход
            
            // Отправка события с реальными данными PID
            pump_events_send_start_event(
                PUMP_PH_UP,
                (uint32_t)((dose / 2.0f) * 1000.0f), // Примерная длительность
                dose,
                &pid_data,
                s_current_ph, s_current_ec,
                s_config->ph_target, s_config->ec_target,
                s_emergency_mode, s_autonomous_mode,
                rssi
            );
            
            pump_controller_run_dose(PUMP_PH_UP, dose);
            ESP_LOGI(TAG, "pH UP: %.2f ml (current=%.2f, target=%.2f)", 
                     dose, s_current_ph, s_config->ph_target);
        }
    } else if (s_current_ph > s_config->ph_target + 0.1f) {
        // pH высокий - нужен pH DOWN
        float dose = pid_compute(&s_pid_ph_down, s_current_ph, dt);
        if (dose > 0.1f) {
            // Получение данных PID для события
            pump_event_pid_data_t pid_data;
            pump_events_get_pid_data(&s_pid_ph_down, s_current_ph, &pid_data);
            pid_data.output = dose; // Устанавливаем реальный выход
            
            // Отправка события с реальными данными PID
            pump_events_send_start_event(
                PUMP_PH_DOWN,
                (uint32_t)((dose / 2.0f) * 1000.0f), // Примерная длительность
                dose,
                &pid_data,
                s_current_ph, s_current_ec,
                s_config->ph_target, s_config->ec_target,
                s_emergency_mode, s_autonomous_mode,
                rssi
            );
            
            pump_controller_run_dose(PUMP_PH_DOWN, dose);
            ESP_LOGI(TAG, "pH DOWN: %.2f ml (current=%.2f, target=%.2f)",
                     dose, s_current_ph, s_config->ph_target);
        }
    }
    
    // Управление EC
    if (s_current_ec < s_config->ec_target - 0.1f) {
        // EC низкая - нужны удобрения
        float dose = pid_compute(&s_pid_ec, s_current_ec, dt);
        if (dose > 0.1f) {
            // Получение данных PID для события
            pump_event_pid_data_t pid_data;
            pump_events_get_pid_data(&s_pid_ec, s_current_ec, &pid_data);
            pid_data.output = dose; // Устанавливаем реальный выход
            
            // Отправка события с реальными данными PID
            pump_events_send_start_event(
                PUMP_EC_A,
                (uint32_t)((dose / 2.0f) * 1000.0f), // Примерная длительность
                dose,
                &pid_data,
                s_current_ph, s_current_ec,
                s_config->ph_target, s_config->ec_target,
                s_emergency_mode, s_autonomous_mode,
                rssi
            );
            
            // Распределяем между A, B, C (упрощённо - только A)
            pump_controller_run_dose(PUMP_EC_A, dose);
            ESP_LOGI(TAG, "EC A: %.2f ml (current=%.2f, target=%.2f)",
                     dose, s_current_ec, s_config->ec_target);
        }
    }
}

// Получение RSSI к родительскому узлу
static int8_t get_rssi_to_parent(void) {
    return mesh_manager_get_parent_rssi();
}

// Отправка discovery сообщения
static void send_discovery(void) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, discovery skipped");
        return;
    }
    
    uint8_t mac[6];
    mesh_manager_get_mac(mac);
    
    uint32_t heap_free = esp_get_free_heap_size();
    int8_t rssi = get_rssi_to_parent();
    
    const char *mesh_id_field = zone_config_validate(s_mesh_network_id)
                                     ? s_mesh_network_id
                                     : ZONE_CONFIG_UNCONFIGURED;
    const char *root_id_field = (s_root_node_id[0] != '\0')
                                     ? s_root_node_id
                                     : ZONE_CONFIG_UNCONFIGURED;

    char discovery_msg[768];
    snprintf(discovery_msg, sizeof(discovery_msg),
            "{\"type\":\"discovery\","
            "\"node_id\":\"%s\","
            "\"node_type\":\"ph_ec\","
            "\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
            "\"firmware\":\"1.0.0\","
            "\"hardware\":\"ESP32-S3\","
            "\"actuators\":[\"pump_ph_up\",\"pump_ph_down\",\"pump_ec_a\",\"pump_ec_b\",\"pump_ec_c\"],"
            "\"heap_free\":%lu,"
            "\"wifi_rssi\":%d,"
            "\"mesh_network_id\":\"%s\","
            "\"root_node_id\":\"%s\"}",
            s_config->base.node_id,
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
            (unsigned long)heap_free,
            rssi,
            mesh_id_field,
            root_id_field);
    
    esp_err_t err = mesh_manager_send_to_root((uint8_t *)discovery_msg, strlen(discovery_msg));
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "🔍 Discovery sent: %s (RSSI=%d)", s_config->base.node_id, rssi);
    } else {
        ESP_LOGW(TAG, "Failed to send discovery: %s", esp_err_to_name(err));
    }
}

// Отправка telemetry
static void send_telemetry(void) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, telemetry skipped");
        return;
    }
    
    int8_t rssi = get_rssi_to_parent();
    
    // Получение статистики насосов
    float pump_ml[PUMP_MAX];
    for (int i = 0; i < PUMP_MAX; i++) {
        pump_ml[i] = pump_controller_get_total_ml((pump_id_t)i);
    }
    
    const char *mesh_id_field = zone_config_validate(s_mesh_network_id)
                                     ? s_mesh_network_id
                                     : ZONE_CONFIG_UNCONFIGURED;
    const char *root_id_field = (s_root_node_id[0] != '\0')
                                     ? s_root_node_id
                                     : ZONE_CONFIG_UNCONFIGURED;

    // Создание JSON telemetry с node_type
    char telemetry_msg[1024];
    snprintf(telemetry_msg, sizeof(telemetry_msg),
            "{\"type\":\"telemetry\","
            "\"node_id\":\"%s\","
            "\"node_type\":\"ph_ec\","
            "\"mesh_network_id\":\"%s\","
            "\"root_node_id\":\"%s\","
            "\"timestamp\":%lu,"
            "\"data\":{"
            "\"ph\":%.2f,"
            "\"ec\":%.2f,"
            "\"temperature\":%.2f,"
            "\"ph_target\":%.2f,"
            "\"ec_target\":%.2f,"
            "\"pump_ph_up_ml\":%.1f,"
            "\"pump_ph_down_ml\":%.1f,"
            "\"pump_ec_a_ml\":%.1f,"
            "\"pump_ec_b_ml\":%.1f,"
            "\"pump_ec_c_ml\":%.1f,"
            "\"mode\":\"%s\","
            "\"emergency\":%s,"
            "\"autonomous\":%s,"
            "\"rssi_to_parent\":%d}}",
            s_config->base.node_id,
            mesh_id_field,
            root_id_field,
            (unsigned long)time(NULL),
            s_current_ph,
            s_current_ec,
            22.5f,  // TODO: реальная температура
            s_config->ph_target,
            s_config->ec_target,
            pump_ml[0], pump_ml[1], pump_ml[2], pump_ml[3], pump_ml[4],
            s_autonomous_mode ? "autonomous" : "online",
            s_emergency_mode ? "true" : "false",
            s_autonomous_mode ? "true" : "false",
            rssi);
    
    esp_err_t err = mesh_manager_send_to_root((uint8_t *)telemetry_msg, strlen(telemetry_msg));
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "📊 Telemetry: pH=%.2f, EC=%.2f, RSSI=%d", 
                 s_current_ph, s_current_ec, rssi);
    } else {
        ESP_LOGW(TAG, "Failed to send telemetry");
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
    
    const char *mesh_id_field = zone_config_validate(s_mesh_network_id)
                                     ? s_mesh_network_id
                                     : ZONE_CONFIG_UNCONFIGURED;
    const char *root_id_field = (s_root_node_id[0] != '\0')
                                     ? s_root_node_id
                                     : ZONE_CONFIG_UNCONFIGURED;

    char heartbeat_msg[384];
    snprintf(heartbeat_msg, sizeof(heartbeat_msg),
            "{\"type\":\"heartbeat\","
            "\"node_id\":\"%s\","
            "\"node_type\":\"ph_ec\","
            "\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
            "\"mesh_network_id\":\"%s\","
            "\"root_node_id\":\"%s\","
            "\"uptime\":%lu,"
            "\"heap_free\":%lu,"
            "\"rssi_to_parent\":%d}",
            s_config->base.node_id,
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
            mesh_id_field,
            root_id_field,
            (unsigned long)uptime,
            (unsigned long)heap_free,
            rssi);
    
    esp_err_t err = mesh_manager_send_to_root((uint8_t *)heartbeat_msg, strlen(heartbeat_msg));
    
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "💓 Heartbeat sent (uptime=%lus, heap=%luB, RSSI=%d)",
                 (unsigned long)uptime, (unsigned long)heap_free, rssi);
    }
}

// Обработка команд
void ph_ec_manager_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "Command received: %s", command);

    if (strcmp(command, "write_config") == 0) {
        if (!params) {
            ESP_LOGE(TAG, "write_config without params");
            return;
        }

        cJSON *node_id_item = cJSON_GetObjectItem(params, "node_id");
        cJSON *mesh_id_item = cJSON_GetObjectItem(params, "mesh_network_id");
        if (!mesh_id_item) {
            mesh_id_item = cJSON_GetObjectItem(params, "mesh_id");
        }
        if (!cJSON_IsString(node_id_item) || !cJSON_IsString(mesh_id_item)) {
            ESP_LOGE(TAG, "write_config missing node_id or mesh_network_id");
            return;
        }

        const char *node_id = node_id_item->valuestring;
        const char *mesh_id = mesh_id_item->valuestring;

        cJSON *root_id_item = cJSON_GetObjectItem(params, "root_node_id");
        const char *root_id = (cJSON_IsString(root_id_item) && root_id_item->valuestring[0] != '\0')
                                  ? root_id_item->valuestring
                                  : node_id;

        if (!zone_config_validate(mesh_id) || !zone_config_validate(root_id)) {
            ESP_LOGE(TAG, "Invalid mesh_id (%s) or root_id (%s)", mesh_id, root_id);
            return;
        }

        strncpy(s_config->base.node_id, node_id, sizeof(s_config->base.node_id) - 1);
        s_config->base.node_id[sizeof(s_config->base.node_id) - 1] = '\0';

        cJSON *zone_item = cJSON_GetObjectItem(params, "zone");
        if (cJSON_IsString(zone_item) && zone_item->valuestring[0] != '\0') {
            strncpy(s_config->base.zone, zone_item->valuestring, sizeof(s_config->base.zone) - 1);
            s_config->base.zone[sizeof(s_config->base.zone) - 1] = '\0';
        }

        strncpy(s_mesh_network_id, mesh_id, sizeof(s_mesh_network_id) - 1);
        s_mesh_network_id[sizeof(s_mesh_network_id) - 1] = '\0';
        strncpy(s_root_node_id, root_id, sizeof(s_root_node_id) - 1);
        s_root_node_id[sizeof(s_root_node_id) - 1] = '\0';

        esp_err_t zone_err = zone_config_save(mesh_id, root_id);
        if (zone_err != ESP_OK) {
            ESP_LOGE(TAG, "zone_config_save failed: %s", esp_err_to_name(zone_err));
            return;
        }

        node_config_set_mesh_network_id(mesh_id);
        node_config_set_root_node_id(root_id);

        cJSON *config_obj = cJSON_GetObjectItem(params, "config");
        if (cJSON_IsObject(config_obj)) {
            ph_ec_manager_handle_config_update(config_obj);
        } else {
            node_config_save(s_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
        }

        node_config_mark_configured(true);
        s_discovery_sent = false;
        ph_ec_manager_send_config_response();
        return;
    }
    
    if (strcmp(command, "set_ph_target") == 0) {
        cJSON *value = cJSON_GetObjectItem(params, "value");
        if (value && cJSON_IsNumber(value)) {
            s_config->ph_target = (float)value->valuedouble;
            pid_set_setpoint(&s_pid_ph_up, s_config->ph_target);
            pid_set_setpoint(&s_pid_ph_down, s_config->ph_target);
            node_config_save(s_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
            ESP_LOGI(TAG, "pH target updated: %.2f", s_config->ph_target);
        }
    } else if (strcmp(command, "set_ec_target") == 0) {
        cJSON *value = cJSON_GetObjectItem(params, "value");
        if (value && cJSON_IsNumber(value)) {
            s_config->ec_target = (float)value->valuedouble;
            pid_set_setpoint(&s_pid_ec, s_config->ec_target);
            node_config_save(s_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
            ESP_LOGI(TAG, "EC target updated: %.2f", s_config->ec_target);
        }
    } else if (strcmp(command, "set_safety_settings") == 0 || strcmp(command, "update_safety_config") == 0) {
        // Safety параметры
        cJSON *max_pump_time = cJSON_GetObjectItem(params, "max_pump_time_ms");
        cJSON *cooldown = cJSON_GetObjectItem(params, "cooldown_ms");
        cJSON *max_daily_volume = cJSON_GetObjectItem(params, "max_daily_volume_ml");
        
        // Emergency пороги
        cJSON *ph_emergency_low = cJSON_GetObjectItem(params, "ph_emergency_low");
        cJSON *ph_emergency_high = cJSON_GetObjectItem(params, "ph_emergency_high");
        cJSON *ec_emergency_high = cJSON_GetObjectItem(params, "ec_emergency_high");
        
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
        if (cJSON_IsNumber(ec_emergency_high)) {
            s_config->ec_emergency_high = (float)ec_emergency_high->valuedouble;
        }
        
        // Сохраняем в NVS
        esp_err_t err = node_config_save(s_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Safety settings updated: max_pump=%lu ms, cooldown=%lu ms, daily_vol=%lu ml, ph_emergency=%.2f-%.2f, ec_emergency=%.2f", 
                     s_config->max_pump_time_ms, s_config->cooldown_ms, s_config->max_daily_volume_ml,
                     s_config->ph_emergency_low, s_config->ph_emergency_high, s_config->ec_emergency_high);
        } else {
            ESP_LOGE(TAG, "Failed to save safety settings to NVS");
        }
    }
    else if (strcmp(command, "get_config") == 0) {
        // Проверка подключения к mesh
    if (!mesh_manager_is_connected()) {
            ESP_LOGW(TAG, "Cannot send config: mesh offline");
            return;
        }
        
    const char *mesh_id_field = zone_config_validate(s_mesh_network_id)
                                     ? s_mesh_network_id
                                     : ZONE_CONFIG_UNCONFIGURED;
    const char *root_id_field = (s_root_node_id[0] != '\0')
                                     ? s_root_node_id
                                     : ZONE_CONFIG_UNCONFIGURED;

        // Создание корневого объекта
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            ESP_LOGE(TAG, "Failed to create root JSON object");
            return;
        }
        
        cJSON_AddStringToObject(root, "type", "config_response");
        cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);
        cJSON_AddNumberToObject(root, "timestamp", (uint32_t)time(NULL));
    cJSON_AddStringToObject(root, "mesh_network_id", mesh_id_field);
    cJSON_AddStringToObject(root, "root_node_id", root_id_field);
        
        // Создание объекта конфигурации
        cJSON *config = cJSON_CreateObject();
        if (!config) {
            ESP_LOGE(TAG, "Failed to create config JSON object");
            cJSON_Delete(root);
            return;
        }
        
        cJSON_AddStringToObject(config, "node_id", s_config->base.node_id);
        cJSON_AddStringToObject(config, "node_type", "ph_ec");
    cJSON_AddStringToObject(config, "zone", s_config->base.zone);
    cJSON_AddStringToObject(config, "mesh_network_id", mesh_id_field);
    cJSON_AddStringToObject(config, "root_node_id", root_id_field);
        
        // pH параметры
        cJSON_AddNumberToObject(config, "ph_target", s_config->ph_target);
        cJSON_AddNumberToObject(config, "ph_min", s_config->ph_min);
        cJSON_AddNumberToObject(config, "ph_max", s_config->ph_max);
        cJSON_AddNumberToObject(config, "ph_cal_offset", s_config->ph_cal_offset);
        
        // EC параметры
        cJSON_AddNumberToObject(config, "ec_target", s_config->ec_target);
        cJSON_AddNumberToObject(config, "ec_min", s_config->ec_min);
        cJSON_AddNumberToObject(config, "ec_max", s_config->ec_max);
        cJSON_AddNumberToObject(config, "ec_cal_offset", s_config->ec_cal_offset);
        
        // Safety параметры
        cJSON_AddNumberToObject(config, "max_pump_time_ms", s_config->max_pump_time_ms);
        cJSON_AddNumberToObject(config, "cooldown_ms", s_config->cooldown_ms);
        cJSON_AddNumberToObject(config, "max_daily_volume_ml", s_config->max_daily_volume_ml);
        
        // Emergency пороги
        cJSON_AddNumberToObject(config, "ph_emergency_low", s_config->ph_emergency_low);
        cJSON_AddNumberToObject(config, "ph_emergency_high", s_config->ph_emergency_high);
        cJSON_AddNumberToObject(config, "ec_emergency_high", s_config->ec_emergency_high);
        
        // Автономия
        cJSON_AddBoolToObject(config, "autonomous_enabled", s_config->autonomous_enabled);
        cJSON_AddNumberToObject(config, "mesh_timeout_ms", s_config->mesh_timeout_ms);
        
        // Добавление config к root
        cJSON_AddItemToObject(root, "config", config);
        
        // Отправка конфигурации
        char json_buffer[2048];
        char *json_string = cJSON_PrintUnformatted(root);
        if (json_string) {
            if (strlen(json_string) < sizeof(json_buffer)) {
                strcpy(json_buffer, json_string);
                mesh_manager_send_to_root((uint8_t*)json_buffer, strlen(json_buffer));
                ESP_LOGI(TAG, "Config sent to server");
            } else {
                ESP_LOGE(TAG, "Config JSON too large");
            }
            free(json_string);
        }
        
        cJSON_Delete(root);
    }
}

// Обработка обновления конфигурации
void ph_ec_manager_handle_config_update(cJSON *config_json) {
    ESP_LOGI(TAG, "Config update received");
    
    node_config_update_from_json(s_config, config_json, "ph_ec");
    node_config_save(s_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
    
    ESP_LOGI(TAG, "Config updated and saved");
}

