# 🤖 ИНСТРУКЦИЯ ДЛЯ ИИ: ROOT NODE

**Координатор mesh-сети и MQTT мост**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**ROOT NODE (ESP32-S3 #1)** - центральный узел системы, координирующий всю mesh-сеть и связь с сервером.

### ⭐ Ключевые роли:

1. **Mesh координатор** - ROOT узел в ESP-WIFI-MESH
2. **MQTT мост** - связь между mesh-сетью и сервером
3. **Реестр узлов** - учет всех подключенных NODE узлов
4. **Маршрутизатор данных** - NODE ↔ MQTT ↔ Server
5. **Резервная логика** - fallback управление климатом
6. **OTA координатор** - раздача обновлений (TODO)

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ❌ **НЕТ ДАТЧИКОВ!** - ROOT не имеет физических датчиков
2. ❌ **НЕТ ДИСПЛЕЯ!** - работает "headless"
3. ❌ **НЕТ ИСПОЛНИТЕЛЕЙ!** - только координирует, не управляет напрямую
4. ✅ **МАКСИМАЛЬНАЯ СТАБИЛЬНОСТЬ** - от ROOT зависит вся сеть
5. ✅ **БЫСТРАЯ МАРШРУТИЗАЦИЯ** - не блокировать обработку данных
6. ✅ **МОНИТОРИНГ УЗЛОВ** - отслеживание таймаутов и статусов

---

## 🏗️ АРХИТЕКТУРА

```
ROOT NODE (ESP32-S3 #1)
│
├── WiFi/Mesh Layer
│   ├── WiFi STA → Home Router (для интернета)
│   ├── Mesh AP → NODE узлы подключаются сюда
│   └── Маршрутизация внутри mesh
│
├── MQTT Client
│   ├── Публикация: telemetry, events, heartbeats
│   ├── Подписка: commands, config
│   ├── QoS: 0 (telemetry), 1 (commands/config)
│   └── Keepalive: 60 сек
│
├── Node Registry
│   ├── Хранение: node_id, MAC, type, status
│   ├── Последние данные от каждого узла
│   ├── Timestamp последнего контакта
│   └── Автоматический timeout (30 сек → offline)
│
├── Data Router
│   ├── Прием от NODE → парсинг → MQTT
│   ├── Прием от MQTT → парсинг → NODE
│   ├── Display запросы (request/response)
│   └── Буферизация при отключении MQTT
│
├── Climate Logic (fallback)
│   ├── Активация при Climate node offline > 30 сек
│   ├── Простые таймеры (форточки, вентиляция)
│   ├── Отправка команд на Relay node
│   └── Деактивация при восстановлении Climate
│
└── Watchdog & Monitoring
    ├── Task watchdog timer
    ├── Heap memory monitoring
    ├── Mesh network monitoring
    └── MQTT connection monitoring
```

---

## 📦 КОМПОНЕНТЫ

### Из common/ (обязательные):

```cmake
# CMakeLists.txt
set(EXTRA_COMPONENT_DIRS "../common")

idf_component_register(
    REQUIRES 
        mesh_manager      # ESP-WIFI-MESH управление
        mesh_protocol     # JSON протокол
        node_config       # NVS конфигурация
        json              # cJSON (встроен в ESP-IDF)
        mqtt              # esp-mqtt
)
```

### Собственные компоненты (components/):

1. **node_registry** - реестр узлов
   - Хранение информации о всех узлах
   - Отслеживание статусов (online/offline)
   - Проверка таймаутов

2. **mqtt_client_manager** - MQTT клиент
   - Подключение к broker
   - Публикация телеметрии
   - Подписка на команды

3. **data_router** - маршрутизатор
   - Прием данных от NODE
   - Прием команд от MQTT
   - Маршрутизация между ними

4. **climate_logic** - резервная логика
   - Fallback управление климатом
   - Периодические команды

5. **ota_coordinator** (TODO) - координатор OTA
   - Раздача обновлений узлам

---

## 🔌 РАСПИНОВКА

### GPIO: **НЕ ИСПОЛЬЗУЮТСЯ**

ROOT узел работает только с WiFi/Mesh, без физических датчиков/актуаторов.

### Индикация (опционально):

Можно добавить LED для визуального контроля:

```c
// GPIO 15 - LED статус системы
// GPIO 16 - LED mesh activity (опционально)

#define GPIO_LED_STATUS  15
#define GPIO_LED_MESH    16

// Зеленый = все ОК
// Желтый мигающий = MQTT offline
// Красный = критическая ошибка
```

---

## ⚙️ КОНФИГУРАЦИЯ

### Файл: main/root_config.h

```c
#ifndef ROOT_CONFIG_H
#define ROOT_CONFIG_H

// === MESH НАСТРОЙКИ ===
#define ROOT_MESH_ID            "HYDRO1"
#define ROOT_MESH_PASSWORD      "hydro_mesh_2025"
#define ROOT_MESH_CHANNEL       1
#define ROOT_MAX_CONNECTIONS    10  // Макс узлов в сети

// === MQTT НАСТРОЙКИ ===
#define MQTT_BROKER_URI         "mqtt://192.168.1.100:1883"
#define MQTT_USERNAME           "hydro_root"
#define MQTT_PASSWORD           "hydro_pass"

// MQTT Топики
#define MQTT_TOPIC_TELEMETRY    "hydro/telemetry"
#define MQTT_TOPIC_EVENT        "hydro/event"
#define MQTT_TOPIC_HEARTBEAT    "hydro/heartbeat"
#define MQTT_TOPIC_COMMAND      "hydro/command/#"
#define MQTT_TOPIC_CONFIG       "hydro/config/#"

// === TIMEOUTS ===
#define HEARTBEAT_INTERVAL_MS   10000  // 10 сек
#define NODE_TIMEOUT_MS         30000  // 30 сек
#define MQTT_KEEPALIVE_SEC      60     // 60 сек
#define CLIMATE_FALLBACK_TIMEOUT_MS 30000

// === BUFFER SIZES ===
#define MAX_MESH_PACKET_SIZE    1456   // Макс размер mesh пакета
#define JSON_BUFFER_SIZE        2048   // Буфер для JSON
#define MQTT_BUFFER_SIZE        4096   // Буфер MQTT

#endif // ROOT_CONFIG_H
```

### Файл: sdkconfig.defaults

```ini
# ESP32-S3 specific
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y

# WiFi Router (домашняя сеть)
CONFIG_WIFI_SSID="MyHomeWiFi"
CONFIG_WIFI_PASSWORD="MyWiFiPassword123"

# Mesh
CONFIG_ESP_WIFI_MESH_SUPPORT=y
CONFIG_MESH_ENABLE=y

# MQTT
CONFIG_MQTT_PROTOCOL_311=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000
CONFIG_ESP_TASK_WDT=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10

# Logging
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
CONFIG_LOG_MAXIMUM_LEVEL_DEBUG=y
```

---

## 💻 РЕАЛИЗАЦИЯ

### main/app_main.c - точка входа

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_registry.h"
#include "mqtt_client_manager.h"
#include "data_router.h"
#include "climate_logic.h"
#include "root_config.h"

static const char *TAG = "ROOT";

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== HYDRO MESH ROOT NODE Starting ===");
    ESP_LOGI(TAG, "========================================");
    
    // 1. Инициализация NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 2. Инициализация Node Registry
    ESP_LOGI(TAG, "[Step 2/7] Initializing Node Registry...");
    ESP_ERROR_CHECK(node_registry_init());
    
    // 3. Инициализация Mesh (ROOT режим)
    ESP_LOGI(TAG, "[Step 3/7] Initializing Mesh (ROOT mode)...");
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_ROOT,
        .mesh_id = ROOT_MESH_ID,
        .mesh_password = ROOT_MESH_PASSWORD,
        .channel = ROOT_MESH_CHANNEL,
        .max_connection = ROOT_MAX_CONNECTIONS,
        .router_ssid = CONFIG_WIFI_SSID,
        .router_password = CONFIG_WIFI_PASSWORD
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(data_router_handle_mesh_data);
    
    // 4. Запуск Mesh
    ESP_LOGI(TAG, "[Step 4/7] Starting Mesh network...");
    ESP_ERROR_CHECK(mesh_manager_start());
    
    // Ждем подключения к роутеру и получения IP
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // 5. Инициализация MQTT Client
    ESP_LOGI(TAG, "[Step 5/7] Initializing MQTT Client...");
    ESP_ERROR_CHECK(mqtt_client_manager_init());
    mqtt_client_manager_register_recv_cb(data_router_handle_mqtt_data);
    
    // 6. Инициализация Data Router
    ESP_LOGI(TAG, "[Step 6/7] Initializing Data Router...");
    ESP_ERROR_CHECK(data_router_init());
    
    // 7. Инициализация Climate Logic (fallback)
    ESP_LOGI(TAG, "[Step 7/7] Initializing Climate Fallback Logic...");
    ESP_ERROR_CHECK(climate_logic_init());
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== ROOT NODE Running ===");
    ESP_LOGI(TAG, "Mesh ID: %s", ROOT_MESH_ID);
    ESP_LOGI(TAG, "MQTT Broker: %s", MQTT_BROKER_URI);
    ESP_LOGI(TAG, "========================================");
    
    // Главная задача мониторинга
    xTaskCreate(root_monitoring_task, "root_monitor", 4096, NULL, 5, NULL);
}

