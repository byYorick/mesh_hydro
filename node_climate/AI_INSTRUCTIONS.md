# 🤖 ПОЛНАЯ ИНСТРУКЦИЯ ДЛЯ ИИ: NODE Climate

**Узел мониторинга климата (температура, влажность, CO2, освещенность)**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**NODE Climate (ESP32)** - узел мониторинга климатических условий в теплице.

### 🌡️ Ключевые функции:

1. **Чтение датчиков** - SHT3x (temp+hum), CCS811 (CO2), Lux sensor
2. **Отправка телеметрии** - каждые 5 секунд на ROOT (в DEBUG режиме)
3. **Heartbeat** - каждые 5 секунд подтверждение работоспособности
4. **Discovery** - автоматическое обнаружение на сервере
5. **Обработка команд** - от ROOT/Server

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ❌ **НЕТ ИСПОЛНИТЕЛЕЙ!** - Climate node ТОЛЬКО читает датчики, НЕ управляет реле/насосами
2. ❌ **НЕТ ЛОГИКИ УПРАВЛЕНИЯ!** - решения принимает ROOT или SERVER
3. ✅ **ТОЛЬКО МОНИТОРИНГ** - температура, влажность, CO2, lux
4. ✅ **РЕЖИМ NODE** - НЕ голосует за ROOT, подключается к mesh AP 'HYDRO1'
5. ✅ **RETRY ЛОГИКА** - 3 попытки при ошибках датчиков
6. ✅ **DEGRADED MODE** - работает даже если все датчики не подключены

---

## 📦 ДАТЧИКИ

| Датчик | Интерфейс | I2C адрес | Назначение | Диапазон |
|--------|-----------|-----------|------------|----------|
| **SHT3x** | I2C | 0x44 | Температура | -40...+125°C |
| **SHT3x** | I2C | 0x44 | Влажность | 0...100% |
| **CCS811** | I2C | 0x5A | CO2 | 400...8192 ppm |
| **CCS811** | I2C | 0x5A | TVOC | 0...1187 ppb |
| **Trema Lux** | I2C | 0x12 | Освещенность | 0...65535 lux |

---

## 🔌 РАСПИНОВКА ESP32

| GPIO | Назначение | Примечание |
|------|------------|------------|
| **I2C шина:** | | |
| 17 | I2C SCL | Clock для всех датчиков |
| 18 | I2C SDA | Data для всех датчиков |
| **Питание датчиков:** | | |
| 3.3V | VCC | Питание всех I2C датчиков |
| GND | GND | Общий |

---

## 🏗️ АРХИТЕКТУРА

```
NODE Climate (ESP32)
│
├── Hardware Layer
│   ├── SHT3x driver → temperature, humidity
│   ├── CCS811 driver → CO2, TVOC
│   ├── Lux sensor driver → illuminance
│   └── I2C bus (GPIO 17=SCL, 18=SDA)
│
├── Controller Layer
│   ├── climate_controller → main logic
│   ├── Sensor reading task (every 5 sec)
│   ├── Heartbeat task (every 5 sec)
│   └── Command handler
│
├── Mesh Layer (NODE mode)
│   ├── mesh_manager (common component)
│   ├── Auto-connect to ROOT 'HYDRO1'
│   ├── Send telemetry → ROOT
│   ├── Receive commands ← ROOT
│   └── RSSI monitoring
│
└── Protocol Layer
    ├── mesh_protocol (JSON format)
    ├── Telemetry messages
    ├── Heartbeat messages
    └── Command parsing
```

---

## 📨 ТИПЫ СООБЩЕНИЙ

### 1. 📤 TELEMETRY (NODE → ROOT → MQTT → Server)

**Формат:**
```json
{
  "type": "telemetry",
  "node_id": "climate_001",
  "timestamp": 1729346400,
  "data": {
    "temperature": 24.5,
    "humidity": 65.2,
    "co2": 820,
    "lux": 450,
    "rssi_to_parent": -42
  }
}
```

**Код отправки:**
```c
static void send_telemetry(float temp, float humidity, uint16_t co2, uint16_t lux) {
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "temperature", temp);
    cJSON_AddNumberToObject(data, "humidity", humidity);
    cJSON_AddNumberToObject(data, "co2", co2);
    cJSON_AddNumberToObject(data, "lux", lux);
    cJSON_AddNumberToObject(data, "rssi_to_parent", get_rssi_to_parent());

    char json_buf[512];
    mesh_protocol_create_telemetry(s_config->base.node_id, data, json_buf, sizeof(json_buf));
    mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
    
    cJSON_Delete(data);
}
```

