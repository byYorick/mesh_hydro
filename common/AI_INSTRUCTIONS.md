# 🤖 ИНСТРУКЦИЯ ДЛЯ ИИ: ОБЩИЕ КОМПОНЕНТЫ (common/)

**Для AI-ассистентов:** Как работать с общими компонентами mesh_hydro проекта

---

## 📁 ЧТО ТАКОЕ common/?

Папка `common/` содержит **универсальные компоненты**, используемые **всеми узлами** (ROOT, NODE pH/EC, NODE Climate, и т.д.).

### Принцип DRY (Don't Repeat Yourself)

```
❌ ПЛОХО: Копировать код mesh_manager в каждый узел
✅ ХОРОШО: Один компонент common/mesh_manager для всех
```

---

## 📦 ГОТОВЫЕ КОМПОНЕНТЫ

### 1. ✅ mesh_manager (ПОЛНОСТЬЮ ГОТОВ)

**Назначение:** Управление ESP-WIFI-MESH сетью

**Файлы:**
- `mesh_manager/mesh_manager.h` - API
- `mesh_manager/mesh_manager.c` - реализация
- `mesh_manager/CMakeLists.txt` - сборка
- `mesh_manager/README.md` - документация

**Ключевые функции:**
```c
// Инициализация (ROOT или NODE)
mesh_manager_config_t config = {
    .mode = MESH_MODE_ROOT,  // или MESH_MODE_NODE
    .mesh_id = "HYDRO1",
    .mesh_password = "mesh_pass",
    .channel = 1
};
mesh_manager_init(&config);
mesh_manager_start();

// NODE: Отправка на ROOT
mesh_manager_send_to_root(data, len);

// ROOT: Broadcast всем узлам
mesh_manager_broadcast(data, len);

// Callback для приема
void on_data(const uint8_t *src, const uint8_t *data, size_t len) {
    // Обработка
}
mesh_manager_register_recv_cb(on_data);
```

**Использование в CMakeLists.txt:**
```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES mesh_manager  # Добавить зависимость
)
```

---

### 2. ✅ mesh_protocol (ПОЛНОСТЬЮ ГОТОВ)

**Назначение:** JSON протокол обмена данными

**Типы сообщений:**
- `telemetry` - телеметрия (NODE → ROOT)
- `command` - команды (ROOT → NODE)
- `config` - конфигурация (ROOT → NODE)
- `event` - события (NODE → ROOT)
- `heartbeat` - проверка связи
- `request` - запрос данных (Display)
- `response` - ответ на запрос

**Создание сообщений:**
```c
#include "mesh_protocol.h"

// Телеметрия
cJSON *data = cJSON_CreateObject();
cJSON_AddNumberToObject(data, "ph", 6.5);
cJSON_AddNumberToObject(data, "ec", 1.8);

char json_buf[1024];
mesh_protocol_create_telemetry("ph_ec_001", data, json_buf, sizeof(json_buf));

// Отправка
mesh_manager_send_to_root((uint8_t*)json_buf, strlen(json_buf));

cJSON_Delete(data);
```

**Парсинг входящих:**
```c
void on_mesh_data(const uint8_t *src, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    
    if (mesh_protocol_parse((char*)data, &msg)) {
        switch (msg.type) {
            case MESH_MSG_COMMAND:
                // Обработка команды
                handle_command(&msg);
                break;
                
            case MESH_MSG_CONFIG:
                // Обновление конфигурации
                node_config_update_from_json(&config, msg.data, "ph_ec");
                node_config_save(&config, sizeof(config), "ph_ec_ns");
                break;
        }
        
        mesh_protocol_free_message(&msg);
    }
}
```

**Использование:**
```cmake
idf_component_register(
    REQUIRES mesh_protocol json  # json = cJSON (встроен в ESP-IDF)
)
```

---

### 3. ✅ node_config (ПОЛНОСТЬЮ ГОТОВ)

**Назначение:** NVS хранение конфигурации для всех типов узлов

**Поддерживаемые типы:**
- `ph_ec` - pH/EC узел (PID, safety, автономия)
- `climate` - Climate узел (температура, влажность, CO2)
- `relay` - Relay узел (свет, вентиляция, форточки)
- `water` - Water узел (полив)

