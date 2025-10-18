# Node Config

Компонент для управления конфигурацией всех типов узлов с хранением в NVS (Non-Volatile Storage).

## Возможности

- ✅ Сохранение/загрузка конфигурации в NVS
- ✅ Парсинг JSON от ROOT узла
- ✅ Экспорт конфигурации в JSON
- ✅ Значения по умолчанию для всех типов узлов
- ✅ Версионность конфигураций
- ✅ Factory reset (полная очистка NVS)

## Поддерживаемые типы узлов

### 1. pH/EC узел (`ph_ec`)

```c
typedef struct {
    base_config_t base;           // Базовая конфигурация
    float ph_target, ec_target;   // Целевые значения
    float ph_min, ph_max;         // Рабочие диапазоны
    float ec_min, ec_max;
    pump_pid_t pump_pid[5];       // PID параметры для 5 насосов
    uint32_t max_pump_time_ms;    // Safety параметры
    bool autonomous_enabled;      // Автономный режим
    float ph_cal_offset;          // Калибровка
    // ... и другие поля
} ph_ec_node_config_t;
```

### 2. Climate узел (`climate`)

```c
typedef struct {
    base_config_t base;
    float temp_target;            // 24.0°C
    float humidity_target;        // 65.0%
    uint16_t co2_max;            // 800 ppm
    uint16_t lux_min;            // 500 lux
    uint32_t read_interval_ms;   // 10000 ms
} climate_node_config_t;
```

### 3. Relay узел (`relay`)

```c
typedef struct {
    base_config_t base;
    struct {
        uint8_t brightness;       // Свет 0-100%
        char schedule_on[6];      // "08:00"
        char schedule_off[6];     // "22:00"
        bool schedule_enabled;
    } light;
    struct {
        uint16_t co2_threshold;   // Вентиляция
        float temp_threshold;
    } ventilation;
    // ... форточки и др.
} relay_node_config_t;
```

### 4. Water узел (`water`)

```c
typedef struct {
    base_config_t base;
    uint32_t pump_on_time_ms;
    uint32_t pump_off_time_ms;
    uint8_t active_zones;         // Битовая маска зон
    uint32_t max_pump_time_ms;
    bool water_level_check;
} water_node_config_t;
```

## API функции

### Инициализация

```c
esp_err_t node_config_init(void);
```

Инициализирует NVS. Вызывать один раз при старте узла.

### Загрузка конфигурации

```c
esp_err_t node_config_load(void *config, size_t size, const char *namespace);
```

Загружает конфигурацию из NVS.

**Пример:**
```c
ph_ec_node_config_t config;
esp_err_t err = node_config_load(&config, sizeof(config), "ph_ec_ns");

if (err != ESP_OK) {
    // Конфигурация не найдена, используем значения по умолчанию
    node_config_reset_to_default(&config, "ph_ec");
}
```

### Сохранение конфигурации

```c
esp_err_t node_config_save(const void *config, size_t size, const char *namespace);
```

Сохраняет конфигурацию в NVS.

**Пример:**
```c
config.ph_target = 6.8;
config.ec_target = 2.0;

node_config_save(&config, sizeof(config), "ph_ec_ns");
ESP_LOGI(TAG, "Config saved to NVS");
```

### Обновление из JSON (от ROOT)

```c
esp_err_t node_config_update_from_json(void *config, cJSON *json_config, const char *node_type);
```

Парсит JSON от ROOT и обновляет конфигурацию. **КРИТИЧНО для работы системы!**

**Пример (NODE получает команду от ROOT):**
```c
void on_mesh_data_received(const uint8_t *data, size_t len) {
    mesh_message_t msg;
    
    if (mesh_protocol_parse((char*)data, &msg)) {
        if (msg.type == MESH_MSG_CONFIG) {
            // Обновление конфигурации
            node_config_update_from_json(&config, msg.data, "ph_ec");
            
            // Сохранение в NVS
            node_config_save(&config, sizeof(config), "ph_ec_ns");
            
            // Применение новой конфигурации
            apply_config(&config);
            
            ESP_LOGI(TAG, "Config updated from ROOT (v%d)", config.base.config_version);
        }
    }
}
```

**JSON пример (команда конфигурации от ROOT):**
```json
{
  "type": "config",
  "node_id": "ph_ec_001",
  "ph_target": 6.5,
  "ec_target": 1.8,
  "autonomous_enabled": true,
  "pump_pid": [
    {"kp": 1.5, "ki": 0.5, "kd": 0.1, "enabled": true},
    {"kp": 1.2, "ki": 0.4, "kd": 0.1, "enabled": true}
  ]
}
```

### Экспорт в JSON

```c
cJSON* node_config_export_to_json(const void *config, const char *node_type);
```

Экспортирует конфигурацию в JSON (для отправки на сервер).

**Пример:**
```c
cJSON *json = node_config_export_to_json(&config, "ph_ec");
char *json_str = cJSON_PrintUnformatted(json);

// Отправка на ROOT
mesh_manager_send_to_root((uint8_t*)json_str, strlen(json_str));

free(json_str);
cJSON_Delete(json);
```

### Значения по умолчанию

```c
esp_err_t node_config_reset_to_default(void *config, const char *node_type);
```

Устанавливает значения по умолчанию для указанного типа узла.

**Пример:**
```c
ph_ec_node_config_t config;
node_config_reset_to_default(&config, "ph_ec");

// Теперь config содержит:
// - ph_target = 6.8
// - ec_target = 2.0
// - ph_min = 6.0, ph_max = 7.5
// - autonomous_enabled = true
// - и другие разумные значения
```

### Factory Reset

```c
esp_err_t node_config_erase_all(void);
```

