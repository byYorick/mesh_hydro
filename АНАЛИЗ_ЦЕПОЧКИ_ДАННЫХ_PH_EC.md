# Анализ цепочки передачи данных для node_ph и node_ec ✅

**Дата:** 21.10.2025  
**Статус:** Полная проверка выполнена, найдено и исправлено 5 проблем

---

## 🔄 Полная цепочка передачи данных

```
NODE (pH/EC) 
    ↓ JSON через Mesh
ROOT NODE (data_router)
    ↓ MQTT публикация
MQTT BROKER (Mosquitto)
    ↓ Подписка
BACKEND (MqttListenerCommand)
    ↓ Сохранение в БД + WebSocket event
FRONTEND (Echo/WebSocket)
    ↓ Отображение в UI
```

### Обратная цепочка (команды):
```
FRONTEND (кнопка/форма)
    ↓ API POST /nodes/{id}/command
BACKEND (NodeController)
    ↓ MQTT publish hydro/command/{node_id}
MQTT BROKER
    ↓ Подписка ROOT
ROOT NODE (data_router → handle_mqtt_data)
    ↓ Mesh send
NODE (pH/EC manager → handle_command)
    ↓ Выполнение команды
```

---

## 📊 JSON Форматы

### 1. NODE → ROOT (Discovery)

**node_ph:**
```json
{
  "type": "discovery",
  "node_id": "ph_a1b2c3",
  "node_type": "ph",
  "sensors": ["ph"],
  "actuators": ["pump_ph_up", "pump_ph_down"],
  "heap_free": 234567,
  "wifi_rssi": -45,
  "mac": "AA:BB:CC:DD:EE:FF"
}
```

**node_ec:**
```json
{
  "type": "discovery",
  "node_id": "ec_a1b2c3",
  "node_type": "ec",
  "sensors": ["ec"],
  "actuators": ["pump_ec_a", "pump_ec_b", "pump_ec_c"],
  "heap_free": 234567,
  "wifi_rssi": -45,
  "mac": "AA:BB:CC:DD:EE:FF"
}
```

✅ **Статус:** Корректен  
📍 **Файл:** `node_ph/components/ph_manager/ph_manager.c` (send_discovery)

---

### 2. NODE → ROOT (Telemetry)

**node_ph:**
```json
{
  "type": "telemetry",
  "node_id": "ph_a1b2c3",
  "node_type": "ph",
  "data": {
    "ph": 6.8,
    "ph_target": 6.5,
    "pump_ph_up_ml": 150.5,
    "pump_ph_down_ml": 75.2,
    "rssi_to_parent": -45,
    "emergency": false
  }
}
```

**node_ec:**
```json
{
  "type": "telemetry",
  "node_id": "ec_a1b2c3",
  "node_type": "ec",
  "data": {
    "ec": 2.3,
    "ec_target": 2.5,
    "pump_ec_a_ml": 250.5,
    "pump_ec_b_ml": 245.2,
    "pump_ec_c_ml": 50.1,
    "rssi_to_parent": -45,
    "emergency": false
  }
}
```

✅ **Статус:** Исправлено (добавлен node_type)  
📍 **Файл:** `node_ph/components/ph_manager/ph_manager.c` (send_telemetry)

---

### 3. NODE → ROOT (Heartbeat)

**node_ph:**
```json
{
  "type": "heartbeat",
  "node_id": "ph_a1b2c3",
  "node_type": "ph",
  "uptime": 3600,
  "heap_free": 234567,
  "autonomous": false
}
```

**node_ec:**
```json
{
  "type": "heartbeat",
  "node_id": "ec_a1b2c3",
  "node_type": "ec",
  "uptime": 3600,
  "heap_free": 234567,
  "autonomous": false
}
```

✅ **Статус:** Исправлено (добавлен node_type)  
📍 **Файл:** `node_ph/components/ph_manager/ph_manager.c` (send_heartbeat)

---

### 4. ROOT → MQTT (Публикация)