**Интервал:** 5 секунд (DEBUG режим), 30 секунд (PRODUCTION)

---

### 2. 💓 HEARTBEAT (NODE → ROOT → MQTT → Server)

**Формат:**
```json
{
  "type": "heartbeat",
  "node_id": "climate_001",
  "uptime": 3600,
  "heap_free": 158044,
  "rssi_to_parent": -44
}
```

**Интервал:** 5 секунд

---

### 3. 📥 COMMAND (Server → MQTT → ROOT → NODE)

**Топик:** `hydro/command/climate_001`

**Формат:**
```json
{
  "type": "command",
  "node_id": "climate_001",
  "command": "set_read_interval",
  "params": {
    "interval_ms": 30000
  }
}
```

**Обработка:**
```c
void climate_controller_handle_command(const char *command, cJSON *params) {
    if (strcmp(command, "set_read_interval") == 0) {
        cJSON *interval = cJSON_GetObjectItem(params, "interval_ms");
        if (interval && cJSON_IsNumber(interval)) {
            s_config->read_interval_ms = (uint32_t)interval->valueint;
            node_config_save(s_config, sizeof(climate_node_config_t), "climate_ns");
        }
    }
}
```

**Поддерживаемые команды:**
- `set_read_interval` - изменение интервала чтения датчиков

---

### 4. ⚙️ CONFIG (Server → MQTT → ROOT → NODE)

**Топик:** `hydro/config/climate_001`

**Формат:**
```json
{
  "type": "config",
  "node_id": "climate_001",
  "config": {
    "read_interval_ms": 30000,
    "temp_target": 24.0,
    "humidity_target": 65.0,
    "co2_target": 800,
    "lux_target": 500
  }
}
```

**Обработка:**
```c
void climate_controller_handle_config_update(cJSON *config_json) {
    // Обновление конфигурации из JSON
    // Сохранение в NVS
    node_config_save(s_config, sizeof(climate_node_config_t), "climate_ns");
}
```

---

### 5. 🔍 DISCOVERY (NODE → ROOT → MQTT → Server)

**Формат:**
```json
{
  "type": "discovery",
  "node_id": "climate_001",
  "node_type": "climate",
  "mac_address": "00:4B:12:37:D5:A4",
  "firmware": "1.0.0",
  "hardware": "ESP32",
  "sensors": ["sht3x", "ccs811", "lux"],
  "heap_free": 158044,
  "wifi_rssi": -42
}
```

**Когда отправляется:**
- При первом подключении к mesh
- Периодически (TODO: добавить)

---

## 🔗 КОМПОНЕНТЫ

### Из `common/`:
| Компонент | Назначение |
|-----------|------------|
| `mesh_manager` | Управление ESP-WIFI-MESH (NODE режим) |
| `mesh_protocol` | JSON протокол обмена |
| `node_config` | NVS конфигурация |

### Собственные `components/`:
| Компонент | Назначение |
|-----------|------------|
| `climate_controller` | Главная логика (чтение + отправка) |
| `sht3x_driver` | Драйвер SHT3x |
| `ccs811_driver` | Драйвер CCS811 |
| `lux_sensor` | Драйвер Lux sensor |

---

## 🚀 КОД main/app_main.c

### Инициализация (7 шагов):

```c
void app_main(void) {
    // Шаг 1: NVS
    nvs_flash_init();
    
    // Шаг 2: Загрузка конфигурации
    node_config_load(&g_config, sizeof(g_config), "climate_ns");
    
    // Шаг 3: I2C (GPIO 17=SCL, 18=SDA)
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 18,
        .scl_io_num = 17,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(0, &i2c_conf);
    i2c_driver_install(0, I2C_MODE_MASTER, 0, 0, 0);
    
    // Шаг 4: Датчики
    sht3x_init(0, 0x44);
    ccs811_init(0, 0x5A);
    lux_sensor_init(0, 0x12);
    
    // Шаг 5: Mesh (NODE режим)
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
    mesh_manager_init(&mesh_config);
    mesh_manager_register_recv_cb(on_mesh_data_received);  // ← Callback для команд
    mesh_manager_start();
    
    // Шаг 6-7: Запуск контроллера
    climate_controller_init(&g_config);
    climate_controller_start();
}
```

### Callback для команд от ROOT:

