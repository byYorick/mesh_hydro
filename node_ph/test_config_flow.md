# Тестирование цепи передачи конфигурации для pH ноды

## Цепь передачи конфигурации

```
Frontend → Backend API → MQTT → Root Node → Target pH Node
                                                    ↓
Frontend ← Backend ← MQTT ← Root Node ← Config Response
```

## Что должно происходить

### 1. Frontend отправляет конфигурацию
**URL:** `PUT /api/nodes/ph_3f0c00/config`
**Body:**
```json
{
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
}
```

### 2. Backend сохраняет в БД и публикует в MQTT
**Topic:** `hydro/config/ph_3f0c00`
**Payload:**
```json
{
  "type": "config",
  "node_id": "ph_3f0c00",
  "config": {
    "ph_target": 6.8,
    "ph_min": 6.2,
    "ph_max": 7.2,
    "pid_params": {
      "kp": 1.5,
      "ki": 0.2,
      "kd": 0.1
    }
  },
  "timestamp": 1698765432
}
```

### 3. Root Node получает и пересылает через mesh
Root узел подписан на `hydro/config/#`, получает сообщение и отправляет через ESP-MESH на узел `ph_3f0c00`.

### 4. pH Node получает, применяет и отвечает
**В логах pH ноды должно появиться:**
```
I (123456) ph_manager: Config update received
I (123457) ph_manager: pH target updated: 6.80
I (123458) ph_manager: pH min updated: 6.20
I (123459) ph_manager: pH max updated: 7.20
I (123460) ph_manager: PID params updated: Kp=1.50 Ki=0.20 Kd=0.10
I (123461) ph_manager: Configuration saved to NVS
I (123462) ph_manager: Config sent to ROOT (XXX bytes)
I (123463) ph_manager: Config confirmation sent to server
```

### 5. Root Node пересылает config_response в MQTT
**Topic:** `hydro/config_response/ph_3f0c00`
**Payload:**
```json
{
  "type": "config_response",
  "node_id": "ph_3f0c00",
  "timestamp": 1698765435,
  "config": {
    "node_id": "ph_3f0c00",
    "node_type": "ph",
    "zone": "grow",
    "ph_target": 6.8,
    "ph_min": 6.2,
    "ph_max": 7.2,
    "ph_cal_offset": 0.0,
    "pumps_calibration": [...],
    "pid_params": [
      {"kp": 1.5, "ki": 0.2, "kd": 0.1},
      {"kp": 1.5, "ki": 0.2, "kd": 0.1}
    ],
    ...
  }
}
```

### 6. Backend обрабатывает config_response
**В логах backend должно появиться:**
```
[INFO] 📋 handleConfigResponse called
[INFO] 📋 Config response received: ph_3f0c00
[INFO] 📋 Node config updated in DB: ph_3f0c00
[INFO] 📋 Pump calibrations saved: ph_3f0c00 (2 pumps)
[INFO] 📋 Config response processed successfully: ph_3f0c00
```

## Тестирование

### 1. Мониторинг MQTT (в отдельном терминале)
```bash
# Подписаться на все топики config
mosquitto_sub -h localhost -t "hydro/config/#" -v

# Подписаться на config_response
mosquitto_sub -h localhost -t "hydro/config_response/#" -v
```

### 2. Мониторинг логов pH ноды
```bash
cd C:\esp\hydro\mesh\mesh_hydro\node_ph
idf.py monitor -p COM4
```

### 3. Мониторинг логов Root ноды
```bash
cd C:\esp\hydro\mesh\mesh_hydro\root_node
idf.py monitor -p COM3
```

### 4. Мониторинг логов Backend
```bash
cd C:\esp\hydro\mesh\mesh_hydro\server
docker-compose logs -f backend | grep -E "config|Config|CONFIG"
```