// Задача мониторинга системы
void root_monitoring_task(void *arg) {
    uint32_t last_log = 0;
    
    while (1) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Логирование статуса каждые 30 сек
        if (now - last_log > 30000) {
            uint32_t free_heap = esp_get_free_heap_size();
            int mesh_nodes = mesh_manager_get_total_nodes();
            int registry_nodes = node_registry_get_count();
            bool mqtt_online = mqtt_client_manager_is_connected();
            
            ESP_LOGI(TAG, "=== Status ===");
            ESP_LOGI(TAG, "Free heap: %d bytes", free_heap);
            ESP_LOGI(TAG, "Mesh nodes: %d", mesh_nodes);
            ESP_LOGI(TAG, "Registry nodes: %d", registry_nodes);
            ESP_LOGI(TAG, "MQTT: %s", mqtt_online ? "ONLINE" : "OFFLINE");
            
            // Проверка критического уровня памяти
            if (free_heap < 50000) {
                ESP_LOGW(TAG, "Low heap memory!");
            }
            
            last_log = now;
        }
        
        // Проверка таймаутов узлов
        node_registry_check_timeouts();
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Каждые 5 сек
    }
}
```

### components/data_router/data_router.c

```c
#include "data_router.h"
#include "node_registry.h"
#include "mqtt_client_manager.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "data_router";