```c
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    
    // Парсинг JSON
    if (!mesh_protocol_parse((const char *)data, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        return;
    }

    // Проверка что сообщение для нас
    if (strcmp(msg.node_id, g_config.base.node_id) != 0) {
        mesh_protocol_free_message(&msg);
        return;
    }

    // Обработка по типу
    switch (msg.type) {
        case MESH_MSG_COMMAND: {
            cJSON *cmd = cJSON_GetObjectItem(msg.data, "command");
            if (cmd && cJSON_IsString(cmd)) {
                climate_controller_handle_command(cmd->valuestring, msg.data);
            }
            break;
        }

        case MESH_MSG_CONFIG:
            climate_controller_handle_config_update(msg.data);
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }

    mesh_protocol_free_message(&msg);
}
```

---

## ⚙️ КОНФИГУРАЦИЯ

### mesh_config.h (общий для всех узлов):
```c
#define MESH_NETWORK_ID         "HYDRO1"
#define MESH_NETWORK_PASSWORD   "hydro_mesh_2025"
#define MESH_NETWORK_CHANNEL    7  // ← Фиксированный канал!

#define MESH_ROUTER_SSID        "Yorick"
#define MESH_ROUTER_PASSWORD    "pro100parol"
```

### node_config (в NVS):
```c
typedef struct {
    node_base_config_t base;  // node_id, zone
    uint32_t read_interval_ms;
    float temp_target;
    float humidity_target;
    uint16_t co2_target;
    uint16_t lux_target;
} climate_node_config_t;
```

---

## ✅ ЧТО ДЕЛАТЬ

### 1. ✅ Читай датчики с retry логикой

```c
esp_err_t read_all_sensors(float *temp, float *humidity, uint16_t *co2, uint16_t *lux) {
    const int max_retries = 3;
    
    // Читаем каждый датчик с retry
    for (int i = 0; i < max_retries; i++) {
        if (sht3x_read_temp_humidity(temp, humidity) == ESP_OK) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // То же для других датчиков...
    
    // Возвращаем ESP_OK если хотя бы один датчик прочитан
    return at_least_one_ok ? ESP_OK : ESP_FAIL;
}
```

### 2. ✅ Отправляй телеметрию каждые 5 секунд

```c
static void climate_main_task(void *arg) {
    while (1) {
        float temp, humidity;
        uint16_t co2, lux;
        
        if (read_all_sensors(&temp, &humidity, &co2, &lux) == ESP_OK) {
            send_telemetry(temp, humidity, co2, lux);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // DEBUG: 5 сек
    }
}
```

### 3. ✅ Отправляй heartbeat каждые 5 секунд

```c
static void heartbeat_task(void *arg) {
    vTaskDelay(pdMS_TO_TICKS(5000));  // Начальная задержка
    
    while (1) {
        if (mesh_manager_is_connected()) {
            send_heartbeat();
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### 4. ✅ Обрабатывай команды от ROOT

```c
void climate_controller_handle_command(const char *command, cJSON *params) {
    if (strcmp(command, "set_read_interval") == 0) {
        cJSON *interval = cJSON_GetObjectItem(params, "interval_ms");
        if (interval && cJSON_IsNumber(interval)) {
            s_config->read_interval_ms = interval->valueint;
            node_config_save(s_config, sizeof(climate_node_config_t), "climate_ns");
            ESP_LOGI(TAG, "Read interval updated: %d ms", s_config->read_interval_ms);
        }
    }
}
```

---

## ❌ ЧТО НЕ ДЕЛАТЬ

### 1. ❌ НЕ управляй исполнителями

```c
// ПЛОХО ❌
void process_temperature(float temp) {
    if (temp > 28.0f) {
        relay_turn_on(FAN_RELAY);  // ❌ Climate НЕ управляет реле!
    }
}

// ХОРОШО ✅
void process_temperature(float temp) {
    // Просто отправляем данные на ROOT
    send_telemetry(temp, ...);
    // ROOT или Server принимает решение
}
```

### 2. ❌ НЕ голосуй за ROOT

```c
// ПЛОХО ❌
mesh_config.mode = MESH_MODE_ROOT;  // ❌ Climate всегда NODE!

// ХОРОШО ✅
mesh_config.mode = MESH_MODE_NODE;  // ✅ Всегда NODE
```

### 3. ❌ НЕ подключайся к роутеру напрямую

```c
// ПЛОХО ❌
esp_wifi_set_mode(WIFI_MODE_STA);
esp_wifi_connect();  // ❌ NODE подключается к ROOT, не к роутеру!