Полностью очищает NVS (сброс к заводским настройкам).

## Типичные сценарии использования

### 1. Первый запуск узла

```c
void app_main(void) {
    // Инициализация NVS
    node_config_init();
    
    // Попытка загрузить конфигурацию
    ph_ec_node_config_t config;
    esp_err_t err = node_config_load(&config, sizeof(config), "ph_ec_ns");
    
    if (err != ESP_OK) {
        // Первый запуск - используем значения по умолчанию
        ESP_LOGI(TAG, "First boot, using default config");
        node_config_reset_to_default(&config, "ph_ec");
        node_config_save(&config, sizeof(config), "ph_ec_ns");
    }
    
    // Применение конфигурации
    apply_config(&config);
}
```

### 2. Получение конфигурации от ROOT

```c
// NODE получает JSON от ROOT
void on_config_received(cJSON *json_config) {
    ph_ec_node_config_t config;
    
    // Загружаем текущую конфигурацию
    node_config_load(&config, sizeof(config), "ph_ec_ns");
    
    // Обновляем из JSON
    node_config_update_from_json(&config, json_config, "ph_ec");
    
    // Сохраняем в NVS (выживет при перезагрузке!)
    node_config_save(&config, sizeof(config), "ph_ec_ns");
    
    // Применяем
    apply_config(&config);
    
    ESP_LOGI(TAG, "Config updated: pH=%.2f, EC=%.2f", 
             config.ph_target, config.ec_target);
}
```

### 3. Отправка текущей конфигурации на сервер

```c
void send_current_config_to_server(void) {
    ph_ec_node_config_t config;
    node_config_load(&config, sizeof(config), "ph_ec_ns");
    
    // Экспорт в JSON
    cJSON *json = node_config_export_to_json(&config, "ph_ec");
    
    // Создание сообщения
    char buffer[2048];
    mesh_protocol_create_telemetry("ph_ec_001", json, buffer, sizeof(buffer));
    
    // Отправка
    mesh_manager_send_to_root((uint8_t*)buffer, strlen(buffer));
    
    cJSON_Delete(json);
}
```

### 4. Автономный режим (критично для pH/EC!)

```c
void check_mesh_connection(void) {
    static uint32_t last_mesh_contact = 0;
    uint32_t now = esp_timer_get_time() / 1000;
    
    if (mesh_manager_is_connected()) {
        last_mesh_contact = now;
    } else {
        // Mesh отключен
        if ((now - last_mesh_contact) > config.mesh_timeout_ms) {
            // Переход в автономный режим
            if (config.autonomous_enabled) {
                ESP_LOGW(TAG, "Mesh timeout, switching to autonomous mode");
                // Продолжаем работу с текущей конфигурацией из NVS
                autonomous_mode = true;
            } else {
                ESP_LOGE(TAG, "Mesh timeout, autonomous mode disabled!");
            }
        }
    }
}
```

## Важные замечания

### ⚠️ Правило #4 из плана

**Каждый узел ОБЯЗАН хранить конфигурацию в NVS!**

```c
// ❌ ПЛОХО - потеряется при перезагрузке
config.ph_target = 6.5;

// ✅ ХОРОШО - выживет при перезагрузке
config.ph_target = 6.5;
node_config_save(&config, sizeof(config), "ph_ec_ns");
```

### 🔒 Автономная работа pH/EC узла

pH/EC узел **ВСЕГДА** должен работать, даже если mesh/ROOT отключены!

```c
// При старте: загружаем последнюю сохраненную конфигурацию
node_config_load(&config, sizeof(config), "ph_ec_ns");

// Работаем с ней независимо от mesh
run_pid_control(&config);
```

### 📝 Версионность

Каждое обновление конфигурации увеличивает `config_version`:

```c
ESP_LOGI(TAG, "Config version: %d, last updated: %llu", 
         config.base.config_version, 
         config.base.last_updated);
```

### 🔄 Namespace для разных узлов

Используйте уникальные namespace для каждого типа узла:

```c
// pH/EC узел
node_config_save(&ph_config, sizeof(ph_config), "ph_ec_ns");

// Climate узел
node_config_save(&climate_config, sizeof(climate_config), "climate_ns");

// Relay узел
node_config_save(&relay_config, sizeof(relay_config), "relay_ns");
```

## Тестирование

### Проверка сохранения/загрузки

```c
void test_config_save_load(void) {
    ph_ec_node_config_t config1, config2;
    
    // Устанавливаем значения
    node_config_reset_to_default(&config1, "ph_ec");
    config1.ph_target = 7.2;
    config1.ec_target = 2.5;
    
    // Сохраняем
    node_config_save(&config1, sizeof(config1), "test_ns");
    
    // Загружаем в другую структуру
    node_config_load(&config2, sizeof(config2), "test_ns");
    
    // Проверяем
    assert(config2.ph_target == 7.2);
    assert(config2.ec_target == 2.5);
    
    ESP_LOGI(TAG, "Config save/load test: PASSED");
}
```

## Размер в памяти

| Тип узла | Размер структуры |
|----------|------------------|
| pH/EC    | ~300 байт        |
| Climate  | ~80 байт         |
| Relay    | ~120 байт        |
| Water    | ~60 байт         |

NVS эффективно сжимает данные, фактический размер в flash меньше.

## Связанные компоненты

- `mesh_manager` - отправка/прием данных через mesh
- `mesh_protocol` - парсинг JSON сообщений
- `node_registry` (ROOT) - реестр всех узлов и их конфигураций

## См. также

- `node_config.h` - полная документация API
- `doc/MESH_AI_QUICK_START.md` - быстрый старт
- `doc/MESH_HYDRO_V2_FINAL_PLAN.md` - полный план проекта

