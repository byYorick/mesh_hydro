# Мультизонная система: Потоки данных

## Обзор

Документ описывает потоки данных в мультизонной mesh-сети от момента включения узла до отображения данных в UI.

---

## 1. Discovery Flow (Регистрация нового узла)

```
┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐
│  Leaf    │      │   Root   │      │   MQTT   │      │  Backend │      │ Frontend │
│  Node    │      │   Node   │      │  Broker  │      │   (PHP)  │      │  (Vue)   │
└────┬─────┘      └────┬─────┘      └────┬─────┘      └────┬─────┘      └────┬─────┘
     │                 │                 │                 │                 │
     │ 1. Boot & Init  │                 │                 │                 │
     ├────────────────>│                 │                 │                 │
     │                 │                 │                 │                 │
     │ 2. Join mesh    │                 │                 │                 │
     ├────────────────>│                 │                 │                 │
     │   (ESP-MESH)    │                 │                 │                 │
     │                 │                 │                 │                 │
     │ 3. Discovery    │                 │                 │                 │
     │   JSON          │                 │                 │                 │
     ├────────────────>│                 │                 │                 │
     │                 │                 │                 │                 │
     │                 │ 4. PUBLISH      │                 │                 │
     │                 │   hydro/{mesh}/discovery         │                 │
     │                 ├────────────────>│                 │                 │
     │                 │                 │                 │                 │
     │                 │                 │ 5. DELIVER      │                 │
     │                 │                 ├────────────────>│                 │
     │                 │                 │                 │                 │
     │                 │                 │ 6. Parse JSON   │                 │
     │                 │                 │    Extract zone │                 │
     │                 │                 │    Create Node  │                 │
     │                 │                 │    in DB        │                 │
     │                 │                 │    <────────>   │                 │
     │                 │                 │                 │                 │
     │                 │                 │                 │ 7. WebSocket    │
     │                 │                 │                 │    NodeDiscovered
     │                 │                 │                 ├────────────────>│
     │                 │                 │                 │                 │
     │                 │                 │                 │ 8. Update UI    │
     │                 │                 │                 │    Add node card│
     │                 │                 │                 │    <──────────> │
```

### Детали потока

**Шаг 1-2:** Leaf node загружается, подключается к mesh-сети Root node через ESP-MESH протокол

**Шаг 3:** Leaf отправляет discovery JSON через mesh:
```json
{
  "type": "discovery",
  "node_id": "climate_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "node_type": "climate",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "firmware": "2.0.0",
  "timestamp": 1699876543
}
```

**Шаг 4:** Root публикует в MQTT топик `hydro/zone_greenhouse_1/discovery`

**Шаг 5:** MQTT брокер доставляет сообщение Backend (подписан на `hydro/+/discovery`)

**Шаг 6:** Backend (MqttService):
- Извлекает `mesh_id = "zone_greenhouse_1"` из топика
- Валидирует zone
- Создаёт запись в таблице `nodes` с полями `zone`, `root_node_id`, `node_id`
- Устанавливает `online = false` (ждём heartbeat)

**Шаг 7:** Backend отправляет WebSocket событие `NodeDiscovered` в канал `zone.zone_greenhouse_1`

**Шаг 8:** Frontend получает событие, добавляет карточку узла в UI (если выбрана эта зона)

---

## 2. Heartbeat Flow (Поддержание online статуса)

```
┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐
│  Leaf    │      │   Root   │      │   MQTT   │      │  Backend │      │ Frontend │
│  Node    │      │   Node   │      │  Broker  │      │   (PHP)  │      │  (Vue)   │
└────┬─────┘      └────┬─────┘      └────┬─────┘      └────┬─────┘      └────┬─────┘
     │                 │                 │                 │                 │
     │ [Every 10s]     │                 │                 │                 │
     │                 │                 │                 │                 │
     │ 1. Heartbeat    │                 │                 │                 │
     │    JSON         │                 │                 │                 │
     ├────────────────>│                 │                 │                 │
     │                 │                 │                 │                 │
     │                 │ 2. PUBLISH      │                 │                 │
     │                 │   hydro/{mesh}/heartbeat/{node}   │                 │
     │                 ├────────────────>│                 │                 │
     │                 │                 │                 │                 │
     │                 │                 │ 3. DELIVER      │                 │
     │                 │                 ├────────────────>│                 │
     │                 │                 │                 │                 │
     │                 │                 │ 4. Update       │                 │
     │                 │                 │    last_seen_at │                 │
     │                 │                 │    online=true  │                 │
     │                 │                 │    <────────>   │                 │
     │                 │                 │                 │                 │
     │                 │                 │                 │ 5. WebSocket    │
     │                 │                 │                 │    (if status   │
     │                 │                 │                 │     changed)    │
     │                 │                 │                 ├────────────────>│
     │                 │                 │                 │                 │
     │                 │                 │                 │ 6. Update badge │
     │                 │                 │                 │    color        │
     │                 │                 │                 │    <──────────> │
```

