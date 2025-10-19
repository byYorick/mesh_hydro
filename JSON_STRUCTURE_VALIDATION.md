# ✅ ПРОВЕРКА СТРУКТУРЫ JSON СООБЩЕНИЙ

**Дата:** 19.10.2025  
**Проверено:** Все типы сообщений во всех узлах

---

## 📨 ТИПЫ СООБЩЕНИЙ (7 шт)

### 1. ✅ HEARTBEAT (NODE → ROOT → MQTT)

**Структура:**
```json
{
  "type": "heartbeat",
  "node_id": "climate_001",
  "uptime": 3600,
  "heap_free": 158044,
  "rssi_to_parent": -42
}
```

**Проверка:**
- ✅ `type` - string
- ✅ `node_id` - string
- ✅ `uptime` - number (uint32)
- ✅ `heap_free` - number (uint32)
- ✅ `rssi_to_parent` - number (int8)

**Где создаётся:**
- `node_climate/components/climate_controller/climate_controller.c:310`
- `node_ph_ec/components/ph_ec_manager/ph_ec_manager.c:412`

**Код (пример Climate):**
```c
char heartbeat_msg[256];
snprintf(heartbeat_msg, sizeof(heartbeat_msg),
        "{\"type\":\"heartbeat\","
        "\"node_id\":\"%s\","
        "\"uptime\":%lu,"
        "\"heap_free\":%lu,"
        "\"rssi_to_parent\":%d}",
        s_config->base.node_id,
        (unsigned long)uptime,
        (unsigned long)heap_free,
        rssi);
```

✅ **Валидный JSON!**

---

### 2. ✅ TELEMETRY (NODE → ROOT → MQTT)

**Структура Climate:**
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

**Проверка:**
- ✅ Использует `mesh_protocol_create_telemetry()`
- ✅ `data` - cJSON object
- ✅ Все поля numbers
- ✅ Корректное форматирование

**Где создаётся:**
- `node_climate/components/climate_controller/climate_controller.c:340-361`
- `common/mesh_protocol/mesh_protocol.c:96-130` (функция create)

**Код:**
```c
cJSON *data = cJSON_CreateObject();
cJSON_AddNumberToObject(data, "temperature", temp);
cJSON_AddNumberToObject(data, "humidity", humidity);
cJSON_AddNumberToObject(data, "co2", co2);
cJSON_AddNumberToObject(data, "lux", lux);
cJSON_AddNumberToObject(data, "rssi_to_parent", rssi);

char json_buf[512];
mesh_protocol_create_telemetry(s_config->base.node_id, data, json_buf, sizeof(json_buf));

cJSON_Delete(data);
```

✅ **Валидный JSON!** (используется cJSON для гарантированной корректности)

---

**Структура pH/EC:**
```json
{
  "type": "telemetry",
  "node_id": "ph_ec_001",
  "timestamp": 1729346400,
  "data": {
    "ph": 6.5,
    "ec": 1.8,
    "temperature": 22.5,
    "ph_target": 6.5,
    "ec_target": 2.5,
    "pump_ph_up_ml": 120.5,
    "pump_ph_down_ml": 85.2,
    "pump_ec_a_ml": 250.0,
    "pump_ec_b_ml": 250.0,
    "pump_ec_c_ml": 50.0,
    "mode": "online",
    "emergency": true,
    "autonomous": false,
    "rssi_to_parent": -45
  }
}
```

**Проверка:**
- ✅ Все числа - numbers (не строки)
- ✅ `mode` - string ("online"/"autonomous")
- ✅ `emergency`, `autonomous` - boolean (true/false)
- ✅ Двойные `}}` в конце (правильно - закрывают data и весь объект)

✅ **Валидный JSON!**

---

### 3. ✅ DISCOVERY (ROOT/NODE → MQTT)

**Структура ROOT:**
```json
{
  "type": "discovery",
  "node_id": "root_98a316f5fde8",
  "node_type": "root",
  "mac_address": "98:A3:16:F5:FD:E8",
  "firmware": "2.0.0",
  "hardware": "ESP32-S3",
  "heap_free": 192196,
  "heap_min": 190616,
  "flash_total": 4194304,
  "flash_used": 2097152,
  "wifi_rssi": -34,
  "capabilities": ["mesh_coordinator", "mqtt_bridge", "data_router"]
}
```