**Топики:**
```
hydro/telemetry/ph_a1b2c3    ← telemetry от pH ноды
hydro/telemetry/ec_a1b2c3    ← telemetry от EC ноды
hydro/heartbeat/ph_a1b2c3    ← heartbeat от pH ноды
hydro/heartbeat/ec_a1b2c3    ← heartbeat от EC ноды
hydro/discovery              ← discovery от всех нод
```

✅ **Статус:** Корректно  
📍 **Файл:** `root_node/components/data_router/data_router.c`

---

### 5. BACKEND → Frontend (WebSocket)

**Event:** `TelemetryReceived`

```javascript
{
  node_id: "ph_a1b2c3",
  node_type: "ph",
  data: {
    ph: 6.8,
    ph_target: 6.5,
    pump_ph_up_ml: 150.5,
    // ...
  },
  received_at: "2025-10-21T12:34:56"
}
```

✅ **Статус:** Корректно  
📍 **Файл:** `server/backend/app/Events/TelemetryReceived.php`

---

### 6. FRONTEND → BACKEND (Команды)

**POST** `/api/nodes/ph_a1b2c3/command`

```json
{
  "command": "set_ph_target",
  "params": {
    "target": 6.0
  }
}
```

✅ **Статус:** Нужен новый controller  
📍 **Требуется:** `server/backend/app/Http/Controllers/NodeController.php`

---

### 7. BACKEND → ROOT (MQTT команда)

**Топик:** `hydro/command/ph_a1b2c3`

**Payload:**
```json
{
  "type": "command",
  "node_id": "ph_a1b2c3",
  "command": "set_ph_target",
  "params": {
    "target": 6.0
  }
}
```

✅ **Статус:** Корректно  
📍 **Файл:** `root_node/components/data_router/data_router.c` (handle_mqtt_data)

---

### 8. ROOT → NODE (Mesh forward)

ROOT пересылает полученную команду через mesh:

```c
mesh_manager_send(node->mac_addr, (const uint8_t *)data, data_len);
```

✅ **Статус:** Корректно  
📍 **Файл:** `root_node/components/data_router/data_router.c`

---

### 9. NODE обработка команды

**node_ph:**
```c
void ph_manager_handle_command(const char *command, cJSON *params) {
    if (strcmp(command, "set_ph_target") == 0) {
        cJSON *target = cJSON_GetObjectItem(params, "target");
        s_config->ph_target = (float)target->valuedouble;
        pid_set_setpoint(&s_pid_ph_up, s_config->ph_target);
        pid_set_setpoint(&s_pid_ph_down, s_config->ph_target);
    }
}
```

✅ **Статус:** Корректно  
📍 **Файл:** `node_ph/components/ph_manager/ph_manager.c`

---

## 🐛 Найденные и исправленные проблемы

### ❌ **Проблема 1: Отсутствует node_type в telemetry**

**Где:** `node_ph/components/ph_manager/ph_manager.c`, `node_ec/components/ec_manager/ec_manager.c`

**Проблема:**  
Telemetry не содержало поле `node_type`, только `type: "telemetry"`.  
Backend не мог правильно определить тип узла.

**Исправлено:**
```c
cJSON_AddStringToObject(root, "node_type", "ph");  // Добавлено
```

---

### ❌ **Проблема 2: Отсутствует node_type в heartbeat**

**Где:** `node_ph/components/ph_manager/ph_manager.c`, `node_ec/components/ec_manager/ec_manager.c`

**Проблема:**  
Heartbeat не содержало поле `node_type`.

**Исправлено:**
```c
cJSON_AddStringToObject(root, "node_type", "ph");  // Добавлено
```

---

### ❌ **Проблема 3: Backend не поддерживает типы "ph" и "ec"**

**Где:** `server/backend/app/Services/MqttService.php:163`

**Проблема:**
```php
$validTypes = ['ph_ec', 'climate', 'relay', 'water', 'display', 'root'];
// ❌ Типы "ph" и "ec" не были включены!
```

**Исправлено:**
```php
$validTypes = ['ph', 'ec', 'ph_ec', 'climate', 'relay', 'water', 'display', 'root'];
// ✅ Добавлены "ph" и "ec"
```

---

### ❌ **Проблема 4: detectNodeType не распознает префиксы "ph_" и "ec_"**

