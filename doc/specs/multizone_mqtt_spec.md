# Спецификация мультизонной MQTT архитектуры

## Версия: 0.9
**Дата:** 2025-11-08  
**Статус:** Черновик

Документ фиксирует единый стандарт обмена сообщениями для мультизонной сети Hydro, включая формат MQTT-топиков, обязательные поля payload и правила обработки зон. Любые отклонения должны документироваться отдельно.

---

## 1. Формат топиков MQTT

### 1.1. Базовая структура

```
hydro/{mesh_id}/{message_type}/{node_id}
```

**Компоненты:**

- `mesh_id` — уникальный идентификатор mesh-сети (зоны)
  - Формат: `[a-zA-Z0-9_-]{3,31}`
  - Примеры: `zone_greenhouse_1`, `mesh_lab_42`, `production_main`
  
- `message_type` — тип сообщения:
  - `heartbeat` — периодический сигнал жизни узла
  - `telemetry` — данные с датчиков
  - `event` — события (аварии, предупреждения)
  - `command` — команды от backend к узлам
  - `config` — конфигурация узла
  - `discovery` — регистрация нового узла
  - `config_response` — ответ с конфигурацией
  - `error` — сообщения об ошибках

- `node_id` — уникальный идентификатор узла в зоне
  - Формат: `[a-zA-Z0-9_-]{3,31}`
  - Примеры: `climate_001`, `ph_042`, `root_setup`

### 1.2. Примеры топиков

**От узлов к backend:**
```
hydro/zone_greenhouse_1/heartbeat/climate_001
hydro/zone_greenhouse_1/telemetry/ph_001
hydro/zone_greenhouse_1/event/water_003
hydro/zone_greenhouse_1/discovery/climate_002
hydro/zone_greenhouse_1/config_response/ph_001
```

**От backend к узлам:**
```
hydro/zone_greenhouse_1/command/ph_001
hydro/zone_greenhouse_1/config/climate_001
```

### 1.3. Специальный топик для setup режима

Для новых неконфигурированных узлов используется временный топик:

```
hydro/setup/discovery
hydro/setup/heartbeat/{mac_address}
```

После конфигурации узел переходит на основные топики с mesh_id.

---

## 2. Обязательные поля JSON payload

### 2.1. Базовые поля (для всех типов)

```json
{
  "type": "heartbeat|telemetry|event|...",
  "node_id": "climate_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "timestamp": 1699876543
}
```

**Описание полей:**

- `type` (string, required) — тип сообщения, должен соответствовать `message_type` в топике
- `node_id` (string, required) — идентификатор отправителя
- `root_node_id` (string, required) — идентификатор Root Node зоны
- `mesh_network_id` (string, required) — идентификатор mesh-сети, должен соответствовать `mesh_id` в топике
- `timestamp` (integer, required) — Unix timestamp в секундах

---

## 3. Специфичные поля по типам сообщений

### 3.1. Heartbeat

**Назначение:** Периодический сигнал о работоспособности узла

**Дополнительные поля:**
```json
{
  "node_type": "climate|ph|ec|water|relay|display|root",
  "uptime": 360000,
  "heap_free": 125000,
  "rssi": -45
}
```

- `node_type` (string, required) — тип узла
- `uptime` (integer, optional) — время работы с последнего перезапуска (мс)
- `heap_free` (integer, optional) — свободная память heap (байты)
- `rssi` (integer, optional) — уровень сигнала WiFi (dBm)

**Пример полного сообщения:**
```json
{
  "type": "heartbeat",
  "node_id": "climate_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "node_type": "climate",
  "timestamp": 1699876543,
  "uptime": 360000,
  "heap_free": 125000,
  "rssi": -45
}
```

**Топик:** `hydro/zone_greenhouse_1/heartbeat/climate_001`

---

### 3.2. Telemetry

**Назначение:** Передача данных с датчиков

**Дополнительные поля:**
```json
{
  "node_type": "climate|ph|ec|...",
  "data": {
    "temperature": 24.5,
    "humidity": 65.2
  }
}
```

- `node_type` (string, required) — тип узла
- `data` (object, required) — объект с показаниями датчиков, структура зависит от типа узла

**Примеры данных по типам узлов:**

**Climate:**
```json
"data": {
  "temperature": 24.5,
  "humidity": 65.2,
  "co2": 450,
  "lux": 15000
}
```

**pH:**
```json
"data": {
  "ph": 6.5,
  "ph_target": 6.3,
  "pump_up_active": false,
  "pump_down_active": false
}
```

**EC:**
```json
"data": {
  "ec": 1.8,
  "ec_target": 2.0,
  "tds": 900
}
```

**Топик:** `hydro/zone_greenhouse_1/telemetry/climate_001`

---

### 3.3. Event

**Назначение:** Сообщения о событиях, предупреждениях, авариях

