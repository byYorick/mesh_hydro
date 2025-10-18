/**
 * @file node_controller.c
 * @brief Реализация главной логики NODE pH/EC
 */

#include "node_controller.h"
#include "connection_monitor.h"
#include "local_storage.h"
#include "oled_display.h"
#include "buzzer_led.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>

static const char *TAG = "node_controller";

static ph_ec_node_config_t *s_config = NULL;
static TaskHandle_t s_main_task = NULL;
static bool s_autonomous_mode = false;

// Forward declarations
static void node_controller_main_task(void *arg);
static void send_telemetry(float ph, float ec, float temp);
static void on_connection_state_changed(connection_state_t new_state, connection_state_t old_state);

// TODO: Эти функции будут использовать sensor_manager, pump_manager, adaptive_pid
static float read_ph_sensor(void);
static float read_ec_sensor(void);
static float read_temp_sensor(void);
static void run_pid_control(float ph, float ec);

esp_err_t node_controller_init(ph_ec_node_config_t *config) {
    if (!config) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    s_config = config;

    // Регистрация callback изменения состояния связи
    connection_monitor_register_state_cb(on_connection_state_changed);

    ESP_LOGI(TAG, "Node Controller initialized");
    ESP_LOGI(TAG, "Node ID: %s, Zone: %s", s_config->base.node_id, s_config->base.zone);
    ESP_LOGI(TAG, "pH target: %.2f, EC target: %.2f", s_config->ph_target, s_config->ec_target);
    ESP_LOGI(TAG, "Autonomous mode: %s", s_config->autonomous_enabled ? "ENABLED" : "DISABLED");

    return ESP_OK;
}

esp_err_t node_controller_start(void) {
    if (s_main_task != NULL) {
        ESP_LOGW(TAG, "Main task already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(node_controller_main_task,
                                  "node_main",
                                  8192,
                                  NULL,
                                  6,
                                  &s_main_task);
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Node Controller started");
    return ESP_OK;
}

esp_err_t node_controller_stop(void) {
    if (s_main_task != NULL) {
        vTaskDelete(s_main_task);
        s_main_task = NULL;
        ESP_LOGI(TAG, "Node Controller stopped");
    }
    return ESP_OK;
}

// Главная задача узла
static void node_controller_main_task(void *arg) {
    ESP_LOGI(TAG, "Main task running (cycle: 10 seconds)");
    
    // Регистрация в watchdog
    esp_task_wdt_add(NULL);

    while (1) {
        // Сброс watchdog
        esp_task_wdt_reset();

        // 1. Чтение датчиков
        float ph = read_ph_sensor();
        float ec = read_ec_sensor();
        float temp = read_temp_sensor();

        // 2. Обновление OLED
        connection_state_t conn_state = connection_monitor_get_state();
        oled_display_update_main(ph, ec, temp, conn_state);

        // 3. PID управление (ВСЕГДА работает, даже в автономном режиме!)
        run_pid_control(ph, ec);

        // 4. Отправка телеметрии или буферизация
        if (conn_state == CONN_STATE_ONLINE || conn_state == CONN_STATE_DEGRADED) {
            send_telemetry(ph, ec, temp);
            
            // Синхронизация буфера если есть несинхронизированные данные
            int unsynced = local_storage_get_unsynced_count();
            if (unsynced > 0) {
                ESP_LOGI(TAG, "Syncing buffer: %d entries", unsynced);
                // TODO: Отправка батчами
            }
        } else {
            // Автономный режим - сохранение в локальный буфер
            local_storage_add(ph, ec, temp);
        }

        // 5. Проверка аварийных ситуаций
        if (ph < s_config->ph_emergency_low) {
            node_controller_handle_emergency("pH TOO LOW", ph);
        } else if (ph > s_config->ph_emergency_high) {
            node_controller_handle_emergency("pH TOO HIGH", ph);
        } else if (ec > s_config->ec_emergency_high) {
            node_controller_handle_emergency("EC TOO HIGH", ec);
        }

        // Цикл каждые 10 секунд
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// Отправка телеметрии на ROOT
static void send_telemetry(float ph, float ec, float temp) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh not connected, telemetry skipped");
        return;
    }

    // Создание JSON
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "ph", ph);
    cJSON_AddNumberToObject(data, "ec", ec);
    cJSON_AddNumberToObject(data, "temp", temp);

    char json_buf[512];
    if (mesh_protocol_create_telemetry(s_config->base.node_id, data,
                                        json_buf, sizeof(json_buf))) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
        
        if (err == ESP_OK) {
            connection_monitor_mark_root_contact();  // Отметка контакта
            ESP_LOGD(TAG, "Telemetry sent");
        } else {
            ESP_LOGW(TAG, "Failed to send telemetry: %s", esp_err_to_name(err));
        }
    }

    cJSON_Delete(data);
}