### Детали потока

**Heartbeat JSON:**
```json
{
  "type": "heartbeat",
  "node_id": "climate_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "node_type": "climate",
  "uptime": 360000,
  "heap_free": 125000,
  "rssi": -45,
  "timestamp": 1699876553
}
```

**Backend обработка:**
- Обновляет `nodes.last_seen_at = now()`
- Проверяет `isOnline()` (разница < 30 секунд)
- Если статус изменился (offline → online) — отправляет WebSocket событие

**Timeout логика:**
```
Heartbeat каждые 10s
Timeout через 30s = 3 пропущенных heartbeat
```

---

## 3. Telemetry Flow (Данные с датчиков)

```
┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐
│  Leaf    │      │   Root   │      │   MQTT   │      │  Backend │      │ Frontend │
│  Node    │      │   Node   │      │  Broker  │      │   (PHP)  │      │  (Vue)   │
└────┬─────┘      └────┬─────┘      └────┬─────┘      └────┬─────┘      └────┬─────┘
     │                 │                 │                 │                 │
     │ [Every 30-60s]  │                 │                 │                 │
     │                 │                 │                 │                 │
     │ 1. Read sensors │                 │                 │                 │
     │    <──────────> │                 │                 │                 │
     │                 │                 │                 │                 │
     │ 2. Telemetry    │                 │                 │                 │
     │    JSON         │                 │                 │                 │
     ├────────────────>│                 │                 │                 │
     │                 │                 │                 │                 │
     │                 │ 3. PUBLISH      │                 │                 │
     │                 │   hydro/{mesh}/telemetry/{node}   │                 │
     │                 ├────────────────>│                 │                 │
     │                 │                 │                 │                 │
     │                 │                 │ 4. DELIVER      │                 │
     │                 │                 ├────────────────>│                 │
     │                 │                 │                 │                 │
     │                 │                 │ 5. Save to DB   │                 │
     │                 │                 │    telemetry    │                 │
     │                 │                 │    table        │                 │
     │                 │                 │    <────────>   │                 │
     │                 │                 │                 │                 │
     │                 │                 │                 │ 6. WebSocket    │
     │                 │                 │                 │    TelemetryRcvd│
     │                 │                 │                 ├────────────────>│
     │                 │                 │                 │                 │
     │                 │                 │                 │ 7. Update chart │
     │                 │                 │                 │    /graphs      │
     │                 │                 │                 │    <──────────> │
```

### Детали потока

**Telemetry JSON (Climate):**
```json
{
  "type": "telemetry",
  "node_id": "climate_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "node_type": "climate",
  "timestamp": 1699876543,
  "data": {
    "temperature": 24.5,
    "humidity": 65.2,
    "co2": 450,
    "lux": 15000
  }
}
```

**Backend обработка:**
- Создаёт запись в `telemetry` таблице
- Обновляет `last_seen_at` в `nodes`
- Broadcast через WebSocket для live-графиков

---

## 4. Command Flow (Отправка команды)

```
┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐
│ Frontend │      │  Backend │      │   MQTT   │      │   Root   │      │   Leaf   │
│  (Vue)   │      │   (PHP)  │      │  Broker  │      │   Node   │      │   Node   │
└────┬─────┘      └────┬─────┘      └────┬─────┘      └────┬─────┘      └────┬─────┘
     │                 │                 │                 │                 │
     │ 1. User clicks  │                 │                 │                 │
     │    "Run Pump"   │                 │                 │                 │
     │    <──────────> │                 │                 │                 │
     │                 │                 │                 │                 │
     │ 2. POST request │                 │                 │                 │
     │    /api/command │                 │                 │                 │
     ├────────────────>│                 │                 │                 │
     │                 │                 │                 │                 │
     │                 │ 3. Get node zone│                 │                 │
     │                 │    from DB      │                 │                 │
     │                 │    <────────>   │                 │                 │
     │                 │                 │                 │                 │
     │                 │ 4. PUBLISH      │                 │                 │
     │                 │   hydro/{mesh}/command/{node}     │                 │
     │                 ├────────────────>│                 │                 │
     │                 │                 │                 │                 │
     │                 │                 │ 5. DELIVER      │                 │
     │                 │                 │   (subscribed)  │                 │
     │                 │                 ├────────────────>│                 │
     │                 │                 │                 │                 │
     │                 │                 │                 │ 6. Route to node│
     │                 │                 │                 │    via mesh     │
     │                 │                 │                 ├────────────────>│
     │                 │                 │                 │                 │
     │                 │                 │                 │ 7. Execute cmd  │
     │                 │                 │                 │    run_pump()   │
     │                 │                 │                 │    <──────────> │
     │                 │                 │                 │                 │
     │                 │                 │                 │ 8. Event JSON   │
     │                 │                 │                 │    "Pump started│
     │                 │                 │                 │    "            │
     │                 │                 │    <────────────┼─────────────────┤
     │                 │                 │                 │                 │
     │                 │                 │    [Event flow continues as above]
```