esp_err_t data_router_init(void) {
    ESP_LOGI(TAG, "Data Router initialized");
    return ESP_OK;
}

// Обработка данных от NODE через mesh
void data_router_handle_mesh_data(const uint8_t *src_addr, const uint8_t *data, size_t len) {
    ESP_LOGD(TAG, "Mesh data received: %d bytes from "MACSTR, len, MAC2STR(src_addr));
    
    // Парсинг JSON
    mesh_message_t msg;
    if (!mesh_protocol_parse((const char*)data, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        return;
    }
    
    // Обновление реестра узлов (отметка последнего контакта)
    node_registry_update_last_seen(msg.node_id, src_addr);
    
    // Маршрутизация в зависимости от типа сообщения
    switch (msg.type) {
        case MESH_MSG_TELEMETRY:
            // Телеметрия → MQTT
            ESP_LOGI(TAG, "Telemetry from %s → MQTT", msg.node_id);
            if (mqtt_client_manager_is_connected()) {
                mqtt_client_manager_publish(MQTT_TOPIC_TELEMETRY, (const char*)data);
            } else {
                ESP_LOGW(TAG, "MQTT offline, telemetry dropped");
                // TODO: буферизация
            }
            break;
            
        case MESH_MSG_EVENT:
            // Событие → MQTT
            ESP_LOGI(TAG, "Event from %s → MQTT", msg.node_id);
            if (mqtt_client_manager_is_connected()) {
                mqtt_client_manager_publish(MQTT_TOPIC_EVENT, (const char*)data);
                
                // Проверка на критичные события
                cJSON *level = cJSON_GetObjectItem(msg.data, "level");
                if (level && strcmp(level->valuestring, "critical") == 0) {
                    ESP_LOGW(TAG, "CRITICAL event from %s!", msg.node_id);
                    // TODO: специальная обработка
                }
            }
            break;
            
        case MESH_MSG_HEARTBEAT:
            // Heartbeat → только обновление реестра (уже сделано выше)
            ESP_LOGD(TAG, "Heartbeat from %s", msg.node_id);
            
            // Опционально отправлять в MQTT для мониторинга
            if (mqtt_client_manager_is_connected()) {
                mqtt_client_manager_publish(MQTT_TOPIC_HEARTBEAT, (const char*)data);
            }
            break;
            
        case MESH_MSG_REQUEST:
            // Запрос от Display узла
            ESP_LOGI(TAG, "Request from %s", msg.node_id);
            handle_display_request(&msg, src_addr);
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }
    
    mesh_protocol_free_message(&msg);
}

// Обработка команд от MQTT
void data_router_handle_mqtt_data(const char *topic, const char *data, int data_len) {
    ESP_LOGI(TAG, "MQTT data received: %s (%d bytes)", topic, data_len);
    
    // Парсинг топика: hydro/command/{node_id} или hydro/config/{node_id}
    char node_id[32];
    bool is_command = (strstr(topic, "/command/") != NULL);
    bool is_config = (strstr(topic, "/config/") != NULL);
    
    if (is_command || is_config) {
        // Извлечение node_id из топика
        const char *slash = strrchr(topic, '/');
        if (slash) {
            strncpy(node_id, slash + 1, sizeof(node_id) - 1);
            node_id[sizeof(node_id) - 1] = '\0';
            
            // Поиск узла в реестре
            node_info_t *node = node_registry_get(node_id);
            if (node && node->online) {
                ESP_LOGI(TAG, "Forwarding %s to %s", is_command ? "command" : "config", node_id);
                
                // Отправка через mesh
                mesh_manager_send(node->mac_addr, (const uint8_t*)data, data_len);
            } else {
                ESP_LOGW(TAG, "Node %s offline or not found, message dropped", node_id);
            }
        }
    } else {
        ESP_LOGW(TAG, "Unknown MQTT topic: %s", topic);
    }
}

// Обработка запросов от Display узла
static void handle_display_request(mesh_message_t *msg, const uint8_t *src_addr) {
    cJSON *request = cJSON_GetObjectItem(msg->data, "request");
    if (!request || !cJSON_IsString(request)) {
        return;
    }
    
    const char *req_type = request->valuestring;
    ESP_LOGI(TAG, "Display request: %s", req_type);
    
    if (strcmp(req_type, "all_nodes_data") == 0) {
        // Собрать данные всех узлов из реестра
        cJSON *response_data = node_registry_export_all_to_json();
        
        // Создать response сообщение
        char response_buf[2048];
        if (mesh_protocol_create_response(msg->node_id, response_data, 
                                          response_buf, sizeof(response_buf))) {
            // Отправить обратно Display узлу
            mesh_manager_send(src_addr, (uint8_t*)response_buf, strlen(response_buf));
        }
        
        cJSON_Delete(response_data);
    }
}
```

### components/node_registry/node_registry.c

```c
#include "node_registry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

#define MAX_NODES 20

static const char *TAG = "node_registry";

typedef struct {
    char node_id[32];
    uint8_t mac_addr[6];
    char node_type[16];
    bool online;
    uint64_t last_seen_ms;
    cJSON *last_data;  // Последние данные от узла
} node_entry_t;

static node_entry_t s_registry[MAX_NODES];
static int s_node_count = 0;

esp_err_t node_registry_init(void) {
    memset(s_registry, 0, sizeof(s_registry));
    s_node_count = 0;
    ESP_LOGI(TAG, "Node Registry initialized");
    return ESP_OK;
}

void node_registry_update_last_seen(const char *node_id, const uint8_t *mac_addr) {
    // Поиск существующего узла
    node_entry_t *node = NULL;
    for (int i = 0; i < s_node_count; i++) {
        if (strcmp(s_registry[i].node_id, node_id) == 0) {
            node = &s_registry[i];
            break;
        }
    }
    
    // Если не найден - добавить новый
    if (node == NULL && s_node_count < MAX_NODES) {
        node = &s_registry[s_node_count++];
        strncpy(node->node_id, node_id, sizeof(node->node_id) - 1);
        memcpy(node->mac_addr, mac_addr, 6);
        
        ESP_LOGI(TAG, "New node added: %s ("MACSTR")", node_id, MAC2STR(mac_addr));
    }
    
    if (node) {
        node->online = true;
        node->last_seen_ms = esp_timer_get_time() / 1000;
    }
}

void node_registry_check_timeouts(void) {
    uint64_t now_ms = esp_timer_get_time() / 1000;
    
    for (int i = 0; i < s_node_count; i++) {
        if (s_registry[i].online) {
            uint64_t elapsed = now_ms - s_registry[i].last_seen_ms;
            
            if (elapsed > NODE_TIMEOUT_MS) {
                s_registry[i].online = false;
                ESP_LOGW(TAG, "Node %s TIMEOUT (offline)", s_registry[i].node_id);
            }
        }
    }
}

node_info_t* node_registry_get(const char *node_id) {
    for (int i = 0; i < s_node_count; i++) {
        if (strcmp(s_registry[i].node_id, node_id) == 0) {
            return (node_info_t*)&s_registry[i];
        }
    }
    return NULL;
}

int node_registry_get_count(void) {
    int count = 0;
    for (int i = 0; i < s_node_count; i++) {
        if (s_registry[i].online) {
            count++;
        }
    }
    return count;
}

cJSON* node_registry_export_all_to_json(void) {
    cJSON *root = cJSON_CreateArray();
    
    for (int i = 0; i < s_node_count; i++) {
        if (s_registry[i].online) {
            cJSON *node_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(node_obj, "node_id", s_registry[i].node_id);
            cJSON_AddStringToObject(node_obj, "type", s_registry[i].node_type);
            cJSON_AddBoolToObject(node_obj, "online", s_registry[i].online);
            
            // Добавить последние данные если есть
            if (s_registry[i].last_data) {
                cJSON_AddItemToObject(node_obj, "data", 
                                     cJSON_Duplicate(s_registry[i].last_data, true));
            }
            
            cJSON_AddItemToArray(root, node_obj);
        }
    }
    
    return root;
}
```

---

## 🚫 ЧТО НЕЛЬЗЯ ДЕЛАТЬ

### 1. ❌ НЕ добавляй датчики на ROOT

```c
// ПЛОХО ❌
void app_main(void) {
    init_mesh();
    init_mqtt();
    init_ph_sensor();  // ❌ НЕТ! Датчики на NODE узлах!
}

// ХОРОШО ✅
void app_main(void) {
    init_mesh();
    init_mqtt();
    // Только координация, никаких датчиков
}
```

### 2. ❌ НЕ блокируй главный поток

```c
// ПЛОХО ❌
void data_router_handle_mesh_data(...) {
    parse_json();
    send_to_mqtt_sync();  // ❌ Блокирующая операция!
    wait_for_response();  // ❌ Долгое ожидание!
}

// ХОРОШО ✅
void data_router_handle_mesh_data(...) {
    parse_json();
    mqtt_publish_async();  // ✅ Асинхронная отправка
    // Продолжаем обработку
}
```

### 3. ❌ НЕ игнорируй таймауты узлов

```c
// ПЛОХО ❌
void send_command(const char *node_id) {
    node_info_t *node = node_registry_get(node_id);
    mesh_send(node->mac_addr, data, len);  // ❌ Не проверяем online!
}

// ХОРОШО ✅
void send_command(const char *node_id) {
    node_info_t *node = node_registry_get(node_id);
    if (node && node->online) {
        mesh_send(node->mac_addr, data, len);
    } else {
        ESP_LOGW(TAG, "Node %s offline", node_id);
    }
}
```

### 4. ❌ НЕ делай тяжелых вычислений

```c
// ПЛОХО ❌
void process_telemetry(mesh_message_t *msg) {
    float prediction = ml_predict(msg->data);  // ❌ ML на ROOT!
    float pid_output = calculate_pid(...);     // ❌ PID на ROOT!
}

// ХОРОШО ✅
void process_telemetry(mesh_message_t *msg) {
    // Просто переслать на сервер
    mqtt_publish("hydro/telemetry", msg);
    // Анализ делает сервер или NODE pH/EC
}
```

---

## ✅ ЧТО НУЖНО ДЕЛАТЬ

### 1. ✅ Мониторинг всех узлов

```c
void monitoring_task(void *arg) {
    while (1) {
        // Проверка таймаутов
        node_registry_check_timeouts();
        
        // Логирование статуса
        log_system_status();
        
        // Проверка критичных узлов
        node_info_t *ph_ec = node_registry_get("ph_ec_001");
        if (!ph_ec || !ph_ec->online) {
            ESP_LOGW(TAG, "CRITICAL: pH/EC node offline!");
            // Отправить алерт
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### 2. ✅ Быстрая маршрутизация

```c
void data_router_handle_mesh_data(...) {
    // Быстрый парсинг
    mesh_message_t msg;
    if (mesh_protocol_parse(data, &msg)) {
        
        // Обновление реестра (< 1 мс)
        node_registry_update_last_seen(msg.node_id, src_addr);
        
        // Асинхронная отправка в MQTT
        mqtt_publish_async(topic, data);
        
        // Освобождение ресурсов
        mesh_protocol_free_message(&msg);
    }
    // Общее время обработки < 10 мс
}
```

### 3. ✅ Резервная логика

```c
void climate_fallback_task(void *arg) {
    while (1) {
        node_info_t *climate = node_registry_get("climate_001");
        
        if (!climate || !climate->online) {
            // Climate offline - используем fallback
            ESP_LOGW(TAG, "Using climate fallback logic");
            
            // Простые периодические команды
            if (time_to_open_windows()) {
                send_command_to_relay("open_windows");
            }
            if (time_to_start_fan()) {
                send_command_to_relay("start_fan");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(60000));  // Каждую минуту
    }
}
```

### 4. ✅ Watchdog reset

```c
#include "esp_task_wdt.h"

void monitoring_task(void *arg) {
    // Регистрация задачи в watchdog
    esp_task_wdt_add(NULL);
    
    while (1) {
        // Сброс watchdog
        esp_task_wdt_reset();
        
        // Полезная работа
        check_system_health();
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

---

## 🎓 УРОКИ ИЗ РЕАЛИЗАЦИИ ROOT NODE

### ⚠️ Критичные баги, которые были найдены и исправлены:

#### 1. **Неправильные форматы printf для uint32_t**
```c
// ❌ НЕПРАВИЛЬНО (ошибка компиляции на ESP32):
uint32_t heap_free = esp_get_free_heap_size();
snprintf(buffer, size, "%u", heap_free);  // format '%u' expects 'unsigned int', but argument has type 'uint32_t' (aka 'long unsigned int')

// ✅ ПРАВИЛЬНО:
uint32_t heap_free = esp_get_free_heap_size();
snprintf(buffer, size, "%lu", (unsigned long)heap_free);
```

**Причина:** На ESP32 `uint32_t` определён как `long unsigned int`, а не `unsigned int`

#### 2. **Неправильная длина для esp_mqtt_client_publish**
```c
// ❌ НЕПРАВИЛЬНО (discovery message был пустой):
char discovery_msg[768];
snprintf(discovery_msg, sizeof(discovery_msg), "{\"type\":\"discovery\"...}");
esp_mqtt_client_publish(client, topic, discovery_msg, 0, 1, 0);  // len = 0 отправляет ПУСТОЕ сообщение!

// ✅ ПРАВИЛЬНО:
char discovery_msg[768];
snprintf(discovery_msg, sizeof(discovery_msg), "{\"type\":\"discovery\"...}");
esp_mqtt_client_publish(client, topic, discovery_msg, strlen(discovery_msg), 1, 0);  // правильная длина
```

**Последствия:** На сервер приходили пустые discovery сообщения, узлы не регистрировались

#### 3. **Циклические зависимости заголовочных файлов**
```c
// ❌ НЕПРАВИЛЬНО (ошибка компиляции):
// в mqtt_client_manager.c:
#include "mesh_manager.h"  // Создаёт циклическую зависимость

// ✅ ПРАВИЛЬНО:
// Использовать только необходимые прямые включения
// Для функций из других компонентов использовать extern или вынести в общий header
```

#### 4. **Отсутствие включения необходимых заголовков**
```c
// ❌ НЕПРАВИЛЬНО (ошибка компиляции):
uint32_t flash_size = spi_flash_get_chip_size();  // Implicit declaration

// ✅ ПРАВИЛЬНО:
#include "spi_flash_mmap.h"
extern uint32_t spi_flash_get_chip_size(void);
uint32_t flash_size = spi_flash_get_chip_size();
```

#### 5. **Устаревший компонент в CMakeLists.txt**
```cmake
# ❌ НЕПРАВИЛЬНО (ошибка компиляции):
REQUIRES esp_task_wdt  # Этот компонент больше не существует отдельно

# ✅ ПРАВИЛЬНО:
REQUIRES esp_system  # esp_task_wdt теперь часть esp_system
```

### 📝 Финальный чеклист перед прошивкой:

1. ✅ Все `uint32_t` форматируются как `%lu` с `(unsigned long)` cast
2. ✅ Все MQTT publish используют `strlen()` для длины сообщения
3. ✅ Проверены все включения заголовочных файлов (нет циклических зависимостей)
4. ✅ Target правильно установлен: `idf.py set-target esp32s3`
5. ✅ Router credentials совпадают во всех нодах
6. ✅ MQTT broker доступен на порту 1883
7. ✅ Windows Firewall разрешает порт 1883 (если Docker)
8. ✅ Discovery message содержит правильный `node_type` (не `type`)

### 🐛 Troubleshooting:

**Проблема:** `esp-tls: select() timeout` в логах ESP32  
**Решение:** 
```powershell
netsh advfirewall firewall add rule name="MQTT Docker" dir=in action=allow protocol=TCP localport=1883
```

**Проблема:** Discovery message не доходит до сервера  
**Решение:** Проверь:
- `strlen(discovery_msg)` используется в `esp_mqtt_client_publish`
- MQTT Listener правильно подписан на `hydro/discovery` (без `/#`)
- Mosquitto логи показывают PUBLISH от ESP32

**Проблема:** Узел создаётся с `node_type: "discovery"`  
**Решение:** В `MqttService.php` используй `$data['node_type']`, а не `$data['type']`

---

## 🧪 ТЕСТИРОВАНИЕ

### Тест 1: Базовая инициализация

```bash
cd root_node
idf.py set-target esp32s3
idf.py build
idf.py -p COM3 flash monitor
```

**Ожидаемый лог:**
```
I ROOT: === HYDRO MESH ROOT NODE Starting ===
I ROOT: [Step 1/7] Initializing NVS...
I ROOT: [Step 2/7] Initializing Node Registry...
I node_registry: Node Registry initialized
I ROOT: [Step 3/7] Initializing Mesh (ROOT mode)...
I mesh_manager: ROOT mode: connecting to router MyHomeWiFi
I ROOT: [Step 4/7] Starting Mesh network...
I mesh_manager: Mesh started
I mesh_manager: ROOT got IP address: 192.168.1.101
I ROOT: [Step 5/7] Initializing MQTT Client...
I mqtt_manager: Connecting to broker...
I mqtt_manager: MQTT connected
I ROOT: [Step 6/7] Initializing Data Router...
I ROOT: [Step 7/7] Initializing Climate Fallback Logic...
I ROOT: === ROOT NODE Running ===
I ROOT: Mesh ID: HYDRO1
I ROOT: MQTT Broker: mqtt://192.168.1.100:1883
```

### Тест 2: Подключение NODE узла

Прошей любой NODE (например node_climate):
```bash
cd ../node_climate
idf.py build flash
```

**В логах ROOT должно появиться:**
```
I mesh_manager: Child connected: AA:BB:CC:DD:EE:FF
I data_router: Mesh data received (256 bytes)
I node_registry: New node added: climate_001 (AA:BB:CC:DD:EE:FF)
I data_router: Telemetry from climate_001 → MQTT
I mqtt_manager: Published to hydro/telemetry
```

### Тест 3: MQTT команды

```bash
# Терминал 1: Мониторинг
mosquitto_sub -h 192.168.1.100 -t "hydro/#" -v

# Терминал 2: Отправка команды
mosquitto_pub -h 192.168.1.100 -t "hydro/command/climate_001" \
  -m '{"type":"command","node_id":"climate_001","command":"restart"}'
```

**В логах ROOT:**
```
I data_router: MQTT data received: hydro/command/climate_001 (65 bytes)
I data_router: Forwarding command to climate_001
I mesh_manager: Sent data to AA:BB:CC:DD:EE:FF
```

### Тест 4: Stress Test

```bash
# Запустить 5+ NODE узлов одновременно
# ROOT должен стабильно обрабатывать все данные

# Мониторинг производительности
python tools/monitor_mesh.py
```

---

## 📊 ЧЕКЛИСТ РАЗРАБОТКИ

- [ ] Mesh инициализация (ROOT режим)
- [ ] Подключение к WiFi роутеру
- [ ] MQTT подключение и reconnect
- [ ] Node Registry (add, update, timeout)
- [ ] Data Router (NODE → MQTT)
- [ ] Data Router (MQTT → NODE)
- [ ] Heartbeat обработка
- [ ] Display request/response
- [ ] Climate fallback логика
- [ ] Watchdog timer
- [ ] Heap monitoring
- [ ] LED индикация (опционально)
- [ ] Тестирование с 1 NODE
- [ ] Тестирование с 3+ NODE
- [ ] Тестирование MQTT команд
- [ ] Стресс-тест (24 часа)

---

## 📚 ДОПОЛНИТЕЛЬНЫЕ РЕСУРСЫ

### Документация
- `../common/AI_INSTRUCTIONS.md` - общие компоненты
- `../doc/MESH_HYDRO_V2_FINAL_PLAN.md` - общий план
- `../doc/MESH_ARCHITECTURE_PLAN.md` - архитектура системы

### ESP-IDF документация
- [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/esp-wifi-mesh.html)
- [ESP-MQTT](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html)
- [FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)

---

**ROOT узел - сердце системы. Делай его максимально надежным и стабильным!** 🏆

**Приоритет:** 🔴 КРИТИЧНО (Шаг 4 из плана)

**Время реализации:** 3-4 дня