**Дополнительные поля:**
```json
{
  "level": "info|warning|critical|emergency",
  "message": "pH far from target, aggressive correction",
  "data": {
    "current_ph": 7.5,
    "target_ph": 6.3
  }
}
```

- `level` (string, required) — уровень критичности
  - `info` — информационное
  - `warning` — предупреждение
  - `critical` — критично
  - `emergency` — авария
- `message` (string, required) — текст сообщения
- `data` (object, optional) — дополнительные данные события

**Пример:**
```json
{
  "type": "event",
  "node_id": "ph_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "timestamp": 1699876543,
  "level": "warning",
  "message": "pH far from target, aggressive correction",
  "data": {
    "current_ph": 7.5,
    "target_ph": 6.3,
    "deviation": 1.2
  }
}
```

**Топик:** `hydro/zone_greenhouse_1/event/ph_001`

---

### 3.4. Discovery

**Назначение:** Регистрация нового узла в системе

**Дополнительные поля:**
```json
{
  "node_type": "climate|ph|ec|...",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "firmware": "2.0.0",
  "hardware": "ESP32-S3",
  "capabilities": ["mesh_node", "autonomous_control"]
}
```

- `node_type` (string, required) — тип узла
- `mac_address` (string, required) — MAC адрес
- `firmware` (string, optional) — версия прошивки
- `hardware` (string, optional) — модель чипа
- `capabilities` (array, optional) — список возможностей

**Пример:**
```json
{
  "type": "discovery",
  "node_id": "climate_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "node_type": "climate",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "firmware": "2.0.0",
  "hardware": "ESP32",
  "timestamp": 1699876543,
  "capabilities": ["sht3x", "ccs811", "bh1750"]
}
```

**Топик:** `hydro/zone_greenhouse_1/discovery`

---

### 3.5. Command

**Назначение:** Команды от backend к узлам

**Дополнительные поля:**
```json
{
  "command": "run_pump|calibrate|restart|...",
  "params": {
    "pump_id": 0,
    "duration_ms": 5000,
    "dose_ml": 2.5
  },
  "command_id": 12345
}
```

- `command` (string, required) — название команды
- `params` (object, optional) — параметры команды
- `command_id` (integer, optional) — ID команды для отслеживания

**Пример:**
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

**Топик:** `hydro/zone_greenhouse_1/command/ph_001`

---

### 3.6. Config

**Назначение:** Обновление конфигурации узла

**Дополнительные поля:**
```json
{
  "config": {
    "ph_target": 6.3,
    "ph_min": 5.5,
    "ph_max": 7.5
  }
}
```

- `config` (object, required) — объект конфигурации, структура зависит от типа узла

**Пример:**
```json
{
  "type": "config",
  "node_id": "ph_001",
  "timestamp": 1699876543,
  "config": {
    "ph_target": 6.3,
    "ph_min": 5.5,
    "ph_max": 7.5,
    "pump_pid": [
      {"kp": 1.0, "ki": 0.05, "kd": 0.3}
    ]
  }
}
```

**Топик:** `hydro/zone_greenhouse_1/config/ph_001`

---

### 3.7. Config Response

**Назначение:** Ответ узла с текущей конфигурацией

**Дополнительные поля:**
```json
{
  "config": {
    "ph_target": 6.3,
    "ph_min": 5.5,
    "ph_max": 7.5
  }
}
```

- `config` (object, required) — текущая конфигурация узла

**Пример:**
```json
{
  "type": "config_response",
  "node_id": "ph_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "timestamp": 1699876543,
  "config": {
    "ph_target": 6.3,
    "ph_min": 5.5,
    "ph_max": 7.5,
    "pump_calibration": [
      {"pump_id": 0, "ml_per_second": 1.2}
    ]
  }
}
```

**Топик:** `hydro/zone_greenhouse_1/config_response/ph_001`

---

## 4. Правила извлечения zone

Backend должен извлекать zone (mesh_id) в следующем порядке приоритета:

### Приоритет 1: Из топика
```
hydro/{mesh_id}/heartbeat/climate_001
      ^^^^^^^^^
```

### Приоритет 2: Из payload `mesh_network_id`
```json
{
  "mesh_network_id": "zone_greenhouse_1"
}
```

### Приоритет 3: ОШИБКА

Если ни топик, ни payload не содержат валидный mesh_id:
- Сообщение **ОТКЛОНЯЕТСЯ**
- Логируется `ERROR` с деталями
- Создаётся критическое событие в БД

**Пример логирования:**
```
[ERROR] Heartbeat without valid zone
  topic: hydro/heartbeat/climate_001
  node_id: climate_001
  payload_mesh_id: missing
  action: REJECTED
```

---

## 5. Валидация mesh_id и node_id

### 5.1. Валидация mesh_id