### Детали потока

**Command JSON:**
```json
{
  "type": "command",
  "node_id": "ph_001",
  "command": "run_pump",
  "params": {
    "pump_id": 0,
    "duration_ms": 5000,
    "dose_ml": 2.5
  },
  "command_id": 12345,
  "timestamp": 1699876543
}
```

**Root Node routing:**
- Root подписан на `hydro/{его_mesh}/command/#`
- Получает команду
- Парсит `node_id` из топика
- Ищет узел в своей mesh-сети
- Отправляет JSON через ESP-MESH

**Leaf Node execution:**
- Получает JSON через mesh
- Проверяет `node_id == свой_id`
- Выполняет команду
- Отправляет event о результате

---

## 5. Zone Isolation (Изоляция зон)

### 5.1. Изоляция на уровне топиков

```
Zone 1 (greenhouse):
  hydro/zone_greenhouse_1/heartbeat/climate_001
  hydro/zone_greenhouse_1/command/ph_001

Zone 2 (lab):
  hydro/zone_lab_42/heartbeat/climate_002
  hydro/zone_lab_42/command/ph_002
```

**Независимость:**
- Root Node 1 подписан только на `hydro/zone_greenhouse_1/command/#`
- Root Node 2 подписан только на `hydro/zone_lab_42/command/#`
- Команда в zone 1 **не попадёт** в zone 2

### 5.2. Изоляция на уровне Backend

```php
// При отправке команды
$node = Node::where('node_id', 'ph_001')->first();
$meshId = $node->zone; // "zone_greenhouse_1"
$topic = "hydro/{$meshId}/command/{$nodeId}";
$mqtt->publish($topic, $payload);
```

**Проверка:**
- Backend берёт zone из БД для узла
- Формирует топик с правильным mesh_id
- Гарантия доставки в правильную зону

### 5.3. Изоляция на уровне Frontend

```typescript
// Фильтрация узлов
const filteredNodes = computed(() => {
  const currentZone = zonesStore.currentZone;
  return nodesStore.nodes.filter(node => node.zone === currentZone);
});
```

**WebSocket подписки:**
```typescript
echo.channel(`zone.${zone}`)
  .listen('NodeDiscovered', ...)
  .listen('TelemetryReceived', ...);
```

---

## 6. Setup Mode Flow (Первое подключение)

```
┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐
│ New Leaf │      │   Root   │      │  Backend │      │ Frontend │
│  Node    │      │  (setup) │      │   (PHP)  │      │  (Vue)   │
└────┬─────┘      └────┬─────┘      └────┬─────┘      └────┬─────┘
     │                 │                 │                 │
     │ 1. Boot         │                 │                 │
     │    No config    │                 │                 │
     │    in NVS       │                 │                 │
     │                 │                 │                 │
     │ 2. Scan WiFi    │                 │                 │
     │    Find         │                 │                 │
     │    "HYDRO_SETUP"│                 │                 │
     │    mesh         │                 │                 │
     │                 │                 │                 │
     │ 3. Join setup   │                 │                 │
     │    mesh         │                 │                 │
     ├────────────────>│                 │                 │
     │                 │                 │                 │
     │ 4. Discovery    │                 │                 │
     │    with PIN     │                 │                 │
     ├────────────────>│                 │                 │
     │                 │                 │                 │
     │                 │ 5. PUBLISH      │                 │
     │                 │   hydro/setup/discovery           │
     │                 ├────────────────>│                 │
     │                 │                 │                 │
     │                 │                 │ 6. Create       │
     │                 │                 │    in new_nodes │
     │                 │                 │    table        │
     │                 │                 │    <────────>   │
     │                 │                 │                 │
     │                 │                 │                 │ 7. Show in UI
     │                 │                 │                 │    setup wizard│
     │                 │                 │                 │    <──────────>│
     │                 │                 │                 │                 │
     │                 │                 │ 8. User config  │                 │
     │                 │                 │    node_id,     │                 │
     │                 │                 │    mesh_id,     │                 │
     │                 │                 │    root_id      │                 │
     │                 │                 │    <────────────┼─────────────────┤
     │                 │                 │                 │                 │
     │                 │ 9. write_config │                 │                 │
     │   <─────────────┼─────────────────┤                 │                 │
     │                 │   command       │                 │                 │
     │                 │                 │                 │                 │
     │ 10. Save to NVS │                 │                 │                 │
     │     Restart     │                 │                 │                 │
     │     <────────>  │                 │                 │                 │
     │                 │                 │                 │                 │
     │ 11. Boot with   │                 │                 │                 │
     │     config      │                 │                 │                 │
     │     Join prod   │                 │                 │                 │
     │     mesh        │                 │                 │                 │
     │                 │                 │                 │                 │
     │     [Continue with normal discovery flow]          │                 │
```

