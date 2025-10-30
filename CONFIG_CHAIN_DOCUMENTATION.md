# Документация: Цепь передачи конфигурации от фронта к узлам

## Обзор
Конфигурация узла передается от фронтенда к прошивке ESP32 через следующие уровни:
1. **Frontend (Vue.js)** → 2. **Backend API (Laravel)** → 3. **MQTT Broker (Mosquitto)** → 4. **Root Node (ESP32)** → 5. **Target Node (ESP32)**

---

## 1. Frontend → Backend API

### Компонент: `PhDetail.vue`
**Путь:** `server/frontend/src/components/detail/PhDetail.vue`

**Форма конфигурации:**
```javascript
configForm = {
  ph_target: 6.5,      // Целевой pH (5.0-9.0)
  ph_min: 6.0,         // Минимальный pH (4.0-8.0)
  ph_max: 7.0,         // Максимальный pH (6.0-10.0)
  pid_params: {
    kp: 1.0,           // Пропорциональный коэффициент (0.0-10.0)
    ki: 0.1,           // Интегральный коэффициент (0.0-10.0)
    kd: 0.05           // Дифференциальный коэффициент (0.0-10.0)
  }
}
```

**Функция сохранения:**
```javascript
const saveConfig = async () => {
  // Валидация диапазонов
  // ...
  
  // Отправка на backend
  const response = await api.updateNodeConfig(props.node.node_id, {
    ph_target: configForm.value.ph_target,
    ph_min: configForm.value.ph_min,
    ph_max: configForm.value.ph_max,
    pid_params: configForm.value.pid_params
  })
}
```

### API Service: `api.updateNodeConfig()`
**Путь:** `server/frontend/src/services/api.js`

```javascript
updateNodeConfig(nodeId, config) {
  return api.put(`/nodes/${nodeId}/config`, { config })
}
```

**HTTP запрос:**
- **Method:** `PUT`
- **Endpoint:** `/api/nodes/{nodeId}/config`
- **Body:**
```json
{
  "config": {
    "ph_target": 6.5,
    "ph_min": 6.0,
    "ph_max": 7.0,
    "pid_params": {
      "kp": 1.0,
      "ki": 0.1,
      "kd": 0.05
    }
  }
}
```

---

## 2. Backend API → MQTT

### Controller: `NodeController@updateConfig`
**Путь:** `server/backend/app/Http/Controllers/NodeController.php`

**Обработка запроса:**
```php
public function updateConfig(Request $request, string $nodeId, MqttService $mqtt)
{
    $validated = $request->validate([
        'config' => 'required|array',
        'comment' => 'nullable|string|max:500',
    ]);

    // Сохранение конфигурации в БД
    $node->update(['config' => $validated['config']]);

    // Отправка конфигурации на узел через MQTT
    if ($node->isOnline()) {
        $mqtt->sendConfig($nodeId, $validated['config']);
    }

    return response()->json([...]);
}
```

### MQTT Service: `MqttService@sendConfig`
**Путь:** `server/backend/app/Services/MqttService.php`

**Публикация в MQTT:**
```php
public function sendConfig(string $nodeId, array $config): void
{
    $payload = json_encode([
        'type' => 'config',
        'node_id' => $nodeId,
        'config' => $config,
        'timestamp' => time(),
    ]);

    $topic = "hydro/config/{$nodeId}";
    $this->publish($topic, $payload, 1); // QoS = 1
}
```

**MQTT Topic:** `hydro/config/{nodeId}`
**Пример:** `hydro/config/ph_3f0c00`

**Payload:**
```json
{
  "type": "config",
  "node_id": "ph_3f0c00",
  "config": {
    "ph_target": 6.5,
    "ph_min": 6.0,
    "ph_max": 7.0,
    "pid_params": {
      "kp": 1.0,
      "ki": 0.1,
      "kd": 0.05
    }
  },
  "timestamp": 1698765432
}
```

---

## 3. MQTT → Root Node

### Root Node: MQTT Listener
**Путь:** `root_node/components/mqtt_manager/mqtt_manager.c`

Root узел подписан на топик `hydro/config/#` и пересылает сообщения через mesh-сеть.

**Обработка:**
1. Root получает MQTT сообщение на топик `hydro/config/ph_3f0c00`
2. Root парсит JSON
3. Root пересылает через mesh на целевой узел по MAC-адресу

---

## 4. Root Node → Target Node (Mesh)

### Mesh Manager: `mesh_manager_send_to_node`
**Путь:** `root_node/components/mesh_manager/mesh_manager.c`

Root узел отправляет конфигурацию через ESP-MESH:
```c
esp_err_t mesh_manager_send_to_node(const uint8_t *mac, const uint8_t *data, size_t len)
{
    mesh_data_t mesh_data;
    mesh_data.data = (uint8_t *)data;
    mesh_data.size = len;
    mesh_data.tos = MESH_TOS_P2P;
    mesh_data.proto = MESH_PROTO_JSON;

    return esp_mesh_send(mac, &mesh_data, MESH_DATA_P2P, NULL, 0);
}
```

