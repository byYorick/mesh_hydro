/**
 * @file app_main.c
 * @brief NODE pH/EC - критичный узел с автономной работой
 * 
 * ESP32-S3 #3 - pH/EC Control + Autonomous Mode
 * 
 * КРИТИЧНО:
 * - ВСЕГДА автономная работа
 * - ВСЕГДА сохранение в NVS
 * - ВСЕГДА watchdog reset
 * - НИКОГДА не блокировать PID
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

// NODE pH/EC компоненты
#include "node_controller.h"
#include "connection_monitor.h"
#include "local_storage.h"
#include "oled_display.h"
#include "buzzer_led.h"
#include "sensor_manager.h"
#include "pump_manager.h"
#include "adaptive_pid.h"

static const char *TAG = "PH_EC";

// Глобальная конфигурация
static ph_ec_node_config_t g_config;

// GPIO пины
#define GPIO_LED        15
#define GPIO_BUZZER     16
#define GPIO_BUTTON     19

#define GPIO_I2C_SDA    18
#define GPIO_I2C_SCL    17

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

    // Отметка контакта с ROOT
    connection_monitor_mark_root_contact();

    // Обработка по типу сообщения
    switch (msg.type) {
        case MESH_MSG_COMMAND: {
            cJSON *cmd = cJSON_GetObjectItem(msg.data, "command");
            if (cmd && cJSON_IsString(cmd)) {
                node_controller_handle_command(cmd->valuestring, msg.data);
            }
            break;
        }

        case MESH_MSG_CONFIG:
            node_controller_handle_config_update(msg.data);
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }

    mesh_protocol_free_message(&msg);
}

/**
 * @brief Callback при нажатии кнопки MODE
 */
static void on_button_press(uint32_t duration_ms) {
    ESP_LOGI(TAG, "Button pressed for %d ms", duration_ms);

    if (duration_ms < 1000) {
        // Короткое нажатие - переключить AUTO/MANUAL
        // TODO: переключение режима
        ESP_LOGI(TAG, "Mode toggle");
        buzzer_beep(1, 50, 0);

    } else if (duration_ms < 3000) {
        // Среднее - сброс Buzzer
        buzzer_stop();
        ESP_LOGI(TAG, "Buzzer stopped");

    } else if (duration_ms < 10000) {
        // Долгое - сброс Emergency
        connection_state_t state = connection_monitor_get_state();
        if (state == CONN_STATE_EMERGENCY) {
            // TODO: Выход из emergency
            ESP_LOGI(TAG, "Emergency reset");
            oled_display_show_message("Emergency Reset");
            buzzer_beep(3, 100, 100);
        }

    } else {
        // Очень долгое (>10 сек) - Factory Reset
        ESP_LOGW(TAG, "Factory Reset initiated!");
        oled_display_show_message("Factory Reset...");
        buzzer_beep(5, 200, 200);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        node_config_erase_all();
        esp_restart();
    }
}

/**
 * @brief Главная функция приложения
 */
void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== NODE pH/EC Starting ===");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3 - Critical Node with Autonomy");
    ESP_LOGI(TAG, "Build: %s %s", __DATE__, __TIME__);

    // === Шаг 1: Инициализация NVS ===
    ESP_LOGI(TAG, "[Step 1/11] Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs erase");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // === Шаг 2: Загрузка конфигурации ===
    ESP_LOGI(TAG, "[Step 2/11] Loading configuration...");
    if (node_config_load(&g_config, sizeof(g_config), "ph_ec_ns") != ESP_OK) {
        ESP_LOGW(TAG, "Config not found, using defaults");
        node_config_reset_to_default(&g_config, "ph_ec");
        strcpy(g_config.base.node_id, "ph_ec_001");
        strcpy(g_config.base.zone, "Zone 1");
        node_config_save(&g_config, sizeof(g_config), "ph_ec_ns");
    }
    ESP_LOGI(TAG, "Loaded: %s (%s)", g_config.base.node_id, g_config.base.zone);
    ESP_LOGI(TAG, "Targets: pH=%.2f, EC=%.2f", g_config.ph_target, g_config.ec_target);

    // === Шаг 3: Инициализация индикации ===
    ESP_LOGI(TAG, "[Step 3/11] Initializing LED/Buzzer/Button...");
    ESP_ERROR_CHECK(buzzer_led_init(GPIO_LED, GPIO_BUZZER, GPIO_BUTTON));
    buzzer_led_register_button_cb(on_button_press);
    buzzer_led_set_mode(LED_MODE_YELLOW_BLINK);  // Старт
    buzzer_beep(1, 100, 0);  // 1 сигнал при старте

    // === Шаг 4: Инициализация OLED ===
    ESP_LOGI(TAG, "[Step 4/11] Initializing OLED Display...");
    ESP_ERROR_CHECK(oled_display_init(I2C_NUM_0, GPIO_I2C_SDA, GPIO_I2C_SCL));
    oled_display_show_message("Starting...");

    // === Шаг 5: Инициализация датчиков ===
    ESP_LOGI(TAG, "[Step 5/11] Initializing Sensors...");
    ESP_ERROR_CHECK(sensor_manager_init());

    // === Шаг 6: Инициализация насосов ===
    ESP_LOGI(TAG, "[Step 6/11] Initializing Pumps...");
    ESP_ERROR_CHECK(pump_manager_init());

    // === Шаг 7: Инициализация PID ===
    ESP_LOGI(TAG, "[Step 7/11] Initializing Adaptive PID...");
    ESP_ERROR_CHECK(adaptive_pid_init(&g_config));

    // === Шаг 8: Инициализация локального хранилища ===
    ESP_LOGI(TAG, "[Step 8/11] Initializing Local Storage...");
    ESP_ERROR_CHECK(local_storage_init());

    // === Шаг 9: Инициализация мониторинга связи ===
    ESP_LOGI(TAG, "[Step 9/11] Initializing Connection Monitor...");
    ESP_ERROR_CHECK(connection_monitor_init());
    ESP_ERROR_CHECK(connection_monitor_start());

    // === Шаг 10: Инициализация Mesh (NODE режим) ===
    ESP_LOGI(TAG, "[Step 10/11] Initializing Mesh (NODE mode)...");
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .mesh_password = "hydro_mesh_2025",
        .channel = 1
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());

    // === Шаг 11: Запуск главного контроллера ===
    ESP_LOGI(TAG, "[Step 11/11] Starting Node Controller...");
    ESP_ERROR_CHECK(node_controller_init(&g_config));
    ESP_ERROR_CHECK(node_controller_start());

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== NODE pH/EC Running ===");
    ESP_LOGI(TAG, "Node ID: %s", g_config.base.node_id);
    ESP_LOGI(TAG, "Autonomous: %s", g_config.autonomous_enabled ? "ENABLED" : "DISABLED");
    ESP_LOGI(TAG, "========================================");

    // LED зеленый - готов к работе
    buzzer_led_set_mode(LED_MODE_GREEN);
    oled_display_show_message("Ready!");

    ESP_LOGI(TAG, "All systems operational. Node ready.");
    ESP_LOGI(TAG, "PID control cycle: 10 seconds");
    ESP_LOGI(TAG, "Autonomous mode will activate if ROOT offline > 30 sec");
}