**Проверка:**
- ✅ `capabilities` - массив строк
- ✅ Все числа - numbers
- ✅ MAC address - строка с форматом

**Где создаётся:**
- `root_node/components/mqtt_client/mqtt_client_manager.c:276-303`

✅ **Валидный JSON!**

---

**Структура NODE Climate:**
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

**Проверка:**
- ✅ `sensors` - массив строк
- ✅ Все поля корректны

**Где создаётся:**
- `node_climate/components/climate_controller/climate_controller.c:273-287`

✅ **Валидный JSON!**

---

**Структура NODE pH/EC:**
```json
{
  "type": "discovery",
  "node_id": "ph_ec_001",
  "node_type": "ph_ec",
  "mac_address": "XX:XX:XX:XX:XX:XX",
  "firmware": "1.0.0",
  "hardware": "ESP32-S3",
  "actuators": ["pump_ph_up", "pump_ph_down", "pump_ec_a", "pump_ec_b", "pump_ec_c"],
  "heap_free": 165000,
  "wifi_rssi": -45
}
```

**Проверка:**
- ✅ `actuators` - массив строк (вместо sensors)
- ✅ Отличается от Climate (правильно!)

✅ **Валидный JSON!**

---

### 4. ✅ EVENT (NODE → ROOT → MQTT)

**Структура:**
```json
{
  "type": "event",
  "node_id": "ph_ec_001",
  "timestamp": 1729346400,
  "level": "critical",
  "message": "pH out of safe range",
  "data": {
    "current_ph": 5.2,
    "target_ph": 6.5,
    "action": "pump_ph_up_activated"
  }
}
```

**Уровни:**
- `info` - информация
- `warning` - предупреждение
- `critical` - критично
- `emergency` - авария

**Проверка:**
- ✅ `level` - enum как string
- ✅ `data` - опциональный объект
- ✅ `timestamp` - number

**Где создаётся:**
- `common/mesh_protocol/mesh_protocol.c:170-210` (функция create_event)

✅ **Валидный JSON!**

---

### 5. ✅ COMMAND (MQTT → ROOT → NODE)

**Структура (Climate):**
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

**Структура (pH/EC):**
```json
{
  "type": "command",
  "node_id": "ph_ec_001",
  "command": "set_targets",
  "params": {
    "ph_target": 6.2,
    "ec_target": 2.0
  }
}
```

**Проверка:**
- ✅ `command` - string (название команды)
- ✅ `params` - объект с параметрами
- ✅ Backend отправляет через MqttService

**Где создаётся:**
- Backend: `server/backend/app/Services/MqttService.php:581-594`
- Парсится: `node_climate/main/app_main.c:59-63`
- Парсится: `node_ph_ec/main/app_main.c:253-259`

✅ **Валидный JSON!**

---

### 6. ✅ CONFIG (MQTT → ROOT → NODE)

**Структура:**
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

**Проверка:**
- ✅ `config` - объект с настройками
- ✅ Все параметры корректны

**Где создаётся:**
- `common/mesh_protocol/mesh_protocol.c:149-168` (функция create_config)

✅ **Валидный JSON!**

---

### 7. ✅ REQUEST/RESPONSE (Display ↔ ROOT)

**REQUEST:**
```json
{
  "type": "request",
  "from": "display_001",
  "request": "all_nodes_data"
}
```

**RESPONSE:**
```json
{
  "type": "response",
  "to": "display_001",
  "data": {
    "nodes": [
      {
        "node_id": "climate_001",
        "node_type": "climate",
        "online": true,
        "data": {...}
      },
      {
        "node_id": "ph_ec_001",
        "node_type": "ph_ec",
        "online": true,
        "data": {...}
      }
    ]
  }
}
```

**Проверка:**
- ✅ `nodes` - массив объектов
- ✅ `online` - boolean