### 5. Отправка конфигурации через Frontend
1. Откройте `http://localhost:3000`
2. Перейдите к узлу `ph_3f0c00`
3. Раскройте секцию "🔧 Управление конфигурацией"
4. Измените параметры:
   - Целевой pH: `6.8`
   - Минимальный pH: `6.2`
   - Максимальный pH: `7.2`
   - Kp: `1.5`
   - Ki: `0.2`
   - Kd: `0.1`
5. Нажмите "Сохранить конфигурацию"

### 6. Проверка результата

**Должны увидеть:**

1. **В MQTT монitorе (hydro/config/#):**
   ```
   hydro/config/ph_3f0c00 {"type":"config","node_id":"ph_3f0c00","config":{...}}
   ```

2. **В логах Root ноды:**
   ```
   I (234567) data_router: 📥 Mesh data received: XXX bytes
   I (234568) data_router: 📋 Config for ph_3f0c00 → Mesh
   ```

3. **В логах pH ноды:**
   ```
   I (123456) ph_manager: Config update received
   I (123461) ph_manager: Configuration saved to NVS
   I (123462) ph_manager: Config sent to ROOT (XXX bytes)
   I (123463) ph_manager: Config confirmation sent to server
   ```

4. **В MQTT монitorе (hydro/config_response/#):**
   ```
   hydro/config_response/ph_3f0c00 {"type":"config_response","node_id":"ph_3f0c00","config":{...}}
   ```

5. **В логах Backend:**
   ```
   [INFO] 📋 handleConfigResponse called
   [INFO] 📋 Config response processed successfully: ph_3f0c00
   ```

6. **В Frontend:**
   - Toast уведомление: "✅ Конфигурация сохранена и отправлена на узел"
   - Обновление отображения целевого pH и диапазона

## Тестирование через curl

```bash
# Отправить конфигурацию
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

# Ожидаемый ответ:
# {"success":true,"message":"Config updated and sent to node","node":{...}}
```

## Проверка сохранения в БД

```bash
# Проверить конфигурацию в БД
curl http://localhost:8000/api/nodes/ph_3f0c00 | jq '.config'

# Должен вернуть:
# {
#   "ph_target": 6.8,
#   "ph_min": 6.2,
#   "ph_max": 7.2,
#   "pid_params": {
#     "kp": 1.5,
#     "ki": 0.2,
#     "kd": 0.1
#   }
# }
```

## Проверка сохранения в NVS на узле

После перезагрузки pH ноды, конфигурация должна сохраниться:

```bash
# Перезагрузка ноды
idf.py monitor -p COM4
# Нажмите Ctrl+T, затем Ctrl+R для перезагрузки

# В логах должно появиться:
I (12345) ph_manager: Node ID: ph_3f0c00, pH target: 6.80
```

## Возможные проблемы

### 1. Config не доходит до ноды
**Причины:**
- Нода офлайн
- Root node не подключен к mesh
- MQTT broker недоступен

**Решение:**
```bash
# Проверить статус ноды
curl http://localhost:8000/api/nodes/ph_3f0c00 | jq '.online'

# Проверить mesh подключение в логах root node
```

### 2. Config_response не приходит на backend
**Причины:**
- Backend MQTT listener не запущен
- Неправильный формат сообщения от ноды

**Решение:**
```bash
# Проверить MQTT listener
docker-compose ps | grep mqtt_listener

# Перезапустить listener
docker-compose restart mqtt_listener

# Проверить логи
docker-compose logs -f mqtt_listener
```

### 3. Конфигурация не сохраняется в NVS
**Причины:**
- NVS партиция заполнена
- Ошибка записи

**Решение:**
```c
// В коде ноды добавить очистку NVS если нужно
nvs_flash_erase();
nvs_flash_init();
```

## Успешный результат

✅ Frontend отправляет конфигурацию
✅ Backend сохраняет в БД и публикует в MQTT
✅ Root node получает и пересылает через mesh
✅ pH node применяет и сохраняет в NVS
✅ pH node отправляет config_response
✅ Root node пересылает config_response в MQTT
✅ Backend обрабатывает config_response и обновляет БД
✅ Frontend получает подтверждение

**Вся цепь работает корректно! 🎉**