**Типичный сценарий (NODE):**
```c
#include "node_config.h"

void app_main(void) {
    // 1. Инициализация NVS
    node_config_init();
    
    // 2. Загрузка конфигурации
    ph_ec_node_config_t config;
    esp_err_t err = node_config_load(&config, sizeof(config), "ph_ec_ns");
    
    if (err != ESP_OK) {
        // Первый запуск - значения по умолчанию
        node_config_reset_to_default(&config, "ph_ec");
        node_config_save(&config, sizeof(config), "ph_ec_ns");
    }
    
    // 3. Применение конфигурации
    apply_config(&config);
    
    // 4. При получении новой конфигурации от ROOT
    // (в callback mesh_manager)
    void on_config_received(cJSON *json_config) {
        node_config_update_from_json(&config, json_config, "ph_ec");
        node_config_save(&config, sizeof(config), "ph_ec_ns");  // ВАЖНО!
        apply_config(&config);
    }
}
```

**Критично для pH/EC узла:**
```c
// ⚠️ ВСЕГДА сохраняй в NVS после обновления!
config.ph_target = 6.8;
node_config_save(&config, sizeof(config), "ph_ec_ns");

// При потере связи с mesh - используй сохраненную конфигурацию
if (!mesh_manager_is_connected() && config.autonomous_enabled) {
    // Продолжаем работу с последней сохраненной конфигурацией
    run_autonomous_mode(&config);
}
```

**Использование:**
```cmake
idf_component_register(
    REQUIRES node_config nvs_flash json
)
```

---

### 4. 🔄 sensor_base (В РАЗРАБОТКЕ)

**Назначение:** Базовые функции для датчиков

**Планируемый API:**
```c
// Регистрация датчика
sensor_handle_t sensor;
sensor_register(&sensor, SENSOR_TYPE_PH, read_ph_callback);

// Автоматическое чтение с интервалом
sensor_start_reading(&sensor, 5000);  // каждые 5 сек

// Callback с данными
void on_sensor_data(sensor_handle_t *sensor, float value) {
    ESP_LOGI(TAG, "pH: %.2f", value);
}
```

---

### 5. 🔄 actuator_base (В РАЗРАБОТКЕ)

**Назначение:** Базовые функции для исполнительных устройств (насосы, реле)

**Планируемый API:**
```c
// Регистрация актуатора
actuator_handle_t pump;
actuator_register(&pump, ACTUATOR_TYPE_PUMP, GPIO_NUM_12);

// Включение с safety
actuator_on(&pump, 5000);  // на 5 сек, автоматически выключится

// Safety проверки
actuator_set_max_time(&pump, 10000);     // макс 10 сек
actuator_set_cooldown(&pump, 60000);     // 1 мин между включениями
actuator_set_daily_limit(&pump, 1000);   // 1 литр в день
```

---

### 6. 🔄 ota_manager (В РАЗРАБОТКЕ)

**Назначение:** OTA обновления прошивки

**Планируемый API:**
```c
// Проверка обновлений
ota_check_update("ph_ec_001");

// Загрузка и установка
ota_download_and_install("https://server.com/firmware.bin");

// Откат к предыдущей версии
ota_rollback();
```

---

## 🔧 КАК ДОБАВИТЬ common КОМПОНЕНТ В ПРОЕКТ

### Шаг 1: Структура проекта

```
node_ph_ec/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   └── main.c
└── (symlink или копия) → common/
```

### Шаг 2: Главный CMakeLists.txt

```cmake
# node_ph_ec/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Добавление пути к common компонентам
set(EXTRA_COMPONENT_DIRS "../common")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(node_ph_ec)
```

### Шаг 3: main/CMakeLists.txt

```cmake
# node_ph_ec/main/CMakeLists.txt
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        mesh_manager    # из common/
        mesh_protocol   # из common/
        node_config     # из common/
        # ... другие зависимости
)
```

### Шаг 4: Использование в коде

```c
// main.c
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

void app_main(void) {
    // Инициализация NVS
    node_config_init();
    
    // Загрузка конфигурации
    ph_ec_node_config_t config;
    node_config_load(&config, sizeof(config), "ph_ec_ns");
    
    // Инициализация mesh
    mesh_manager_config_t mesh_cfg = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .channel = 1
    };
    mesh_manager_init(&mesh_cfg);
    mesh_manager_register_recv_cb(on_mesh_data);
    mesh_manager_start();
    
    // ... ваша логика
}
```

---

## 📋 ПРИМЕРЫ ИНТЕГРАЦИИ