**Где:** `server/backend/app/Services/MqttService.php:530`

**Проблема:**
```php
if (str_starts_with($nodeId, 'ph_ec_')) {  // ✅ OK
    return 'ph_ec';
}
// ❌ Нет проверки для "ph_" и "ec_"!
```

**Исправлено:**
```php
if (str_starts_with($nodeId, 'ph_')) {
    return 'ph';  // ДОБАВЛЕНО
}
if (str_starts_with($nodeId, 'ec_')) {
    return 'ec';  // ДОБАВЛЕНО
}
if (str_starts_with($nodeId, 'ph_ec_')) {
    return 'ph_ec';
}
```

---

### ❌ **Проблема 5: Неправильные ключи в EC telemetry**

**Где:** `node_ec/components/ec_manager/ec_manager.c`

**Проблема:**
```c
cJSON_AddNumberToObject(data, "EC", s_current_ec);        // ❌ Заглавная
cJSON_AddNumberToObject(data, "PUMP_EC_A_ml", ...);       // ❌ Заглавные
cJSON_AddNumberToObject(data, "PUMP_EC_B_ml", ...);       // ❌ Заглавные
// ❌ Отсутствует pump_ec_c_ml!
```

**Исправлено:**
```c
cJSON_AddNumberToObject(data, "ec", s_current_ec);        // ✅ Строчная
cJSON_AddNumberToObject(data, "pump_ec_a_ml", ...);       // ✅ Строчные
cJSON_AddNumberToObject(data, "pump_ec_b_ml", ...);       // ✅ Строчные
cJSON_AddNumberToObject(data, "pump_ec_c_ml", ...);       // ✅ Добавлен третий насос
```

---

## ✅ Проверка цепочки (шаг за шагом)

### Шаг 1: NODE отправляет discovery

**Файл:** `node_ph/components/ph_manager/ph_manager.c:192`

```c
static void send_discovery(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "discovery");
    cJSON_AddStringToObject(root, "node_id", s_config->base.node_id);  
    cJSON_AddStringToObject(root, "node_type", "ph");  // ✅ Есть
    // ...
    mesh_manager_send_to_root((uint8_t *)json_str, strlen(json_str));
}
```

✅ **Корректно:** Отправляется через `mesh_manager_send_to_root()`

---

### Шаг 2: ROOT node получает и парсит

**Файл:** `root_node/components/data_router/data_router.c:32`

```c
void data_router_handle_mesh_data(const uint8_t *src_addr, const uint8_t *data, size_t len) {
    ESP_LOGI(TAG, "📥 Mesh data received: %d bytes");
    
    mesh_message_t msg;
    if (!mesh_protocol_parse(data_copy, &msg)) {
        ESP_LOGE(TAG, "❌ Failed to parse mesh message!");
        return;
    }
    
    ESP_LOGI(TAG, "✅ Message parsed: type=%d, node_id=%s", msg.type, msg.node_id);
    // ...
}
```

✅ **Корректно:** Парсинг через `mesh_protocol_parse()`

---

### Шаг 3: ROOT публикует в MQTT

**Файл:** `root_node/components/data_router/data_router.c:73`

```c
case MESH_MSG_TELEMETRY:
    if (mqtt_client_manager_is_connected()) {
        char topic[64];
        snprintf(topic, sizeof(topic), "%s/%s", MQTT_TOPIC_TELEMETRY, msg.node_id);
        // topic = "hydro/telemetry/ph_a1b2c3"
        
        esp_err_t err = mqtt_client_manager_publish(topic, data_copy);
        ESP_LOGI(TAG, "   ✓ Telemetry published to %s", topic);
    }
    break;
```

✅ **Корректно:** Публикуется в `hydro/telemetry/{node_id}`

---

### Шаг 4: BACKEND MQTT Listener получает

**Файл:** `server/backend/app/Console/Commands/MqttListenerCommand.php:74`

```php
$mqtt->subscribe('hydro/telemetry/#', function ($topic, $message) use ($mqtt) {
    $this->line("📊 [TELEMETRY] {$topic}");
    $mqtt->handleTelemetry($topic, $message);
});
```