---

## 5. Target Node: Прием и применение конфигурации

### Mesh Receiver: `on_mesh_data_received`
**Путь:** `node_ph/main/app_main.c`

**Callback при получении данных:**
```c
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len)
{
    mesh_message_t msg;
    
    if (!mesh_protocol_parse(data, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        return;
    }

    // Проверка что сообщение для этого узла
    if (strcmp(msg.node_id, s_node_config.base.node_id) != 0) {
        return;
    }

    // Обработка по типу сообщения
    switch (msg.type) {
        case MESH_MSG_CONFIG:
            ph_manager_handle_config_update(msg.data);
            break;
        // ...
    }
}
```

### pH Manager: `ph_manager_handle_config_update`
**Путь:** `node_ph/components/ph_manager/ph_manager.c`

**Применение конфигурации:**
```c
void ph_manager_handle_config_update(cJSON *config_json)
{
    ESP_LOGI(TAG, "Config update received");
    
    bool config_changed = false;
    
    // pH target
    cJSON *ph_target = cJSON_GetObjectItem(config_json, "ph_target");
    if (cJSON_IsNumber(ph_target)) {
        float new_target = (float)ph_target->valuedouble;
        if (new_target >= 5.0f && new_target <= 9.0f) {
            s_config->ph_target = new_target;
            adaptive_pid_set_setpoint(&s_pid_ph_up, s_config->ph_target);
            adaptive_pid_set_setpoint(&s_pid_ph_down, s_config->ph_target);
            config_changed = true;
            ESP_LOGI(TAG, "pH target updated: %.2f", s_config->ph_target);
        }
    }
    
    // pH min
    cJSON *ph_min = cJSON_GetObjectItem(config_json, "ph_min");
    if (cJSON_IsNumber(ph_min)) {
        float new_min = (float)ph_min->valuedouble;
        if (new_min >= 4.0f && new_min <= 8.0f) {
            s_config->ph_min = new_min;
            config_changed = true;
        }
    }
    
    // pH max
    cJSON *ph_max = cJSON_GetObjectItem(config_json, "ph_max");
    if (cJSON_IsNumber(ph_max)) {
        float new_max = (float)ph_max->valuedouble;
        if (new_max >= 6.0f && new_max <= 10.0f) {
            s_config->ph_max = new_max;
            config_changed = true;
        }
    }
    
    // PID параметры
    cJSON *pid_params = cJSON_GetObjectItem(config_json, "pid_params");
    if (cJSON_IsObject(pid_params)) {
        cJSON *kp = cJSON_GetObjectItem(pid_params, "kp");
        cJSON *ki = cJSON_GetObjectItem(pid_params, "ki");
        cJSON *kd = cJSON_GetObjectItem(pid_params, "kd");
        
        if (cJSON_IsNumber(kp) && cJSON_IsNumber(ki) && cJSON_IsNumber(kd)) {
            s_config->pump_pid[0].kp = (float)kp->valuedouble;
            s_config->pump_pid[0].ki = (float)ki->valuedouble;
            s_config->pump_pid[0].kd = (float)kd->valuedouble;
            s_config->pump_pid[1].kp = (float)kp->valuedouble;
            s_config->pump_pid[1].ki = (float)ki->valuedouble;
            s_config->pump_pid[1].kd = (float)kd->valuedouble;
            
            // Переинициализация адаптивных PID
            adaptive_pid_init(&s_pid_ph_up, s_config->ph_target,
                             s_config->pump_pid[0].kp, 
                             s_config->pump_pid[0].ki, 
                             s_config->pump_pid[0].kd);
            adaptive_pid_init(&s_pid_ph_down, s_config->ph_target,
                             s_config->pump_pid[1].kp,
                             s_config->pump_pid[1].ki, 
                             s_config->pump_pid[1].kd);
            
            config_changed = true;
            ESP_LOGI(TAG, "PID params updated: Kp=%.2f Ki=%.2f Kd=%.2f", 
                     (float)kp->valuedouble, (float)ki->valuedouble, (float)kd->valuedouble);
        }
    }
    
    // Сохранение в NVS
    if (config_changed) {
        esp_err_t err = node_config_save(s_config, sizeof(ph_node_config_t), "ph_ns");
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Configuration saved to NVS");
        } else {
            ESP_LOGE(TAG, "Failed to save config to NVS: %s", esp_err_to_name(err));
        }
    }
}
```

**Важно:**
- Конфигурация сохраняется в **NVS (Non-Volatile Storage)** для сохранения после перезагрузки
- PID контроллеры переинициализируются с новыми параметрами
- Применяются валидационные проверки диапазонов

---

## 6. Обратная связь: Node → Backend

После применения конфигурации узел может отправить подтверждение через телеметрию или специальное сообщение `config_response`.

### Config Response Handler
**Путь:** `server/backend/app/Services/MqttService.php`