### Пример 1: NODE pH/EC (полный цикл)

```c
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

static ph_ec_node_config_t g_config;

// Callback приема данных от ROOT
void on_mesh_data(const uint8_t *src, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    
    if (!mesh_protocol_parse((char*)data, &msg)) {
        return;
    }
    
    switch (msg.type) {
        case MESH_MSG_COMMAND:
            if (strcmp(msg.node_id, g_config.base.node_id) == 0) {
                handle_command(&msg);
            }
            break;
            
        case MESH_MSG_CONFIG:
            // Обновление конфигурации
            node_config_update_from_json(&g_config, msg.data, "ph_ec");
            node_config_save(&g_config, sizeof(g_config), "ph_ec_ns");
            apply_config(&g_config);
            ESP_LOGI(TAG, "Config updated from ROOT");
            break;
    }
    
    mesh_protocol_free_message(&msg);
}

// Отправка телеметрии
void send_telemetry(float ph, float ec, float temp) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, buffering data");
        return;
    }
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "ph", ph);
    cJSON_AddNumberToObject(data, "ec", ec);
    cJSON_AddNumberToObject(data, "temp", temp);
    
    char json_buf[512];
    if (mesh_protocol_create_telemetry(g_config.base.node_id, data, 
                                        json_buf, sizeof(json_buf))) {
        mesh_manager_send_to_root((uint8_t*)json_buf, strlen(json_buf));
    }
    
    cJSON_Delete(data);
}

void app_main(void) {
    // Инициализация
    node_config_init();
    
    // Загрузка конфигурации
    if (node_config_load(&g_config, sizeof(g_config), "ph_ec_ns") != ESP_OK) {
        node_config_reset_to_default(&g_config, "ph_ec");
        strcpy(g_config.base.node_id, "ph_ec_001");
        node_config_save(&g_config, sizeof(g_config), "ph_ec_ns");
    }
    
    // Mesh
    mesh_manager_config_t mesh_cfg = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .channel = 1
    };
    mesh_manager_init(&mesh_cfg);
    mesh_manager_register_recv_cb(on_mesh_data);
    mesh_manager_start();
    
    // Основной цикл
    while (1) {
        float ph = read_ph_sensor();
        float ec = read_ec_sensor();
        float temp = read_temp_sensor();
        
        send_telemetry(ph, ec, temp);
        
        // PID управление насосами
        run_pid_control(&g_config, ph, ec);
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### Пример 2: ROOT узел (маршрутизация)

```c
#include "mesh_manager.h"
#include "mesh_protocol.h"

void on_mesh_data_from_node(const uint8_t *src, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    
    if (!mesh_protocol_parse((char*)data, &msg)) {
        return;
    }
    
    switch (msg.type) {
        case MESH_MSG_TELEMETRY:
            // Переслать на MQTT сервер
            mqtt_publish("hydro/telemetry", (char*)data, len);
            ESP_LOGI(TAG, "Telemetry from %s forwarded to MQTT", msg.node_id);
            break;
            
        case MESH_MSG_EVENT:
            // Переслать события
            mqtt_publish("hydro/events", (char*)data, len);
            break;
            
        case MESH_MSG_HEARTBEAT:
            // Обновить статус узла в реестре
            node_registry_update_heartbeat(msg.node_id);
            break;
    }
    
    mesh_protocol_free_message(&msg);
}

void on_mqtt_message(const char *topic, const char *payload) {
    // Команда от сервера → отправить в mesh
    if (strcmp(topic, "hydro/commands") == 0) {
        cJSON *json = cJSON_Parse(payload);
        const char *node_id = cJSON_GetObjectItem(json, "node_id")->valuestring;
        
        // Получить MAC адрес узла из реестра
        uint8_t mac[6];
        if (node_registry_get_mac(node_id, mac)) {
            mesh_manager_send(mac, (uint8_t*)payload, strlen(payload));
        }
        
        cJSON_Delete(json);
    }
}
```

---

## ⚠️ ВАЖНЫЕ ПРАВИЛА

### Правило 1: ВСЕГДА используй готовые компоненты

```c
❌ НЕ ДЕЛАЙ:
esp_err_t my_mesh_init() {
    // Свой код mesh с нуля
}

✅ ДЕЛАЙ:
#include "mesh_manager.h"
mesh_manager_init(&config);
```

### Правило 2: Сохраняй конфигурацию в NVS

```c
❌ НЕ ДЕЛАЙ:
config.ph_target = 6.8;  // Потеряется при перезагрузке!