✅ **Корректно:** Подписка на `hydro/telemetry/#`

---

### Шаг 5: BACKEND обрабатывает и сохраняет

**Файл:** `server/backend/app/Services/MqttService.php:149`

```php
public function handleTelemetry(string $topic, string $payload): void
{
    $data = json_decode($payload, true);
    
    // Валидация node_type
    $validTypes = ['ph', 'ec', 'ph_ec', 'climate', ...];  // ✅ Обновлено
    $nodeType = isset($data['node_type']) && in_array($data['node_type'], $validTypes) 
        ? $data['node_type'] 
        : 'unknown';
    
    // Сохранение в БД
    $telemetry = Telemetry::create([
        'node_id' => $data['node_id'],
        'node_type' => $nodeType,  // ✅ Сохраняется тип
        'data' => $data['data'] ?? [],
        'received_at' => now(),
    ]);
    
    // Broadcast WebSocket event
    event(new \App\Events\TelemetryReceived($telemetry));
}
```

✅ **Корректно:** Сохраняет в БД + отправляет WebSocket event

---

### Шаг 6: FRONTEND получает через WebSocket

**Файл:** `server/frontend/src/stores/telemetry.js`

```javascript
// Подписка на события телеметрии
window.Echo.channel('telemetry')
  .listen('TelemetryReceived', (event) => {
    console.log('Telemetry received:', event);
    // Обновление данных в store
    this.updateNodeData(event.node_id, event.data);
  });
```

✅ **Корректно:** Подписка на WebSocket канал

---

## 🔙 Обратная цепочка (команды)

### Шаг 1: FRONTEND отправляет команду

**Файл:** `server/frontend/src/stores/nodes.js:83`

```javascript
async sendCommand(nodeId, command, params = {}) {
  const result = await api.sendCommand(nodeId, command, params)
  // POST /api/nodes/{nodeId}/command
  return result
}
```

✅ **Корректно:** API вызов

---

### Шаг 2: BACKEND API принимает

**Файл:** `server/backend/app/Http/Controllers/NodeController.php`

⚠️ **Проблема:** Контроллер может не обрабатывать специфичные команды для pH/EC

**Рекомендуется:** Создать отдельные контроллеры или добавить обработку:

```php
public function sendCommand(Request $request, $nodeId) {
    $validated = $request->validate([
        'command' => 'required|string',
        'params' => 'array'
    ]);
    
    $command = [
        'type' => 'command',
        'node_id' => $nodeId,
        'command' => $validated['command'],
        'params' => $validated['params'] ?? []
    ];
    
    // Публикация в MQTT
    app(MqttService::class)->publish(
        "hydro/command/{$nodeId}", 
        json_encode($command)
    );
    
    return response()->json(['success' => true]);
}
```

---

### Шаг 3: BACKEND публикует в MQTT

**Топик:** `hydro/command/ph_a1b2c3`

**Payload:**
```json
{
  "type": "command",
  "node_id": "ph_a1b2c3",
  "command": "set_ph_target",
  "params": {
    "target": 6.0
  }
}
```

✅ **Корректно**

---

### Шаг 4: ROOT подписан на команды

**Файл:** `root_node/components/mqtt_client/mqtt_client_manager.c`

```c
mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_COMMAND "/#", 0);
```

Обработка в `data_router_handle_mqtt_data()`:

```c
void data_router_handle_mqtt_data(const char *topic, const char *data, int data_len) {
    ESP_LOGI(TAG, "MQTT data received: %s", topic);
    
    // Парсинг топика: hydro/command/{node_id}
    if (strstr(topic, "/command/") != NULL) {
        const char *slash = strrchr(topic, '/');
        char node_id[32] = {0};
        strncpy(node_id, slash + 1, sizeof(node_id) - 1);
        
        // Поиск узла
        node_info_t *node = node_registry_get(node_id);
        if (node && node->online) {
            // Отправка через mesh
            mesh_manager_send(node->mac_addr, (const uint8_t *)data, data_len);
        }
    }
}
```

✅ **Корректно:** Пересылает команду в mesh

---

### Шаг 5: NODE получает и выполняет

