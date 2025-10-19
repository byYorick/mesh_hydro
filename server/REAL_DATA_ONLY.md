# ✅ Настроено: Только реальные данные

**Дата:** 2025-10-18  
**Статус:** Тестовые данные удалены

---

## 🎯 ЧТО ИЗМЕНИЛОСЬ

### ❌ БЫЛО (тестовые данные):
```yaml
# docker-compose.yml
command: >
  sh -c "...
         php artisan migrate --force &&
         php artisan db:seed --force &&  ← Создавал тестовые данные
         php artisan serve ..."
```

**При каждом запуске создавались:**
- 6 тестовых узлов (ROOT, Climate, pH/EC, Relay, Display, Water)
- 576 записей телеметрии (24 часа истории)
- 5 тестовых событий

### ✅ СТАЛО (только реальные данные):
```yaml
# docker-compose.yml
command: >
  sh -c "...
         php artisan migrate --force &&
         php artisan serve ..."
```

**Теперь:**
- База данных пустая
- Данные появляются только от реальных ESP32
- Узлы создаются автоматически при первом подключении (auto-discovery)

---

## 📊 КАК ЭТО РАБОТАЕТ

### 1. ESP32 подключается к MQTT
```
ROOT Node → WiFi → MQTT (localhost:1883)
```

### 2. Отправляет discovery сообщение
```json
{
  "type": "discovery",
  "node_id": "esp32_aabbccddee",
  "node_type": "climate",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "capabilities": ["temperature", "humidity", "co2"]
}
```

### 3. Backend автоматически создаёт узел
```
MQTT Listener → Auto-Discovery → Database
```

### 4. Узел появляется на дашборде
```
Frontend → API → Database → Реальный узел ✅
```

---

## 🧪 ПРОВЕРКА

### Откройте дашборд:
```
http://localhost:3000
```

**Должно быть:**
```
┌─────────────────────────────────┐
│  Dashboard                      │
├─────────────────────────────────┤
│  📊 Total Nodes: 0              │
│  🟢 Online: 0                   │
│  🔴 Offline: 0                  │
│  ⚠️  Events: 0                  │
│                                 │
│  No nodes yet                   │
│  Connect ESP32 to start...      │
└─────────────────────────────────┘
```

**Проверено:** ✅
```json
{
  "nodes": {"total": 0, "online": 0, "offline": 0},
  "events": {"active": 0, "critical": 0},
  "telemetry": {"last_hour": 0}
}
```

---

## 🔌 ПОДКЛЮЧЕНИЕ ESP32

### 1. Прошейте ROOT Node:
```bash
cd root_node
idf.py build flash monitor
```

### 2. ROOT подключится к MQTT:
```
I mqtt_manager: MQTT connected to broker
I mqtt_manager: Published discovery message
```

### 3. Узел появится на дашборде:
```
Dashboard → ROOT Node (online) ✅
```

### 4. Прошейте другие узлы:
```bash
cd node_climate
idf.py build flash

cd node_ph_ec
idf.py build flash
```

### 5. Они появятся автоматически:
```
Dashboard:
  ✅ ROOT Node
  ✅ NODE Climate
  ✅ NODE pH/EC
```

---

## 📝 AUTO-DISCOVERY

### Как работает:

**1. ESP32 отправляет:**
```
Topic: hydro/discovery
Payload: { node_id, type, mac, capabilities }
```

**2. Backend получает:**
```
MQTT Listener → Parse → Check DB
```

**3. Если узла нет - создаёт:**
```sql
INSERT INTO nodes (
  node_id,
  name,
  type,
  mac_address,
  status
) VALUES (
  'esp32_aabbccddee',
  'NODE Climate',
  'climate',
  'AA:BB:CC:DD:EE:FF',
  'online'
);
```

**4. Frontend обновляется:**
```
WebSocket/Polling → New node → Dashboard ✅
```

---

## 🎯 ЧТО ДАЛЬШЕ

### После прошивки ESP32:

**1. Проверьте логи backend:**
```bash
docker compose logs -f backend
```

**Должно быть:**
```
[info] New node discovered: esp32_aabbccddee
[info] Node type: climate
[info] Created in database
```

**2. Проверьте дашборд:**
```
http://localhost:3000
```

**Узел должен появиться автоматически!**

**3. Проверьте телеметрию:**
```bash
docker compose logs -f mqtt_listener
```

**Должно быть:**
```
📡 Received telemetry from: esp32_aabbccddee
💾 Saved to database
```

---

## 🔧 ЕСЛИ УЗЛЫ НЕ ПОЯВЛЯЮТСЯ

### Проверьте MQTT подключение:

**1. ESP32 логи:**
```
I mqtt_manager: Connected to broker
I mqtt_manager: Published discovery
```

**2. MQTT Listener логи:**
```bash
docker compose logs mqtt_listener --tail=50
```

**Должно быть:**
```
🎧 MQTT Listener is running...
📡 Subscribed to: hydro/discovery/#
```

**3. Тест MQTT:**
```bash
# Войти в mosquitto контейнер
docker compose exec mosquitto sh

# Подписаться на топики
mosquitto_sub -t "hydro/#" -v
```

**Должны появиться сообщения от ESP32!**

---

## 📊 УПРАВЛЕНИЕ УЗЛАМИ

### Вручную добавить узел:

**Через Frontend:**
```
Dashboard → Add Node (+)
→ Заполнить форму
→ Save
```

**Через API:**
```bash
curl -X POST http://localhost:3000/api/nodes \
  -H "Content-Type: application/json" \
  -d '{
    "node_id": "manual_node_001",
    "name": "Manual Node",
    "type": "climate",
    "mac_address": "00:00:00:00:00:00"
  }'
```

### Удалить узел:

**Через Frontend:**
```
Nodes → Node Card → Delete (🗑️)
```

**Через API:**
```bash
curl -X DELETE http://localhost:3000/api/nodes/1
```

---

## ✅ ПРЕИМУЩЕСТВА

**1. Чистая система:**
- Нет мусорных тестовых данных
- Только реальные узлы
- Честная статистика

**2. Auto-Discovery:**
- ESP32 подключается → автоматически появляется
- Не нужно создавать вручную
- Plug & Play

**3. Масштабируемость:**
- Добавили новый ESP32 → появился на дашборде
- Убрали ESP32 → статус offline
- Простое управление

---

## 🎉 ИТОГО

```
╔════════════════════════════════════╗
║  НАСТРОЕНО: ТОЛЬКО РЕАЛЬНЫЕ ДАННЫЕ ║
╠════════════════════════════════════╣
║                                    ║
║  ✅ Тестовые данные удалены        ║
║  ✅ Seeding отключен               ║
║  ✅ База данных пустая             ║
║  ✅ Auto-discovery работает        ║
║                                    ║
║  Прошейте ESP32 и данные           ║
║  появятся автоматически!           ║
║                                    ║
╚════════════════════════════════════╝
```

---

**Создано:** 2025-10-18  
**Версия:** 1.0  
**Статус:** ✅ Готово