// Callback при изменении состояния связи
static void on_connection_state_changed(connection_state_t new_state, connection_state_t old_state) {
    ESP_LOGI(TAG, "Connection state changed: %d → %d", old_state, new_state);

    switch (new_state) {
        case CONN_STATE_ONLINE:
            buzzer_led_set_mode(LED_MODE_GREEN);
            oled_display_show_message("ONLINE");
            if (old_state == CONN_STATE_AUTONOMOUS) {
                ESP_LOGI(TAG, "Exiting autonomous mode");
                node_controller_exit_autonomous();
            }
            break;

        case CONN_STATE_DEGRADED:
            buzzer_led_set_mode(LED_MODE_YELLOW_BLINK);
            oled_display_show_message("DEGRADED");
            break;

        case CONN_STATE_AUTONOMOUS:
            buzzer_led_set_mode(LED_MODE_YELLOW_BLINK);
            oled_display_show_message("AUTONOMOUS");
            ESP_LOGW(TAG, "Entering autonomous mode");
            node_controller_enter_autonomous();
            buzzer_beep(2, 100, 200);  // 2 сигнала
            break;

        case CONN_STATE_EMERGENCY:
            buzzer_led_set_mode(LED_MODE_RED_BLINK_FAST);
            buzzer_alarm();
            break;
    }
}

void node_controller_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "Command received: %s", command);

    // TODO: Обработка команд
    // - "set_target" - установка pH/EC целевых значений
    // - "run_pump" - ручной запуск насоса
    // - "calibrate" - калибровка датчиков
    // - "reset_stats" - сброс статистики
}

void node_controller_handle_config_update(cJSON *config_json) {
    ESP_LOGI(TAG, "Config update received");

    // Обновление конфигурации
    node_config_update_from_json(s_config, config_json, "ph_ec");
    
    // Сохранение в NVS
    node_config_save(s_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
    
    ESP_LOGI(TAG, "Config updated and saved: pH=%.2f, EC=%.2f",
             s_config->ph_target, s_config->ec_target);
    
    oled_display_show_message("Config Updated");
}

void node_controller_enter_autonomous(void) {
    s_autonomous_mode = true;
    ESP_LOGW(TAG, "Autonomous mode ENABLED");
    
    // Загрузка последней конфигурации из NVS (на случай если не загружена)
    node_config_load(s_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
}

void node_controller_exit_autonomous(void) {
    s_autonomous_mode = false;
    ESP_LOGI(TAG, "Autonomous mode DISABLED");
}

void node_controller_handle_emergency(const char *message, float value) {
    ESP_LOGE(TAG, "EMERGENCY: %s (value: %.2f)", message, value);

    // Визуальная индикация
    buzzer_led_set_mode(LED_MODE_RED_BLINK_FAST);
    buzzer_alarm();
    oled_display_show_emergency(message, value);

    // TODO: Агрессивная коррекция
    // TODO: Отправка SOS на ROOT (если online)
}

// TODO: Заглушки для функций датчиков и PID
// Эти функции будут реализованы в sensor_manager, pump_manager, adaptive_pid

static float read_ph_sensor(void) {
    // TODO: sensor_manager_read_ph()
    return 6.5;  // Заглушка
}

static float read_ec_sensor(void) {
    // TODO: sensor_manager_read_ec()
    return 1.8;  // Заглушка
}

static float read_temp_sensor(void) {
    // TODO: sensor_manager_read_temp()
    return 24.0;  // Заглушка
}

static void run_pid_control(float ph, float ec) {
    // TODO: adaptive_pid_update()
    ESP_LOGD(TAG, "PID control: pH=%.2f (target=%.2f), EC=%.2f (target=%.2f)",
             ph, s_config->ph_target, ec, s_config->ec_target);
}