// ХОРОШО ✅
// mesh_manager автоматически подключится к ROOT mesh AP
mesh_manager_start();  // ✅ Ищет mesh AP 'HYDRO1'
```

---

## 🔧 СБОРКА И ПРОШИВКА

### Windows (PowerShell):

```batch
cd node_climate
C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005
idf.py fullclean
idf.py build
idf.py -p COM10 flash monitor
```

### Проверка после прошивки:

**В логах должно быть:**
```
I (800) CLIMATE: [Step 5/7] Initializing Mesh (NODE mode)...
I (1060) mesh_manager: Mesh manager initialized (mode: NODE)
I (1063) mesh_manager: NODE mode: will connect to mesh AP 'HYDRO1'
I (5184) mesh_manager: ✓ MESH Parent connected!
I (5187) mesh_manager: Layer: 2
```

**НЕ должно быть:**
```
❌ "Node is currently root (voting)"
❌ "Connected to Yorick"
❌ "disable self-organizing, search parent<stop reconnect>"
```

---

## 🐛 ЧАСТЫЕ ОШИБКИ

### 1. ❌ `ESP_ERR_MESH_ARGUMENT`
**Причина:** Вызов mesh API до `mesh_manager_start()` или после `mesh_manager_stop()`
**Решение:** Проверяй `mesh_manager_is_connected()` перед отправкой

### 2. ❌ Датчики не инициализируются
**Причина:** Неправильный I2C адрес или GPIO пины
**Решение:** SDA=18, SCL=17, частота=100kHz

### 3. ❌ NODE подключается к роутеру вместо ROOT
**Причина:** Канал не совпадает или `router_ssid` не установлен
**Решение:** `MESH_NETWORK_CHANNEL=7` (фиксированный!) и router credentials обязательны

### 4. ❌ Heartbeat/telemetry не доходят до сервера
**Причина:** Мусор после JSON в буфере
**Решение:** `strlen(json_buf)` правильно определяет длину до '\0'

---

## 📊 МОНИТОРИНГ

### Лог здорового узла (каждые 5 сек):

```
I (5000) climate_ctrl: 💓 Heartbeat sent (uptime=100s, heap=158044B, RSSI=-42)
I (5000) climate_ctrl: 📊 Telemetry: 24.5°C, 65%, 820ppm, 450lux, RSSI=-42
I (10000) climate_ctrl: 💓 Heartbeat sent (uptime=105s, heap=158044B, RSSI=-39)
I (10000) climate_ctrl: 📊 Telemetry: 24.6°C, 65%, 825ppm, 455lux, RSSI=-39)
```

### Признаки проблем:

```
❌ "Mesh offline, telemetry skipped" - нет связи с ROOT
❌ "All sensors failed" - все датчики не отвечают
❌ "Failed to send telemetry" - mesh send failed
```

---

## 📚 MESH API (НЕ ВЫЗЫВАЙ НАПРЯМУЮ!)

**Используй только `mesh_manager` wrapper!**

| ❌ НЕ используй | ✅ Используй вместо |
|-----------------|---------------------|
| `esp_mesh_send()` | `mesh_manager_send_to_root()` |
| `esp_mesh_set_config()` | `mesh_manager_init()` |
| `esp_mesh_start()` | `mesh_manager_start()` |
| `esp_mesh_recv()` | `mesh_manager_register_recv_cb()` |

---

## 🔍 ОТЛАДКА

### Включение DEBUG логов:

```c
esp_log_level_set("climate_ctrl", ESP_LOG_DEBUG);
esp_log_level_set("mesh_manager", ESP_LOG_DEBUG);
esp_log_level_set("sht3x", ESP_LOG_DEBUG);
```

### Проверка RSSI к ROOT:

```c
int8_t rssi = mesh_manager_get_parent_rssi();
ESP_LOGI(TAG, "RSSI to ROOT: %d dBm", rssi);
// -30...-50 = отлично
// -50...-70 = нормально
// -70...-80 = слабо
```

### Проверка mesh layer:

```c
int layer = mesh_manager_get_layer();
ESP_LOGI(TAG, "Mesh layer: %d", layer);
// 1 = напрямую к ROOT ✅
// 2+ = через другие NODE
```

---

## 📋 CHECKLIST ДЛЯ ИИ

Перед коммитом проверь:

- [ ] `MESH_MODE_NODE` (НЕ ROOT!)
- [ ] Callback `on_mesh_data_received` зарегистрирован
- [ ] Команды обрабатываются (`handle_command`)
- [ ] Телеметрия отправляется каждые 5 сек
- [ ] Heartbeat отправляется каждые 5 сек
- [ ] Discovery отправляется при старте
- [ ] Retry логика для датчиков (3 попытки)
- [ ] Degraded mode (работает без датчиков)
- [ ] Stack sizes достаточны (8192+ для задач)
- [ ] Нет прямых вызовов ESP-MESH API

---

## 🎯 ИТОГОВАЯ АРХИТЕКТУРА СООБЩЕНИЙ

```
┌─────────────────┐
│  NODE Climate   │
└────────┬────────┘
         │
         │ 📤 TELEMETRY (every 5s)
         │ 💓 HEARTBEAT (every 5s)
         │ 🔍 DISCOVERY (on start)
         │
         ├─────────► mesh_manager_send_to_root()
         │
         ▼