**Правила:**
- Длина: 3-31 символа
- Символы: `[a-zA-Z0-9_-]`
- Не может быть: `setup`, `UNCONFIGURED` (зарезервированы)

**Примеры валидных:**
- `zone_greenhouse_1`
- `lab-42`
- `production_main`

**Примеры невалидных:**
- `z1` (слишком короткий)
- `zone#1` (недопустимый символ)
- `setup` (зарезервирован)

### 5.2. Валидация node_id

**Правила:**
- Длина: 3-31 символа
- Символы: `[a-zA-Z0-9_-]`
- Рекомендуемый шаблон: `{type}_{number}` (например, `climate_001`)

**Примеры валидных:**
- `climate_001`
- `ph-042`
- `root_setup`

---

## 6. QoS и retain флаги

### 6.1. Heartbeat, Telemetry, Event
- **QoS:** 0 (at most once)
- **Retain:** false
- **Обоснование:** данные периодичны, потеря одного сообщения некритична

### 6.2. Command, Config
- **QoS:** 1 (at least once)
- **Retain:** false
- **Обоснование:** важно доставить, но не хранить историю

### 6.3. Discovery
- **QoS:** 1 (at least once)
- **Retain:** false (или true для последнего discovery)
- **Обоснование:** важно зарегистрировать узел

---

## 7. Ограничения и best practices

### 7.1. Размер payload
- **Максимум:** 2048 байт
- **Рекомендуемо:** < 1024 байт
- **Обоснование:** ограничения ESP32 и MQTT брокера

### 7.2. Частота публикаций
- **Heartbeat:** каждые 10 секунд
- **Telemetry:** каждые 30-60 секунд
- **Event:** по событию
- **Discovery:** при старте + переподключении

### 7.3. Timeout узла
- **Backend offline timeout:** 30 секунд
- **Обоснование:** 3 пропущенных heartbeat = offline

---

## 8. Примеры полных сценариев

### 8.1. Регистрация нового узла

**Шаг 1:** Узел отправляет discovery
```
Topic: hydro/zone_greenhouse_1/discovery
Payload:
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

**Шаг 2:** Backend регистрирует узел в БД

**Шаг 3:** Узел начинает отправлять heartbeat
```
Topic: hydro/zone_greenhouse_1/heartbeat/climate_001
Payload:
{
  "type": "heartbeat",
  "node_id": "climate_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "node_type": "climate",
  "timestamp": 1699876553,
  "uptime": 10000,
  "heap_free": 130000
}
```

### 8.2. Отправка команды

**Шаг 1:** Backend публикует команду
```
Topic: hydro/zone_greenhouse_1/command/ph_001
Payload:
{
  "type": "command",
  "node_id": "ph_001",
  "command": "run_pump",
  "params": {
    "pump_id": 0,
    "duration_ms": 5000
  },
  "command_id": 12345,
  "timestamp": 1699876543
}
```

**Шаг 2:** Узел выполняет команду

**Шаг 3:** Узел отправляет event о выполнении
```
Topic: hydro/zone_greenhouse_1/event/ph_001
Payload:
{
  "type": "event",
  "node_id": "ph_001",
  "root_node_id": "root_setup",
  "mesh_network_id": "zone_greenhouse_1",
  "timestamp": 1699876548,
  "level": "info",
  "message": "Pump started",
  "data": {
    "pump_id": 0,
    "duration_ms": 5000,
    "command_id": 12345
  }
}
```

---

## 9. Миграция с legacy формата

### 9.1. Legacy формат (удаляется)
```
hydro/setup/heartbeat/{node_id}
hydro/heartbeat/{node_id}
hydro/telemetry/{node_id}
```

### 9.2. Новый формат (обязательный)
```
hydro/{mesh_id}/heartbeat/{node_id}
hydro/{mesh_id}/telemetry/{node_id}
```

### 9.3. Период перехода
- **Legacy поддержка:** НЕТ (полное удаление)
- **Fallback:** только для `hydro/setup/*` (временный режим для новых узлов)
- **Обязательное требование:** все узлы должны быть обновлены

---

## 10. Контрольный чек-лист реализации

### Прошивки
- [ ] Все сообщения содержат `root_node_id` и `mesh_network_id`
- [ ] Топики формируются через `mesh_topic_format()`
- [ ] Zone загружается из NVS при старте
- [ ] Валидация zone перед отправкой

### Backend
- [ ] Парсинг топика извлекает mesh_id
- [ ] Валидация mesh_id по regex
- [ ] Отклонение сообщений без zone с логированием ERROR
- [ ] API `/api/zones` возвращает список зон

### Frontend
- [ ] UI отображает текущую zone
- [ ] Фильтрация узлов по zone
- [ ] WebSocket подписки разделены по zone

---

## Версионирование

**Текущая версия:** 1.0  
**История изменений:**

- **1.0 (2025-11-08):** Первая версия спецификации с полным удалением legacy