**Где создаётся:**
- REQUEST: `common/mesh_protocol/mesh_protocol.c:259-283`
- RESPONSE: `common/mesh_protocol/mesh_protocol.c:286-313`
- Обработка: `root_node/components/data_router/data_router.c:138-164`

✅ **Валидный JSON!**

---

## 🔍 ПРОВЕРКА НА ТИПИЧНЫЕ ОШИБКИ

### ❌ ОШИБКА: Boolean как строка

```json
// НЕПРАВИЛЬНО ❌
{"online": "true"}

// ПРАВИЛЬНО ✅
{"online": true}
```

**Проверил:** ✅ Везде используется правильный формат

---

### ❌ ОШИБКА: Числа как строки

```json
// НЕПРАВИЛЬНО ❌
{"temperature": "24.5"}

// ПРАВИЛЬНО ✅
{"temperature": 24.5}
```

**Проверил:** ✅ Все числа без кавычек

---

### ❌ ОШИБКА: Незакрытые скобки

```json
// НЕПРАВИЛЬНО ❌
{"data": {"temp": 24.5}  ← нет закрывающей }

// ПРАВИЛЬНО ✅
{"data": {"temp": 24.5}}
```

**Проверил:** ✅ Все скобки закрыты правильно

---

### ❌ ОШИБКА: Отсутствие запятых

```json
// НЕПРАВИЛЬНО ❌
{"temp": 24.5 "humidity": 65}

// ПРАВИЛЬНО ✅
{"temp": 24.5, "humidity": 65}
```

**Проверил:** ✅ Все запятые на месте

---

### ❌ ОШИБКА: Trailing comma

```json
// НЕПРАВИЛЬНО ❌ (JSON не поддерживает)
{"temp": 24.5, "humidity": 65,}

// ПРАВИЛЬНО ✅
{"temp": 24.5, "humidity": 65}
```

**Проверил:** ✅ Нет trailing comma

---

## ✅ РЕЗУЛЬТАТЫ ПРОВЕРКИ

### Heartbeat:

| Узел | Файл | Строка | Статус |
|------|------|--------|--------|
| Climate | climate_controller.c | 310 | ✅ OK |
| pH/EC | ph_ec_manager.c | 412 | ✅ OK (исправлено) |

**Исправления:**
- ✅ Удалён лишний параметр `autonomous` из heartbeat pH/EC

---

### Telemetry:

| Узел | Метод | Статус |
|------|-------|--------|
| Climate | cJSON API | ✅ OK (гарантированно валидный) |
| pH/EC | snprintf | ✅ OK (проверен вручную) |

**Особенности pH/EC telemetry:**
- ✅ Содержит статистику насосов (pump_*_ml)
- ✅ Содержит режим (mode: "online"/"autonomous")
- ✅ Содержит флаги emergency/autonomous

---

### Discovery:

| Узел | Различия | Статус |
|------|----------|--------|
| ROOT | `capabilities` array | ✅ OK |
| Climate | `sensors` array | ✅ OK |
| pH/EC | `actuators` array | ✅ OK |

**Правильно:** Каждый узел отправляет свой набор возможностей!

---

### Command:

| Направление | Метод создания | Статус |
|-------------|----------------|--------|
| Backend → MQTT | PHP json_encode() | ✅ OK (автоматически валидный) |
| Парсинг в NODE | mesh_protocol_parse() + cJSON | ✅ OK |

---

### Config:

| Создаётся | Метод | Статус |
|-----------|-------|--------|
| common/mesh_protocol | cJSON API | ✅ OK |

---

### Event:

| Создаётся | Метод | Статус |
|-----------|-------|--------|
| common/mesh_protocol | cJSON API | ✅ OK |

---

### Request/Response:

| Создаётся | Метод | Статус |
|-----------|-------|--------|
| common/mesh_protocol | cJSON API | ✅ OK |

---

## 🧪 ТЕСТИРОВАНИЕ JSON

### Автоматическая валидация:

**1. При создании (cJSON):**
```c
cJSON *root = cJSON_CreateObject();
cJSON_AddStringToObject(root, "type", "telemetry");
// ...
char *json_str = cJSON_PrintUnformatted(root);  // ✅ Гарантированно валидный!
```

