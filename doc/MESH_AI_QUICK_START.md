# 🤖 MESH HYDRO V2 - БЫСТРЫЙ СТАРТ ДЛЯ AI

**Для AI-ассистентов:** Что нужно знать перед началом работы

---

## 📖 ОБЯЗАТЕЛЬНО ПРОЧИТАТЬ

### 1. Главные файлы плана:
- **MESH_HYDRO_V2_FINAL_PLAN.md** - полный детальный план
- **MESH_ARCHITECTURE_PLAN.md** - архитектура системы
- **MESH_PINOUT_ALL_NODES.md** - GPIO распиновка
- **MESH_IMPLEMENTATION_PRIORITY.md** - порядок реализации

### 2. Базовый проект (для примеров):
- **c:\Users\admin\2\hydro1.0** - текущий монолитный проект
- Используй компоненты оттуда! Не пиши с нуля!

---

## 🎯 КЛЮЧЕВЫЕ ПРАВИЛА

### Правило #1: Используй существующий код
```
❌ НЕ ДЕЛАЙ: Пиши sensor_manager с нуля
✅ ДЕЛАЙ: Копируй из hydro1.0/components/sensor_manager/
```

### Правило #2: NODE pH/EC - святое!
```
⚠️ КРИТИЧНО:
- ВСЕГДА автономная работа
- ВСЕГДА локальный буфер
- ВСЕГДА watchdog reset
- НИКОГДА не блокировать PID
```

### Правило #3: JSON < 1 KB
```
Mesh пакет: max 1456 байт
Используй краткие ключи:
  ❌ "timestamp"  →  ✅ "ts"
  ❌ "temperature" → ✅ "temp"
```

### Правило #4: Каждый узел хранит конфигурацию в NVS
```c
// При получении конфигурации от ROOT
void on_config_received(cJSON *json) {
    parse_json_to_config(json, &config);
    node_config_save(&config, ...);  // Сохранить в NVS!
    apply_config(&config);
}
```

### Правило #5: ESP-IDF v5.5.1, НЕ Arduino!
```
✅ Используй: esp_mesh.h, esp_wifi.h
❌ НЕ используй: Arduino WiFi.h
```

---

## 🚀 С ЧЕГО НАЧАТЬ

### Шаг 1: Создать mesh_manager

**Приоритет:** 🔴 КРИТИЧНО

**Файлы:**
```
common/mesh_manager/mesh_manager.h
common/mesh_manager/mesh_manager.c
common/mesh_manager/CMakeLists.txt
```

**Что должно быть:**
```c
// ROOT режим
esp_err_t mesh_manager_init_root(mesh_root_config_t *config);
esp_err_t mesh_send_to_node(uint8_t *mac, uint8_t *data, size_t len);
esp_err_t mesh_broadcast(uint8_t *data, size_t len);

// NODE режим
esp_err_t mesh_manager_init_node(mesh_node_config_t *config);
esp_err_t mesh_send_to_root(uint8_t *data, size_t len);

// Общее
esp_err_t mesh_set_receive_callback(mesh_receive_cb_t callback);
bool mesh_is_connected(void);
```

**Используй:** esp_mesh.h из ESP-IDF

---

### Шаг 2: Создать mesh_protocol

**Приоритет:** 🔴 КРИТИЧНО

**Файлы:**
```
common/mesh_protocol/mesh_protocol.h
common/mesh_protocol/mesh_protocol.c
common/mesh_protocol/CMakeLists.txt
```

**Что должно быть:**
```c
// Парсинг
esp_err_t mesh_protocol_parse(const char *json, mesh_message_t *msg);

// Создание
char* mesh_protocol_create_telemetry(const char *node_id, cJSON *data);
char* mesh_protocol_create_command(const char *node_id, const char *cmd, cJSON *params);
char* mesh_protocol_create_event(const char *node_id, const char *level, const char *msg);
```

**Используй:** cJSON (встроен в ESP-IDF)

---

### Шаг 3: Создать node_config

**Приоритет:** 🔴 КРИТИЧНО

