/**
 * @file app_main.c
 * @brief NODE Display - TFT экран с LVGL для мониторинга mesh
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"

#include "../../common/mesh_manager/mesh_manager.h"
#include "../../common/mesh_protocol/mesh_protocol.h"
#include "../../common/node_config/node_config.h"
#include "../../common/mesh_config/mesh_config.h"

static const char *TAG = "DISPLAY";

// ════════════════════════════════════════════════════════
// КОНФИГУРАЦИЯ
// ════════════════════════════════════════════════════════

// GPIO для TFT (из hydro1.0)
#define LCD_PIN_MOSI    11
#define LCD_PIN_SCLK    12
#define LCD_PIN_CS      10
#define LCD_PIN_DC      9
#define LCD_PIN_RST     14
#define LCD_PIN_BCKL    15

// GPIO для энкодера
#define ENCODER_PIN_CLK 4
#define ENCODER_PIN_DT  5
#define ENCODER_PIN_SW  6

// Таймеры
#define REQUEST_INTERVAL_MS    5000   // Запрос данных каждые 5 сек
#define HEARTBEAT_INTERVAL_MS  10000  // Heartbeat каждые 10 сек

// ════════════════════════════════════════════════════════
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ════════════════════════════════════════════════════════

typedef struct {
    base_node_config_t base;
    uint8_t backlight_brightness;  // 0-100%
    uint32_t request_interval_ms;
    uint32_t heartbeat_interval_ms;
} display_node_config_t;

static display_node_config_t s_config;
static uint32_t s_boot_time = 0;

// Кэш данных узлов (последний RESPONSE от ROOT)
typedef struct {
    char node_id[32];
    char node_type[16];
    bool online;
    uint64_t last_update_ms;
    char data_json[512];  // Последние данные (сырой JSON)
} cached_node_t;

#define MAX_CACHED_NODES 10
static cached_node_t s_nodes_cache[MAX_CACHED_NODES];
static int s_cache_count = 0;
static SemaphoreHandle_t s_cache_mutex;

// ════════════════════════════════════════════════════════
// ПРОТОТИПЫ ФУНКЦИЙ
// ════════════════════════════════════════════════════════

static void init_default_config(void);
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len);
static void request_task(void *arg);
static void heartbeat_task(void *arg);
static void display_task(void *arg);
static void send_request_all_nodes(void);
static void send_heartbeat(void);
static void update_cache(cJSON *nodes_array);
static int8_t get_rssi_to_parent(void);

// ════════════════════════════════════════════════════════
// ИНИЦИАЛИЗАЦИЯ КОНФИГУРАЦИИ
// ════════════════════════════════════════════════════════

static void init_default_config(void) {
    // Базовая конфигурация
    strcpy(s_config.base.node_id, "display_001");
    strcpy(s_config.base.zone, "Main Interface");
    s_config.base.type = NODE_TYPE_DISPLAY;
    s_config.base.enabled = true;
    
    // Специфичная для Display
    s_config.backlight_brightness = 80;  // 80% яркость
    s_config.request_interval_ms = REQUEST_INTERVAL_MS;
    s_config.heartbeat_interval_ms = HEARTBEAT_INTERVAL_MS;
    
    ESP_LOGI(TAG, "Default config initialized: %s (%s)", 
             s_config.base.node_id, s_config.base.zone);
}

// ════════════════════════════════════════════════════════
// MESH CALLBACK
// ════════════════════════════════════════════════════════

static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    ESP_LOGI(TAG, "📥 Mesh data received: %d bytes from "MACSTR, len, MAC2STR(src));
    
    // Создаём NULL-terminated копию
    char *data_copy = malloc(len + 1);
    if (!data_copy) {
        ESP_LOGE(TAG, "Failed to allocate memory for data copy");
        return;
    }
    memcpy(data_copy, data, len);
    data_copy[len] = '\0';
    
    ESP_LOGI(TAG, "   Data: %s", data_copy);
    
    // Парсинг сообщения
    mesh_message_t msg;
    if (!mesh_protocol_parse(data_copy, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        free(data_copy);
        return;
    }
    
    // Проверка адресата
    if (strcmp(msg.node_id, s_config.base.node_id) != 0) {
        ESP_LOGD(TAG, "Message not for us (for %s)", msg.node_id);
        free(data_copy);
        mesh_protocol_free_message(&msg);
        return;
    }
    
    ESP_LOGI(TAG, "✅ Message parsed: type=%d, node_id=%s", msg.type, msg.node_id);
    
    switch (msg.type) {
        case MESH_MSG_RESPONSE: {
            ESP_LOGI(TAG, "📥 RESPONSE from ROOT received!");
            
            // Получение массива узлов
            cJSON *nodes = cJSON_GetObjectItem(msg.data, "nodes");
            if (nodes && cJSON_IsArray(nodes)) {
                int node_count = cJSON_GetArraySize(nodes);
                ESP_LOGI(TAG, "   Response contains %d nodes", node_count);
                
                // Обновление кэша
                update_cache(nodes);
                
                // TODO: Обновление UI
                ESP_LOGI(TAG, "   Cache updated (%d nodes)", s_cache_count);
            } else {
                ESP_LOGW(TAG, "   Response has no 'nodes' array!");
            }
            break;
        }
        
        case MESH_MSG_CONFIG: {
            ESP_LOGI(TAG, "📥 CONFIG update from ROOT");
            // TODO: Обновление конфигурации
            break;
        }
        
        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }
    
    free(data_copy);
    mesh_protocol_free_message(&msg);
}

// ════════════════════════════════════════════════════════
// ОБНОВЛЕНИЕ КЭША УЗЛОВ
// ════════════════════════════════════════════════════════

static void update_cache(cJSON *nodes_array) {
    xSemaphoreTake(s_cache_mutex, portMAX_DELAY);
    
    // Очистка старого кэша
    s_cache_count = 0;
    memset(s_nodes_cache, 0, sizeof(s_nodes_cache));
    
    // Заполнение нового кэша
    int array_size = cJSON_GetArraySize(nodes_array);
    for (int i = 0; i < array_size && i < MAX_CACHED_NODES; i++) {
        cJSON *node = cJSON_GetArrayItem(nodes_array, i);
        
        cJSON *node_id = cJSON_GetObjectItem(node, "node_id");
        cJSON *node_type = cJSON_GetObjectItem(node, "node_type");
        cJSON *online = cJSON_GetObjectItem(node, "online");
        cJSON *data = cJSON_GetObjectItem(node, "data");
        
        if (node_id && cJSON_IsString(node_id)) {
            strncpy(s_nodes_cache[i].node_id, node_id->valuestring, 31);
            
            if (node_type && cJSON_IsString(node_type)) {
                strncpy(s_nodes_cache[i].node_type, node_type->valuestring, 15);
            }
            
            s_nodes_cache[i].online = online ? cJSON_IsTrue(online) : false;
            s_nodes_cache[i].last_update_ms = esp_timer_get_time() / 1000;
            
            if (data) {
                char *data_str = cJSON_PrintUnformatted(data);
                if (data_str) {
                    strncpy(s_nodes_cache[i].data_json, data_str, 511);
                    free(data_str);
                }
            }
            
            s_cache_count++;
            
            ESP_LOGI(TAG, "   [%d] %s (%s) - %s", 
                     i, s_nodes_cache[i].node_id, s_nodes_cache[i].node_type,
                     s_nodes_cache[i].online ? "ONLINE" : "OFFLINE");
        }
    }
    
    xSemaphoreGive(s_cache_mutex);
}

// ════════════════════════════════════════════════════════
// ЗАПРОС ДАННЫХ У ROOT
// ════════════════════════════════════════════════════════

static void send_request_all_nodes(void) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, request skipped");
        return;
    }
    
    // Создание REQUEST JSON
    char json_buf[256];
    if (mesh_protocol_create_request(s_config.base.node_id, "all_nodes_data",
                                      json_buf, sizeof(json_buf))) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
        
        if (err == ESP_OK) {
            ESP_LOGD(TAG, "🔍 Data request sent to ROOT");
        } else {
            ESP_LOGW(TAG, "Failed to send request: %s", esp_err_to_name(err));
        }
    }
}

// ════════════════════════════════════════════════════════
// HEARTBEAT
// ════════════════════════════════════════════════════════

static int8_t get_rssi_to_parent(void) {
    return mesh_manager_get_parent_rssi();
}

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
    
    // Создание heartbeat JSON с MAC адресом
    char heartbeat_msg[384];
    snprintf(heartbeat_msg, sizeof(heartbeat_msg),
            "{\"type\":\"heartbeat\","
            "\"node_id\":\"%s\","
            "\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
            "\"uptime\":%lu,"
            "\"heap_free\":%lu,"
            "\"rssi_to_parent\":%d}",
            s_config.base.node_id,
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
            (unsigned long)uptime,
            (unsigned long)heap_free,
            rssi);
    
    esp_err_t err = mesh_manager_send_to_root((uint8_t *)heartbeat_msg, strlen(heartbeat_msg));
    
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "💓 Heartbeat sent (uptime=%lus, heap=%luB, RSSI=%d)", 
                 (unsigned long)uptime, (unsigned long)heap_free, rssi);
    }
}

// ════════════════════════════════════════════════════════
// ЗАДАЧА ЗАПРОСОВ (каждые 5 сек)
// ════════════════════════════════════════════════════════

static void request_task(void *arg) {
    ESP_LOGI(TAG, "Request task running (every 5 sec)");
    
    // Ожидание подключения к mesh
    vTaskDelay(pdMS_TO_TICKS(10000));
    
    while (1) {
        if (mesh_manager_is_connected()) {
            send_request_all_nodes();
        } else {
            ESP_LOGW(TAG, "⚠️ Mesh offline - waiting for connection");
        }
        
        vTaskDelay(pdMS_TO_TICKS(s_config.request_interval_ms));
    }
}

// ════════════════════════════════════════════════════════
// ЗАДАЧА HEARTBEAT (каждые 10 сек)
// ════════════════════════════════════════════════════════

static void heartbeat_task(void *arg) {
    ESP_LOGI(TAG, "Heartbeat task running (every 10 sec)");
    
    vTaskDelay(pdMS_TO_TICKS(10000));  // Начальная задержка
    
    while (1) {
        if (mesh_manager_is_connected()) {
            send_heartbeat();
        }
        
        vTaskDelay(pdMS_TO_TICKS(s_config.heartbeat_interval_ms));
    }
}

// ════════════════════════════════════════════════════════
// ЗАДАЧА DISPLAY (обновление экрана)
// ════════════════════════════════════════════════════════

static void display_task(void *arg) {
    ESP_LOGI(TAG, "Display task running (console output every 10 sec)");
    
    vTaskDelay(pdMS_TO_TICKS(15000));  // Начальная задержка
    
    while (1) {
        // ═══════════════════════════════════════════════════════════
        // КОНСОЛЬНЫЙ DASHBOARD (пока нет LCD)
        // ═══════════════════════════════════════════════════════════
        
        xSemaphoreTake(s_cache_mutex, portMAX_DELAY);
        
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════╗");
        ESP_LOGI(TAG, "║      🌿 HYDRO MESH DASHBOARD (Display)           ║");
        ESP_LOGI(TAG, "╠═══════════════════════════════════════════════════╣");
        ESP_LOGI(TAG, "║  Cached nodes: %d                                 ║", s_cache_count);
        ESP_LOGI(TAG, "╠═══════════════════════════════════════════════════╣");
        
        if (s_cache_count == 0) {
            ESP_LOGI(TAG, "║  ⚠️ No data yet (waiting for RESPONSE...)       ║");
        } else {
            for (int i = 0; i < s_cache_count; i++) {
                ESP_LOGI(TAG, "║                                                   ║");
                ESP_LOGI(TAG, "║  %s %s (%s)", 
                         s_nodes_cache[i].online ? "🟢" : "🔴",
                         s_nodes_cache[i].node_id,
                         s_nodes_cache[i].node_type);
                
                if (strlen(s_nodes_cache[i].data_json) > 0) {
                    ESP_LOGI(TAG, "║     Data: %s", s_nodes_cache[i].data_json);
                }
            }
        }
        
        ESP_LOGI(TAG, "╠═══════════════════════════════════════════════════╣");
        ESP_LOGI(TAG, "║  Heap: %lu bytes                                  ║", 
                 (unsigned long)esp_get_free_heap_size());
        ESP_LOGI(TAG, "║  RSSI to parent: %d dBm                           ║", 
                 get_rssi_to_parent());
        ESP_LOGI(TAG, "╚═══════════════════════════════════════════════════╝");
        ESP_LOGI(TAG, "");
        
        xSemaphoreGive(s_cache_mutex);
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // Каждые 10 секунд
    }
}

// ════════════════════════════════════════════════════════
// ГЛАВНАЯ ФУНКЦИЯ
// ════════════════════════════════════════════════════════

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== NODE Display Starting ===");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3 - TFT 320x240 + LVGL Dashboard");
    ESP_LOGI(TAG, "Build: %s %s", __DATE__, __TIME__);
    
    // === Шаг 1: NVS ===
    ESP_LOGI(TAG, "[Step 1/6] Initializing NVS...");
    ESP_ERROR_CHECK(nvs_flash_init());

    // === Шаг 2: Загрузка конфигурации ===
    ESP_LOGI(TAG, "[Step 2/6] Loading configuration...");
    esp_err_t ret = node_config_load(&s_config, sizeof(s_config), "display_ns");
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Config not found, using defaults");
        init_default_config();
        node_config_save(&s_config, sizeof(s_config), "display_ns");
    }
    
    ESP_LOGI(TAG, "Loaded: %s (%s)", s_config.base.node_id, s_config.base.zone);
    ESP_LOGI(TAG, "Backlight: %d%%", s_config.backlight_brightness);
    
    s_boot_time = (uint32_t)time(NULL);
    
    // === Шаг 3: Создание мьютекса кэша ===
    ESP_LOGI(TAG, "[Step 3/6] Initializing cache...");
    s_cache_mutex = xSemaphoreCreateMutex();
    if (!s_cache_mutex) {
        ESP_LOGE(TAG, "Failed to create cache mutex");
        return;
    }
    ESP_LOGI(TAG, "Cache mutex created");
    
    // === Шаг 4: TODO - LCD init ===
    ESP_LOGI(TAG, "[Step 4/6] Initializing LCD... (TODO)");
    ESP_LOGW(TAG, "  ⚠️ LCD DRIVER NOT IMPLEMENTED YET!");
    ESP_LOGW(TAG, "  ⚠️ Using CONSOLE OUTPUT for now");
    ESP_LOGI(TAG, "  GPIO config: MOSI=%d, SCLK=%d, CS=%d, DC=%d, RST=%d, BCKL=%d",
             LCD_PIN_MOSI, LCD_PIN_SCLK, LCD_PIN_CS, 
             LCD_PIN_DC, LCD_PIN_RST, LCD_PIN_BCKL);
    
    // === Шаг 5: TODO - Encoder init ===
    ESP_LOGI(TAG, "[Step 5/6] Initializing Encoder... (TODO)");
    ESP_LOGW(TAG, "  ⚠️ ENCODER DRIVER NOT IMPLEMENTED YET!");
    ESP_LOGI(TAG, "  GPIO config: CLK=%d, DT=%d, SW=%d",
             ENCODER_PIN_CLK, ENCODER_PIN_DT, ENCODER_PIN_SW);
    
    // === Шаг 6: Инициализация Mesh (NODE режим) ===
    ESP_LOGI(TAG, "[Step 6/6] Initializing Mesh (NODE mode)...");
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
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());
    ESP_LOGI(TAG, "Mesh started");
    
    // === Шаг 7: Запуск задач ===
    ESP_LOGI(TAG, "[Step 7/7] Starting tasks...");
    
    xTaskCreate(request_task, "request", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "  - Request task started");
    
    xTaskCreate(heartbeat_task, "heartbeat", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "  - Heartbeat task started");
    
    xTaskCreate(display_task, "display", 6144, NULL, 6, NULL);
    ESP_LOGI(TAG, "  - Display task started (console dashboard)");
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== NODE Display Running ===");
    ESP_LOGI(TAG, "Node ID: %s", s_config.base.node_id);
    ESP_LOGI(TAG, "Request interval: %lu ms", (unsigned long)s_config.request_interval_ms);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "All systems operational. Monitoring mesh...");
}