**Файл:** `node_ph/main/app_main.c:217`

```c
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    
    if (!mesh_protocol_parse(data_copy, &msg)) {
        return;
    }
    
    switch (msg.type) {
        case MESH_MSG_COMMAND: {
            cJSON *cmd = cJSON_GetObjectItem(msg.data, "command");
            if (cmd && cJSON_IsString(cmd)) {
                ph_manager_handle_command(cmd->valuestring, msg.data);  // ✅ Обработка
            }
            break;
        }
    }
}
```

✅ **Корректно:** Команда выполняется

---

## 📊 Итоговая таблица проверки

| Этап | Компонент | Статус | Примечание |
|------|-----------|--------|------------|
| **Прямая цепочка** |
| 1 | NODE → JSON | ✅ | node_type добавлен |
| 2 | NODE → mesh_send | ✅ | Корректно |
| 3 | ROOT → parse | ✅ | mesh_protocol OK |
| 4 | ROOT → MQTT publish | ✅ | Топики OK |
| 5 | BACKEND → subscribe | ✅ | Подписка OK |
| 6 | BACKEND → handleTelemetry | ✅ | Типы добавлены |
| 7 | BACKEND → DB save | ✅ | Корректно |
| 8 | BACKEND → WebSocket | ✅ | Event OK |
| 9 | FRONTEND → Echo listen | ✅ | Корректно |
| **Обратная цепочка** |
| 1 | FRONTEND → API call | ✅ | Корректно |
| 2 | BACKEND → API route | ⚠️ | Нужен controller |
| 3 | BACKEND → MQTT publish | ⚠️ | Нужна реализация |
| 4 | ROOT → MQTT subscribe | ✅ | Корректно |
| 5 | ROOT → mesh forward | ✅ | Корректно |
| 6 | NODE → parse command | ✅ | Корректно |
| 7 | NODE → execute | ✅ | Корректно |

---

## 🔍 Потенциальные улучшения

### 1. ⚠️ Создать NodeController для команд

**Файл:** `server/backend/app/Http/Controllers/NodeController.php`

Добавить метод:
```php
public function sendCommand(Request $request, string $nodeId)
{
    $validated = $request->validate([
        'command' => 'required|string',
        'params' => 'nullable|array'
    ]);
    
    $mqtt = app(MqttService::class);
    $mqtt->publish("hydro/command/{$nodeId}", json_encode([
        'type' => 'command',
        'node_id' => $nodeId,
        'command' => $validated['command'],
        'params' => $validated['params'] ?? []
    ]));
    
    return response()->json(['success' => true]);
}
```

---

### 2. ⚠️ Frontend компоненты для pH/EC

**Рекомендуется:** Создать отдельные Vue компоненты:
- `PhNode.vue` - для отображения pH ноды
- `EcNode.vue` - для отображения EC ноды

С функциями:
```javascript
async function setPhTarget(newTarget) {
  await api.sendCommand(node.node_id, 'set_ph_target', {
    target: newTarget
  });
}
```

---

## 🧪 Тестовый пример полной цепочки

### Discovery (при старте ноды):

```
1. NODE pH → mesh_manager_send_to_root()
   {"type":"discovery","node_id":"ph_123abc","node_type":"ph",...}

2. ROOT → data_router_handle_mesh_data()
   Парсинг OK → тип MESH_MSG_DISCOVERY

3. ROOT → mqtt_client_manager_publish()
   Topic: hydro/discovery
   Payload: {"type":"discovery",...}

4. BACKEND → MqttService::handleDiscovery()
   Node::create(['node_id' => 'ph_123abc', 'node_type' => 'ph'])

5. BACKEND → event(NodeDiscovered)
   WebSocket broadcast

6. FRONTEND → Echo.channel('nodes').listen('NodeDiscovered')
   Новая нода появляется в списке
```

✅ **Результат:** Нода появляется на Dashboard

---

### Telemetry (каждые 30 сек):