**Файлы:**
```
common/node_config/node_config.h
common/node_config/node_config.c
common/node_config/CMakeLists.txt
```

**Что должно быть:**
```c
esp_err_t node_config_init(void);
esp_err_t node_config_load(void *config, size_t size, const char *ns);
esp_err_t node_config_save(const void *config, size_t size, const char *ns);
```

**Используй:** nvs_flash (ESP-IDF)

---

## 💡 ПРИМЕРЫ КОДА

### Как отправить данные на ROOT (NODE):

```c
#include "mesh_manager.h"
#include "mesh_protocol.h"

void send_telemetry(float ph, float ec) {
    // Создание JSON
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "ph", ph);
    cJSON_AddNumberToObject(data, "ec", ec);
    
    char *json_str = mesh_protocol_create_telemetry("ph_ec_001", data);
    
    // Отправка на ROOT
    mesh_send_to_root((uint8_t*)json_str, strlen(json_str));
    
    free(json_str);
    cJSON_Delete(data);
}
```

### Как обработать команду (NODE):

```c
void mesh_receive_callback(const uint8_t *from, const uint8_t *data, size_t len) {
    // Парсинг JSON
    mesh_message_t msg;
    mesh_protocol_parse((const char*)data, &msg);
    
    if (strcmp(msg.type, "command") == 0) {
        if (strcmp(msg.command, "set_target") == 0) {
            float new_ph = cJSON_GetObjectItem(msg.params, "ph_target")->valuedouble;
            
            // Обновление конфигурации
            config.ph_target = new_ph;
            
            // Сохранение в NVS
            node_config_save(&config, sizeof(config), "ph_ec_ns");
            
            ESP_LOGI(TAG, "pH target updated: %.2f", new_ph);
        }
    }
}
```

---

## 📚 ПОЛЕЗНЫЕ ССЫЛКИ

### Документация ESP-IDF:
- ESP-WIFI-MESH: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/esp-wifi-mesh.html
- ESP MQTT: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html
- NVS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html

### Текущий проект (hydro1.0):
- ИНСТРУКЦИЯ_ДЛЯ_ИИ.md - команды сборки/прошивки
- components/sensor_manager/ - пример менеджера датчиков
- components/pump_manager/ - пример менеджера насосов
- components/adaptive_pid/ - готовый PID контроллер

---

## ⚠️ ЧАСТЫЕ ОШИБКИ

### 1. Забыть watchdog reset
```c
// ❌ ПЛОХО
while (1) {
    heavy_operation();
}

// ✅ ХОРОШО
while (1) {
    esp_task_wdt_reset();
    heavy_operation();
    vTaskDelay(pdMS_TO_TICKS(100));
}
```

### 2. JSON слишком большой
```c
// ❌ ПЛОХО (> 1KB)
cJSON_AddStringToObject(json, "very_long_description", "...");

// ✅ ХОРОШО (краткие ключи)
cJSON_AddNumberToObject(json, "ph", 6.5);  // Не "ph_value"
```

### 3. Не сохранить в NVS
```c
// ❌ ПЛОХО
config.ph_target = 6.5;  // Потеряется при перезагрузке!

// ✅ ХОРОШО
config.ph_target = 6.5;
node_config_save(&config, sizeof(config), "ph_ec_ns");
```

---

## 🎯 ПРИОРИТЕТ РЕАЛИЗАЦИИ

1. **mesh_manager** (КРИТИЧНО!)
2. **mesh_protocol** (КРИТИЧНО!)
3. **node_config** (КРИТИЧНО!)
4. **ROOT узел** (ВЫСОКИЙ)
5. **Сервер** (ВЫСОКИЙ)
6. **NODE pH/EC** (МАКСИМАЛЬНЫЙ! ⚠️)
7. Остальные узлы (средний)
8. OTA, Telegram, SMS (низкий, можно потом)

---

**ГОТОВ К РАБОТЕ!** 🚀

**Начни с:** mesh_manager → mesh_protocol → node_config → ROOT → NODE pH/EC