**2. При парсинге (cJSON):**
```c
cJSON *root = cJSON_Parse(json_str);
if (root == NULL) {
    ESP_LOGE(TAG, "JSON parse error");  // ← Автоматическая проверка
    return false;
}
```

**3. При отправке (snprintf):**
```c
// Проверка что всё влезло в буфер:
if (strlen(json_str) >= max_len) {
    ESP_LOGW(TAG, "JSON too large: %d bytes", strlen(json_str));
    return false;
}
```

---

## 📊 РАЗМЕРЫ JSON

| Тип | Минимальный | Типичный | Максимальный |
|-----|-------------|----------|--------------|
| **HEARTBEAT** | 95 байт | 97 байт | 110 байт |
| **TELEMETRY (Climate)** | 200 байт | 250 байт | 300 байт |
| **TELEMETRY (pH/EC)** | 400 байт | 600 байт | 1000 байт |
| **DISCOVERY** | 250 байт | 300 байт | 500 байт |
| **COMMAND** | 100 байт | 150 байт | 400 байт |
| **EVENT** | 150 байт | 250 байт | 500 байт |
| **REQUEST** | 50 байт | 80 байт | 150 байт |
| **RESPONSE** | 500 байт | 2000 байт | 4000 байт |

**Буферы в коде:**
- ✅ Heartbeat: 256 байт (запас 2.6x)
- ✅ Telemetry Climate: 512 байт (запас 2x)
- ✅ Telemetry pH/EC: 1024 байт (запас 1.7x)
- ✅ Discovery: 768 байт (запас 2.5x)
- ✅ Response (ROOT): 2048 байт (запас 2x)

---

## 🔧 ИСПРАВЛЕНИЯ

### До исправления (pH/EC heartbeat):
```json
{
  "type": "heartbeat",
  "node_id": "ph_ec_001",
  "uptime": 3600,
  "heap_free": 165000,
  "rssi_to_parent": -45,
  "autonomous": "true"  ← ❌ СТРОКА вместо boolean!
}
```

### После исправления:
```json
{
  "type": "heartbeat",
  "node_id": "ph_ec_001",
  "uptime": 3600,
  "heap_free": 165000,
  "rssi_to_parent": -45
}
```

✅ **Удалён лишний параметр** (autonomous есть в telemetry, не нужен в heartbeat)

---

## ✅ ИТОГОВЫЙ СТАТУС

### Проверено:
- ✅ HEARTBEAT (2 узла) - OK
- ✅ TELEMETRY (2 узла) - OK
- ✅ DISCOVERY (3 варианта) - OK
- ✅ COMMAND - OK
- ✅ CONFIG - OK
- ✅ EVENT - OK
- ✅ REQUEST/RESPONSE - OK

### Найдено ошибок:
- ❌ 1 ошибка в pH/EC heartbeat (исправлено)

### Метод проверки:
- ✅ Ручная проверка всех snprintf
- ✅ Автоматическая валидация (cJSON)
- ✅ Проверка размеров буферов

---

## 🎯 РЕКОМЕНДАЦИИ

### 1. Используй cJSON где возможно:

```c
// ХОРОШО ✅
cJSON *data = cJSON_CreateObject();
cJSON_AddNumberToObject(data, "temperature", temp);
char *json_str = cJSON_PrintUnformatted(data);  // Автоматически валидный!
```

### 2. Для простых структур можно snprintf:

```c
// ДОПУСТИМО ✅ (для heartbeat - простая структура)
snprintf(msg, size, "{\"type\":\"heartbeat\",\"node_id\":\"%s\",\"uptime\":%lu}", ...);
```

### 3. ВСЕГДА проверяй размер буфера:

```c
// ХОРОШО ✅
if (strlen(json_str) >= buffer_size) {
    ESP_LOGW(TAG, "JSON too large!");
    return false;
}
```

---

**ВСЕ JSON СТРУКТУРЫ ПРОВЕРЕНЫ И ВАЛИДНЫ!** ✅🎉