✅ ДЕЛАЙ:
config.ph_target = 6.8;
node_config_save(&config, sizeof(config), "ph_ec_ns");
```

### Правило 3: JSON < 1 KB для mesh

```c
❌ НЕ ДЕЛАЙ:
cJSON_AddStringToObject(json, "very_long_description", "...");

✅ ДЕЛАЙ:
// Используй краткие данные
cJSON_AddNumberToObject(json, "ph", 6.5);
cJSON_AddNumberToObject(json, "ec", 1.8);
```

### Правило 4: Watchdog reset в циклах

```c
❌ НЕ ДЕЛАЙ:
while (1) {
    heavy_operation();
}

✅ ДЕЛАЙ:
while (1) {
    esp_task_wdt_reset();
    heavy_operation();
    vTaskDelay(pdMS_TO_TICKS(100));
}
```

### Правило 5: Проверяй mesh соединение

```c
✅ ХОРОШАЯ ПРАКТИКА:
void send_data(void) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, buffering data");
        buffer_locally();  // Сохранить локально
        return;
    }
    
    mesh_manager_send_to_root(data, len);
}
```

---

## 🔍 ОТЛАДКА

### Проверка компонентов

```bash
# Сборка проекта покажет ошибки линковки, если компонент не найден
cd node_ph_ec
idf.py build
```

### Логирование

```c
// Установка уровня логов для компонента
esp_log_level_set("mesh_manager", ESP_LOG_DEBUG);
esp_log_level_set("mesh_protocol", ESP_LOG_INFO);
esp_log_level_set("node_config", ESP_LOG_INFO);
```

### Проверка NVS

```c
// Вывод текущей конфигурации
ph_ec_node_config_t config;
node_config_load(&config, sizeof(config), "ph_ec_ns");

ESP_LOGI(TAG, "Config: pH=%.2f, EC=%.2f, version=%d", 
         config.ph_target, config.ec_target, config.base.config_version);
```

---

## 📚 ДОПОЛНИТЕЛЬНЫЕ РЕСУРСЫ

### Документация компонентов
- `common/mesh_manager/README.md` - полная документация mesh_manager
- `common/mesh_protocol/README.md` - протокол и примеры
- `common/node_config/README.md` - **340+ строк** примеров и сценариев

### Общая документация проекта
- `doc/MESH_AI_QUICK_START.md` - быстрый старт для ИИ
- `doc/MESH_HYDRO_V2_FINAL_PLAN.md` - полный план проекта
- `doc/MESH_ARCHITECTURE_PLAN.md` - архитектура системы

### ESP-IDF документация
- [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/esp-wifi-mesh.html)
- [NVS Storage](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html)
- [cJSON](https://github.com/DaveGamble/cJSON)

---

## ✅ ЧЕКЛИСТ ДЛЯ НОВОГО УЗЛА

Когда создаешь новый узел (например, `node_climate`), убедись:

- [ ] Добавлен `set(EXTRA_COMPONENT_DIRS "../common")` в CMakeLists.txt
- [ ] Добавлены `REQUIRES mesh_manager mesh_protocol node_config` в main/CMakeLists.txt
- [ ] Инициализирован NVS через `node_config_init()`
- [ ] Загружена/создана конфигурация через `node_config_load()` или `node_config_reset_to_default()`
- [ ] Инициализирован mesh через `mesh_manager_init()`
- [ ] Зарегистрирован callback через `mesh_manager_register_recv_cb()`
- [ ] Обрабатываются сообщения типа `MESH_MSG_COMMAND` и `MESH_MSG_CONFIG`
- [ ] Конфигурация **сохраняется в NVS** при изменении
- [ ] Есть обработка отключения mesh (автономный режим для критичных узлов)

---

## 🎯 ИТОГО

**common/** - это фундамент всей mesh системы. Используй готовые компоненты, не изобретай велосипед!

Все компоненты протестированы и готовы к использованию. Если что-то не работает - проверь CMakeLists.txt и зависимости.

**Вопросы?** Читай README.md в каждом компоненте - там подробные примеры!

---

**Версия:** 1.0  
**Дата:** 2025-10-18  
**Статус:** 3 компонента готовы (mesh_manager, mesh_protocol, node_config), остальные в разработке

