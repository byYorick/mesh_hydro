# 🚀 БЫСТРАЯ СПРАВКА: JSON ФОРМАТЫ ВСЕХ НОД

## 📤 NODE → ROOT (отправка)

### 1. DISCOVERY (при старте)
```json
{
  "type": "discovery",
  "node_id": "climate_001",
  "node_type": "climate",          // ✅ ОБЯЗАТЕЛЬНО
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "firmware": "1.0.0",
  "hardware": "ESP32",
  "sensors": ["sht3x", "ccs811"],
  "heap_free": 123456,
  "wifi_rssi": -45
}
```

### 2. TELEMETRY (каждые 5-30 сек)
```json
{
  "type": "telemetry",
  "node_id": "climate_001",
  "node_type": "climate",          // ✅ ОБЯЗАТЕЛЬНО
  "timestamp": 1729500000,
  "data": {
    "temperature": 24.5,
    "humidity": 65.0,
    "rssi_to_parent": -45
  }
}
```

### 3. HEARTBEAT (каждые 5-10 сек)
```json
{
  "type": "heartbeat",
  "node_id": "climate_001",
  "node_type": "climate",          // ✅ ОБЯЗАТЕЛЬНО
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "timestamp": 1729500000,
  "uptime": 3600,
  "heap_free": 123456,
  "rssi_to_parent": -45
}
```

### 4. EVENT (критичные события)
```json
{
  "type": "event",
  "node_id": "ph_001",
  "node_type": "ph",               // ✅ ОБЯЗАТЕЛЬНО (добавляется автоматически)
  "level": "critical",             // info, warning, critical, emergency
  "message": "pH critical level",
  "timestamp": 1729500000,
  "data": {
    "ph": 4.2,
    "ph_target": 6.0
  }
}
```

---

## 📥 ROOT → NODE (получение)

### 5. COMMAND
```json
{
  "type": "command",
  "node_id": "ph_001",
  "command": "set_ph_target",
  "params": {
    "value": 6.2
  }
}
```

### 6. CONFIG UPDATE
```json
{
  "type": "config",
  "node_id": "ph_001",
  "config": {
    "ph_target": 6.2,
    "ph_min": 5.8,
    "ph_max": 6.5
  }
}
```

---

## 🎯 ТИПЫ НОД (`node_type`)

| Type | Описание | Датчики/Актуаторы |
|------|----------|-------------------|
| `climate` | Климатический узел | SHT3x, CCS811, Lux |
| `ph` | pH узел | pH sensor, 2 насоса (UP/DOWN) |
| `ec` | EC узел | EC sensor, 3 насоса (A/B/C) |
| `ph_ec` | Объединенный pH+EC | pH+EC sensors, 5 насосов |
| `display` | Дисплей | TFT LCD + энкодер |
| `relay` | Реле/актуаторы | ⚠️ TODO |
| `water` | Вода | ⚠️ TODO |

---

## 📋 ПРИМЕРЫ ПО ТИПАМ НОД

### CLIMATE
```json
{
  "type": "telemetry",
  "node_id": "climate_001",
  "node_type": "climate",
  "data": {
    "temperature": 24.5,
    "humidity": 65.0,
    "co2": 850,
    "lux": 450,
    "rssi_to_parent": -45
  }
}
```

### PH
```json
{
  "type": "telemetry",
  "node_id": "ph_001",
  "node_type": "ph",
  "data": {
    "ph": 6.2,
    "ph_target": 6.0,
    "pump_ph_up_ml": 125.5,
    "pump_ph_down_ml": 89.2,
    "autonomous": true,
    "rssi_to_parent": -50
  }
}
```

### EC
```json
{
  "type": "telemetry",
  "node_id": "ec_001",
  "node_type": "ec",
  "data": {
    "ec": 2.1,
    "ec_target": 2.0,
    "pump_ec_a_ml": 450.0,
    "pump_ec_b_ml": 360.0,
    "pump_ec_c_ml": 90.0,
    "autonomous": true,
    "rssi_to_parent": -48
  }
}
```

### DISPLAY
```json
{
  "type": "heartbeat",
  "node_id": "display_001",
  "node_type": "display",
  "uptime": 7200,
  "heap_free": 150000,
  "rssi_to_parent": -42
}
```

---

## ⚙️ КАК ИСПОЛЬЗОВАТЬ

### В коде ноды (C):

#### TELEMETRY:
```c
cJSON *data = cJSON_CreateObject();
cJSON_AddNumberToObject(data, "temperature", 24.5);

char json_buf[512];
mesh_protocol_create_telemetry(
    s_config->base.node_id, 
    "climate",              // ✅ node_type
    data,
    json_buf, 
    sizeof(json_buf)
);

mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
cJSON_Delete(data);
```

#### HEARTBEAT (manual):
```c
cJSON *root = cJSON_CreateObject();
cJSON_AddStringToObject(root, "type", "heartbeat");
cJSON_AddStringToObject(root, "node_id", "climate_001");
cJSON_AddStringToObject(root, "node_type", "climate");  // ✅
cJSON_AddNumberToObject(root, "uptime", 3600);

char *json_str = cJSON_PrintUnformatted(root);
mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
free(json_str);
cJSON_Delete(root);
```

#### EVENT:
```c
cJSON *data = cJSON_CreateObject();
cJSON_AddNumberToObject(data, "ph", 4.2);

char json_buf[512];
mesh_protocol_create_event(
    s_config->base.node_id,
    MESH_EVENT_CRITICAL,
    "pH critical level",
    data,
    json_buf,
    sizeof(json_buf)
);

mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
cJSON_Delete(data);
```

---

## 🔍 BACKEND (PHP - MqttService.php)

### Определение типа ноды:
```php
private function detectNodeType($nodeId, $data = null)
{
    // 1. По префиксу ID
    if (str_starts_with($nodeId, 'climate_')) return 'climate';
    if (str_starts_with($nodeId, 'ph_')) return 'ph';
    if (str_starts_with($nodeId, 'ec_')) return 'ec';
    if (str_starts_with($nodeId, 'display_')) return 'display';
    
    // 2. По содержимому data
    if (isset($data['ph']) && isset($data['ec'])) return 'ph_ec';
    if (isset($data['ph'])) return 'ph';
    if (isset($data['ec'])) return 'ec';
    if (isset($data['temperature']) && isset($data['co2'])) return 'climate';
    
    return 'unknown';
}
```

---

## ✅ ЧЕКЛИСТ ПРОВЕРКИ

При добавлении новой ноды или изменении существующей:

- [ ] **Discovery** содержит `node_type`
- [ ] **Telemetry** содержит `node_type`
- [ ] **Heartbeat** содержит `node_type`
- [ ] **Event** использует `mesh_protocol_create_event()`
- [ ] Все команды сохраняют изменения в NVS
- [ ] Config updates сохраняются в NVS
- [ ] Backend распознает тип ноды в `detectNodeType()`
- [ ] Валидация параметров перед сохранением

---

**Дата:** 21 октября 2025  
**Версия:** 2.0 (с обязательным node_type)