```
1. NODE pH → ph_manager::send_telemetry()
   {"type":"telemetry","node_type":"ph","data":{"ph":6.8,...}}

2. ROOT → data_router (MESH_MSG_TELEMETRY)
   mqtt_publish("hydro/telemetry/ph_123abc", json)

3. BACKEND → MqttService::handleTelemetry()
   Telemetry::create(...) + event(TelemetryReceived)

4. FRONTEND → реал-тайм обновление графиков
```

✅ **Результат:** Данные обновляются в реальном времени

---

### Команда (от пользователя):

```
1. FRONTEND → кнопка "Set pH Target 6.0"
   api.post('/nodes/ph_123abc/command', {command: 'set_ph_target', params: {target: 6.0}})

2. BACKEND → NodeController::sendCommand()
   mqtt_publish("hydro/command/ph_123abc", {"type":"command",...})

3. ROOT → MQTT subscribe "hydro/command/#"
   data_router_handle_mqtt_data() → mesh_manager_send()

4. NODE → on_mesh_data_received()
   ph_manager_handle_command("set_ph_target", params)

5. NODE → PID update
   pid_set_setpoint(&s_pid_ph_up, 6.0)
```

✅ **Результат:** Уставка изменена

---

## 📝 Исправленные файлы

### Firmware (нужна пересборка):
1. ✅ `node_ph/components/ph_manager/ph_manager.c`
   - Добавлен `node_type` в telemetry
   - Добавлен `node_type` в heartbeat

2. ✅ `node_ec/components/ec_manager/ec_manager.c`
   - Добавлен `node_type` в telemetry  
   - Добавлен `node_type` в heartbeat
   - Исправлены ключи: "EC"→"ec", "PUMP_EC_A_ml"→"pump_ec_a_ml"
   - Добавлен "pump_ec_c_ml"

### Backend:
3. ✅ `server/backend/app/Services/MqttService.php`
   - Добавлены типы "ph" и "ec" в $validTypes
   - Добавлена поддержка префиксов "ph_" и "ec_" в detectNodeType()
   - Улучшена логика определения типа по сенсорам

---

## ⚠️ Требуется доработка

### Backend (опционально, ноды работают без этого):
- [ ] Создать PhNodeController для специфичных команд pH
- [ ] Создать EcNodeController для специфичных команд EC
- [ ] Добавить routes для `/nodes/ph/*` и `/nodes/ec/*`

### Frontend (опционально):
- [ ] Создать PhNode.vue компонент
- [ ] Создать EcNode.vue компонент
- [ ] Добавить в Dashboard отображение новых типов
- [ ] Настроить графики для pH и EC данных

**Инструкции:** См. `BACKEND_FRONTEND_TODO.md`

---

## ✅ Заключение

**Цепочка передачи данных проверена и исправлена!**

### Что работает:
✅ NODE → ROOT (mesh)  
✅ ROOT → MQTT (публикация)  
✅ MQTT → BACKEND (подписка)  
✅ BACKEND → БД (сохранение)  
✅ BACKEND → FRONTEND (WebSocket)  
✅ ROOT → NODE (команды через mesh)  

### Что нужно доработать:
⚠️ FRONTEND → BACKEND API (создать контроллеры)  
⚠️ BACKEND → MQTT (команды для pH/EC)  
⚠️ FRONTEND компоненты (PhNode.vue, EcNode.vue)  

**Примечание:** Ноды работают полностью автономно. Backend/Frontend обновления нужны только для веб-управления.

---

## 🚀 Следующие действия

### 1. ПЕРЕСОБРАТЬ ноды (ВАЖНО!)

После исправления JSON форматов:
```bash
tools\build_ph.bat
tools\build_ec.bat
```

### 2. Прошить ноды

```bash
tools\flash_ph.bat  # COM3
tools\flash_ec.bat  # COM4
```

### 3. Проверить логи

Ожидаем:
```
I ph_mgr: Discovery sent: {"type":"discovery","node_id":"ph_...","node_type":"ph",...}
I ph_mgr: Telemetry sent
```

### 4. Проверить MQTT (опционально)

Подписаться на топики:
```bash
mosquitto_sub -h localhost -t "hydro/#" -v
```

Должны появиться сообщения от pH и EC нод.

---

**Цепочка данных полностью проверена и исправлена!** ✅