### Детали Setup Mode

**Setup Discovery JSON:**
```json
{
  "type": "discovery",
  "node_type": "climate",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "pin": "AABBCC",
  "temp_mesh_id": "HYDRO_SETUP_AABBCC",
  "firmware": "2.0.0",
  "timestamp": 1699876543
}
```

**Write Config Command:**
```json
{
  "type": "command",
  "command": "write_config",
  "params": {
    "pin": "AABBCC",
    "node_id": "climate_001",
    "mesh_id": "zone_greenhouse_1",
    "root_node_id": "root_setup",
    "zone": "Greenhouse Zone 1",
    "config": {
      "temp_target": 24.0
    }
  }
}
```

---

## 7. Error Handling (Обработка ошибок)

### 7.1. Отсутствие zone в сообщении

```
Leaf → Root → MQTT: hydro/heartbeat/climate_001 (LEGACY формат)
                            ↓
                       Backend:
                         ❌ extractZoneFromTopic() = null
                         ❌ payload['mesh_network_id'] = missing
                         ❌ REJECT сообщение
                         ❌ Log ERROR
                         ❌ Create Event (level: critical)
```

### 7.2. Mesh_id не совпадает

```
Topic:   hydro/zone_lab_42/command/ph_001
Payload: mesh_network_id = "zone_greenhouse_1"
                            ↑
                   Backend WARNING:
                   "Topic zone != payload zone"
                   Use topic zone (приоритет)
```

### 7.3. Команда для чужой зоны

```
Root Node 1 (zone_greenhouse_1):
  Subscribed: hydro/zone_greenhouse_1/command/#
  
MQTT публикует: hydro/zone_lab_42/command/ph_002
                            ↓
            Root 1: НЕ получит (не подписан)
            Root 2: Получит и обработает
```

---

## 8. Производительность и масштабирование

### 8.1. Метрики одной зоны

**Типичная зона (10 узлов):**
- Heartbeat: 10 msg/10s = 1 msg/s
- Telemetry: 10 msg/60s = 0.16 msg/s
- **Итого:** ~1.2 msg/s

**Пиковая нагрузка:**
- Events при аварии: +10 msg
- Commands: +5 msg
- **Итого:** ~15 msg за секунду (пик)

### 8.2. Многозонная система (10 зон)

- **Steady state:** 12 msg/s
- **Peak:** 150 msg/s
- **Backend:** легко справляется (Laravel + Redis + PostgreSQL)
- **MQTT broker:** Mosquitto handle > 10k msg/s

### 8.3. Bottleneck анализ

**Не являются bottleneck:**
- ✅ ESP-MESH (до 100 узлов в сети)
- ✅ MQTT broker (Mosquitto)
- ✅ Backend (PHP 8.2 + opcache)

**Потенциальные bottleneck:**
- ⚠️ База данных (при большом объёме telemetry)
  - **Решение:** партиционирование по zone и timestamp
- ⚠️ WebSocket (при большом количестве подключений)
  - **Решение:** отдельный WebSocket сервер на Node.js

---

## 9. Безопасность

### 9.1. Аутентификация

- **MQTT:** Anonymous (для локальной сети)
- **Mesh:** Shared password (в конфиге)
- **Backend API:** JWT токены
- **WebSocket:** Laravel Echo (с токеном)

### 9.2. Авторизация

- **Zone isolation:** автоматическая через топики
- **Command validation:** проверка существования узла в БД
- **UI:** пользователь видит только свои зоны

### 9.3. Валидация

- **Топики:** regex для mesh_id и node_id
- **Payload:** JSON schema validation
- **Размер:** max 2048 байт
- **Rate limiting:** (опционально) в backend

---

## Выводы

1. **Изоляция зон обеспечивается на всех уровнях:** топики MQTT, routing в Root Node, фильтрация в Backend/Frontend

2. **Отказоустойчивость:** независимость зон — падение одной не влияет на другие

3. **Масштабируемость:** линейная (добавление зоны не увеличивает нагрузку на существующие)

4. **Простота отладки:** логи разделены по zone, метрики по zone

5. **Производительность:** достаточна для 10+ зон по 10+ узлов каждая


