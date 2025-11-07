# 📡 Обновление ESP32 Child Nodes для Zoning

## Статус: ✅ ГОТОВО

### Обзор изменений

Все child nodes уже используют обновленную функцию `mesh_protocol_create_telemetry()` из `common/mesh_protocol/mesh_protocol.h`, которая автоматически добавляет `root_node_id` во все сообщения.

## Проверка узлов

### ✅ Node Climate
**Файл:** `node_climate/main/app_main.c`

**Использует:**
```c
mesh_message_t msg = mesh_protocol_create_telemetry(
    s_node_config.node_id,
    "climate",
    telemetry_json
);
```

**Функция `mesh_protocol_create_telemetry` автоматически:**
1. Получает `root_node_id` из NVS через `node_config_get_root_node_id()`
2. Добавляет его в структуру сообщения
3. Отправляет через mesh-сеть

### ✅ Node pH/EC
**Файл:** `node_ph/main/app_main.c`

**Использует:** Аналогично node_climate
```c
mesh_message_t msg = mesh_protocol_create_telemetry(/*...*/);
```

### ✅ Node Relay
**Файл:** `node_relay/main/app_main.c`

**Использует:** Аналогично
```c
mesh_protocol_create_telemetry(/*...*/);
```

### ✅ Node Water
**Файл:** `node_water/main/app_main.c`

**Использует:** Аналогично
```c
mesh_protocol_create_telemetry(/*...*/);
```

## Как это работает

### 1. Обновленный mesh_protocol.h

```c
// common/mesh_protocol/mesh_protocol.h

mesh_message_t mesh_protocol_create_telemetry(
    const char* node_id,
    const char* node_type,
    const char* data
) {
    mesh_message_t msg = {
        .msg_type = MESH_MSG_TYPE_TELEMETRY,
        .timestamp = (uint32_t)(esp_timer_get_time() / 1000000),
        .root_node_id = {0},  // ⭐ Автоматически заполняется
    };

    // Копируем node_id
    strncpy(msg.node_id, node_id, sizeof(msg.node_id) - 1);
    
    // Копируем node_type
    strncpy(msg.node_type, node_type, sizeof(msg.node_type) - 1);
    
    // ⭐ НОВОЕ: Получаем root_node_id из конфигурации
    char root_id[32] = {0};
    if (node_config_get_root_node_id(root_id, sizeof(root_id)) == ESP_OK) {
        strncpy(msg.root_node_id, root_id, sizeof(msg.root_node_id) - 1);
    }
    
    // Копируем данные
    strncpy(msg.data, data, sizeof(msg.data) - 1);
    
    return msg;
}
```

### 2. Node Config хранит root_node_id в NVS

```c
// common/node_config/node_config.h

esp_err_t node_config_get_root_node_id(char* buffer, size_t buffer_size);
esp_err_t node_config_set_root_node_id(const char* root_node_id);
```

### 3. Конфигурация через menuconfig

При компиляции каждого узла настраивается:

```
idf.py menuconfig
└── Mesh Configuration
    ├── Node ID: "ph_ec_nft_001"
    ├── Node Type: "ph_ec"
    └── Root Node ID: "root_nft_001"  ⭐ Указывает на Root Node зоны
```

### 4. Конфигурация через NVS runtime

Можно изменить через команду от сервера:

```json
{
  "command": "update_config",
  "params": {
    "root_node_id": "root_nft_001"
  }
}
```

## Результат

### Структура сообщения телеметрии

```json
{
  "msg_type": "telemetry",
  "node_id": "ph_ec_nft_001",
  "node_type": "ph_ec",
  "root_node_id": "root_nft_001",  ⭐ Автоматически добавляется
  "timestamp": 1699380000,
  "data": {
    "ph": 6.0,
    "ec": 1.5,
    "temp": 22.5
  }
}
```

### MQTT топик

Root Node отправляет на:
```
hydro/nft1/telemetry/ph_ec_nft_001
```

Server извлекает:
- Zone: `nft1` (из топика)
- Node: `ph_ec_nft_001`
- Root Node: `root_nft_001` (из payload)

## Валидация на сервере

Backend проверяет:

```php
// MqttListenerCommand.php

$zoneName = $this->extractZoneFromTopic($topic);
$rootNodeId = $data['root_node_id'] ?? null;

// Проверяем соответствие зоны и Root Node
$zone = Zone::where('mqtt_topic_prefix', "hydro/{$zoneName}/")->first();
if ($zone && $zone->root_node_id !== $rootNodeId) {
    Log::warning("Root Node mismatch", [
        'expected' => $zone->root_node_id,
        'received' => $rootNodeId,
    ]);
}
```

## Заключение

✅ **Все child nodes уже обновлены!**

Изменения были сделаны на уровне общих компонентов:
- `common/mesh_protocol/mesh_protocol.h` ✅
- `common/node_config/node_config.h` ✅

Каждый child node автоматически получил поддержку:
- ✅ node_climate
- ✅ node_ph (node_ph_ec)
- ✅ node_relay
- ✅ node_water

**Дополнительная работа не требуется!**

---

**Дата:** 2024-11-07  
**Статус:** COMPLETE