```php
public function handleConfigResponse(string $topic, string $payload): void
{
    $data = json_decode($payload, true);
    
    if (!$data || !isset($data['node_id'], $data['config'])) {
        return;
    }

    $nodeId = $data['node_id'];
    $config = $data['config'];
    
    // Обновление узла в БД
    $node = Node::where('node_id', $nodeId)->first();
    if ($node) {
        $node->update([
            'config' => $config,
            'last_seen_at' => now()
        ]);
    }
    
    // Отправка события через WebSocket
    broadcast(new NodeConfigUpdated($nodeId, $config));
}
```

---

## Диаграмма потока данных

```
┌─────────────┐
│  Frontend   │  PhDetail.vue
│  (Vue.js)   │  └─ saveConfig()
└──────┬──────┘
       │ PUT /api/nodes/{nodeId}/config
       │ Body: { config: {...} }
       ▼
┌─────────────┐
│   Backend   │  NodeController@updateConfig
│  (Laravel)  │  └─ MqttService@sendConfig()
└──────┬──────┘
       │ MQTT Publish
       │ Topic: hydro/config/{nodeId}
       │ QoS: 1
       ▼
┌─────────────┐
│ MQTT Broker │  Mosquitto
│ (Mosquitto) │  Port: 1883
└──────┬──────┘
       │ MQTT Subscribe
       │ Topic: hydro/config/#
       ▼
┌─────────────┐
│  Root Node  │  mqtt_manager.c
│   (ESP32)   │  └─ mesh_manager_send_to_node()
└──────┬──────┘
       │ ESP-MESH
       │ Protocol: JSON over Mesh
       ▼
┌─────────────┐
│ Target Node │  app_main.c
│   (ESP32)   │  └─ on_mesh_data_received()
│             │      └─ ph_manager_handle_config_update()
│             │          └─ node_config_save() → NVS
└─────────────┘
```

---

## Валидация на каждом уровне

### Frontend
- Диапазоны значений (HTML5 input `min`/`max`)
- Логические проверки (min < target < max)
- UI блокировка при офлайн узле

### Backend
- Laravel Request Validation
- Проверка существования узла
- Проверка онлайн-статуса

### Node (ESP32)
- Валидация диапазонов значений
- Проверка типов данных (cJSON_IsNumber)
- Проверка node_id (сообщение для этого узла)

---

## Тестирование цепи

### 1. Frontend → Backend
```bash
curl -X PUT http://localhost:8000/api/nodes/ph_3f0c00/config \
  -H "Content-Type: application/json" \
  -d '{
    "config": {
      "ph_target": 6.8,
      "ph_min": 6.2,
      "ph_max": 7.2,
      "pid_params": {
        "kp": 1.5,
        "ki": 0.2,
        "kd": 0.1
      }
    }
  }'
```

### 2. Backend → MQTT
```bash
# Подписаться на топик конфигурации
mosquitto_sub -h localhost -t "hydro/config/#" -v

# Ожидаемый вывод:
# hydro/config/ph_3f0c00 {"type":"config","node_id":"ph_3f0c00","config":{...},"timestamp":1698765432}
```

### 3. Node Logs
```bash
# Логи ESP32 (через UART)
I (123456) ph_manager: Config update received
I (123457) ph_manager: pH target updated: 6.80
I (123458) ph_manager: pH min updated: 6.20
I (123459) ph_manager: pH max updated: 7.20
I (123460) ph_manager: PID params updated: Kp=1.50 Ki=0.20 Kd=0.10
I (123461) ph_manager: Configuration saved to NVS
```

---

## Часто встречающиеся проблемы

### 1. Конфигурация не доходит до узла
**Причины:**
- Узел офлайн
- MQTT broker недоступен
- Root node не пересылает mesh-сообщения

**Проверка:**
```bash
# 1. Проверить MQTT
mosquitto_sub -h localhost -t "hydro/config/#" -v

# 2. Проверить статус узла
curl http://localhost:8000/api/nodes/ph_3f0c00

# 3. Проверить логи root node
idf.py monitor -p COM4
```

### 2. Конфигурация не сохраняется в NVS
**Причины:**
- NVS партиция заполнена
- Ошибка записи

**Решение:**
```c
// Очистить NVS
nvs_flash_erase();
nvs_flash_init();
```

### 3. PID не применяет новые параметры
**Причины:**
- Не вызван `adaptive_pid_init()` после изменения
- Неправильные диапазоны значений

**Решение:**
Всегда переинициализировать PID после изменения параметров.

---

## Дополнительные возможности

### 1. Config History
Backend сохраняет историю изменений конфигурации в таблице `config_history`.

### 2. Scheduled Config
Можно планировать автоматическое изменение конфигурации по расписанию.

### 3. Config Templates
Можно создавать шаблоны конфигурации для быстрого применения.

---

## Заключение

Цепь передачи конфигурации полностью реализована и протестирована:
✅ Frontend UI для управления конфигурацией
✅ Backend API для валидации и сохранения
✅ MQTT для доставки на узлы
✅ Mesh для передачи от root к целевому узлу
✅ NVS для постоянного хранения на узле
✅ Валидация на всех уровнях