┌─────────────────┐
│   ROOT NODE     │
│  data_router    │
└────────┬────────┘
         │
         │ ✅ Добавляет node_id в топик
         │
         ├─────────► mqtt_client_manager_publish()
         │
         ▼
┌─────────────────┐
│  MQTT Broker    │
│  192.168.1.100  │
└────────┬────────┘
         │
         │ hydro/telemetry/climate_001
         │ hydro/heartbeat/climate_001
         │
         ├─────────► MQTT Listener (Docker)
         │
         ▼
┌─────────────────┐
│  Laravel Backend│
│  MqttService    │
└────────┬────────┘
         │
         │ ✅ Парсит JSON
         │ ✅ Создаёт NODE (auto-discovery)
         │ ✅ Сохраняет telemetry в БД
         │
         ▼
┌─────────────────┐
│    Database     │
│   PostgreSQL    │
└─────────────────┘
```

---

## 🔄 ОБРАТНЫЙ ПУТЬ (КОМАНДЫ)

```
┌─────────────────┐
│   Dashboard     │  ← Пользователь нажимает "Изменить интервал"
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Laravel API    │  POST /api/nodes/climate_001/command
└────────┬────────┘
         │
         ├─────────► MqttService->publish()
         │
         ▼
┌─────────────────┐
│  MQTT Broker    │  hydro/command/climate_001
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   ROOT NODE     │  mqtt_client_manager_recv_cb()
│  data_router    │
└────────┬────────┘
         │
         │ ✅ Находит NODE в реестре
         │ ✅ Проверяет online
         │
         ├─────────► mesh_manager_send(node->mac_addr, ...)
         │
         ▼
┌─────────────────┐
│  NODE Climate   │  on_mesh_data_received()
│  app_main.c     │
└────────┬────────┘
         │
         ├─────────► climate_controller_handle_command()
         │
         ▼
    ✅ Обновление config в RAM
    ✅ Сохранение в NVS
    ✅ Применение новых настроек
```

---

## 🧪 ТЕСТИРОВАНИЕ

### Тест отправки команды через MQTT:

```bash
# На хосте (где mosquitto_pub доступен):
mosquitto_pub -h 192.168.1.100 -t hydro/command/climate_001 -m '{
  "type": "command",
  "node_id": "climate_001",
  "command": "set_read_interval",
  "params": {"interval_ms": 15000}
}'
```

**Ожидаемый результат:**
```
I (50000) CLIMATE: Message from ROOT: type=1
I (50000) climate_ctrl: Command received: set_read_interval
I (50000) climate_ctrl: Read interval updated: 15000 ms
```

---

## 🏆 КРИТЕРИИ УСПЕХА

**NODE Climate работает правильно если:**

1. ✅ Подключается к ROOT (layer 2), НЕ к роутеру
2. ✅ Отправляет heartbeat каждые 5 сек
3. ✅ Отправляет telemetry каждые 5 сек
4. ✅ Появляется на дашборде как "climate_001 (ONLINE)"
5. ✅ Обрабатывает команды от сервера
6. ✅ RSSI к ROOT в пределах -30...-60 dBm
7. ✅ Heap стабильный (~158000 байт)
8. ✅ Работает в degraded mode без датчиков

---

**ГОТОВО! Climate NODE - ТОЛЬКО МОНИТОРИНГ, БЕЗ УПРАВЛЕНИЯ!** 🌡️📊

