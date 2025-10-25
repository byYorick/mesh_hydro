# 🔗 Диаграмма полной цепочки событий насосов

## 📊 Архитектура системы

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           ПОЛНАЯ ЦЕПОЧКА СОБЫТИЙ НАСОСОВ                        │
└─────────────────────────────────────────────────────────────────────────────────┘

┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   NODE pH   │    │   NODE EC   │    │ NODE ph_ec  │
│             │    │             │    │             │
│ pump_events │    │ pump_events │    │ pump_events │
│ PH_UP/DOWN  │    │ EC_A/B/C    │    │ ALL PUMPS   │
└─────┬───────┘    └─────┬───────┘    └─────┬───────┘
       │                  │                  │
       │ mesh_manager_send_to_root()         │
       │                  │                  │
       └──────────────────┼──────────────────┘
                          │
                    ┌─────▼───────┐
                    │ ROOT NODE   │
                    │             │
                    │ data_router │
                    │             │
                    │ mesh → MQTT │
                    └─────┬───────┘
                          │
                    ┌─────▼───────┐
                    │ MQTT Broker │
                    │ (Mosquitto) │
                    │             │
                    │ Topic:      │
                    │ hydro/event │
                    └─────┬───────┘
                          │
                    ┌─────▼───────┐
                    │   SERVER    │
                    │             │
                    │ MqttService │
                    │             │
                    │ Event::create│
                    └─────┬───────┘
                          │
                    ┌─────▼───────┐
                    │ PostgreSQL  │
                    │             │
                    │ events table│
                    │ JSONB data  │
                    └─────┬───────┘
                          │
                    ┌─────▼───────┐
                    │ WebSocket   │
                    │ (Reverb)    │
                    │             │
                    │ Broadcasting│
                    └─────┬───────┘
                          │
                    ┌─────▼───────┐
                    │  FRONTEND   │
                    │             │
                    │ Vue.js      │
                    │             │
                    │ Events.vue  │
                    │ Real-time   │
                    └─────────────┘
```

## 🔄 Детальный поток данных

### **1. Нода отправляет событие:**
```c
// В pump_events.c
pump_events_send_start_event(
    PUMP_PH_UP,
    duration_ms,
    dose_ml,
    &pid_data,
    current_ph,
    ph_target,
    emergency_mode,
    autonomous_mode,
    rssi
);
```

### **2. ROOT нода маршрутизирует:**
```c
// В data_router.c
data_router_handle_mesh_data() {
    // Парсинг JSON
    mesh_protocol_parse(data, &msg);
    
    // Публикация в MQTT
    mqtt_client_manager_publish(topic, data);
}
```

### **3. MQTT Broker получает:**
```
Topic: hydro/event/ph_001
Payload: {
  "type": "event",
  "node_id": "ph_001",
  "node_type": "ph",
  "data": {
    "event_type": "pump_start",
    "pump_id": 0,
    "dose_ml": 1.5,
    "pid_data": { ... }
  }
}
```

### **4. Сервер обрабатывает:**
```php
// В MqttService.php
public function handleEvent($topic, $payload) {
    $data = json_decode($payload, true);
    
    // Специальная обработка насосов
    if (strpos($data['data']['event_type'], 'pump_') === 0) {
        $message = $this->translatePumpEventMessage($data['data']);
        $level = $this->getPumpEventLevel($data['data']);
    }
    
    // Сохранение в БД
    $event = Event::create([
        'node_id' => $data['node_id'],
        'level' => $level,
        'message' => $message,
        'data' => $data['data']
    ]);
    
    // Broadcast через WebSocket
    event(new EventCreated($event));
}
```

### **5. База данных сохраняет:**
```sql
INSERT INTO events (
    node_id, level, message, data
) VALUES (
    'ph_001',
    'info',
    '🚰 Насос pH UP запущен: 1.5 мл (3000 мс)',
    '{"event_type": "pump_start", "pump_id": 0, ...}'
);
```

### **6. Фронтенд получает:**
```javascript
// В events store
onMounted(() => {
    // WebSocket подключение
    this.socket.on('event.created', (event) => {
        this.events.push(event);
        this.updateStats();
    });
});
```

## 📊 Типы событий насосов

| Тип события | Уровень | Сообщение | Описание |
|-------------|---------|-----------|----------|
| `pump_start` | INFO | 🚰 Насос {name} запущен: {dose} мл | Насос включен |
| `pump_stop` | INFO | 🛑 Насос {name} остановлен: {dose} мл | Насос выключен |
| `pump_emergency_stop` | CRITICAL | 🚨 Аварийная остановка насоса {name} | Аварийная остановка |
| `pump_timeout` | WARNING | ⏰ Таймаут насоса {name} | Таймаут работы |
| `pump_calibration_start` | INFO | 🔧 Начало калибровки насоса {name} | Начало калибровки |
| `pump_calibration_end` | INFO | ✅ Калибровка насоса {name} завершена | Конец калибровки |

## 🎯 Названия насосов по типам нод

### **pH нода:**
- Pump ID 0 → "pH UP"
- Pump ID 1 → "pH DOWN"

### **EC нода:**
- Pump ID 0 → "EC A"
- Pump ID 1 → "EC B"  
- Pump ID 2 → "EC C"

### **ph_ec нода:**
- Pump ID 0 → "pH UP"
- Pump ID 1 → "pH DOWN"
- Pump ID 2 → "EC A"
- Pump ID 3 → "EC B"
- Pump ID 4 → "EC C"

## 🔧 Данные PID в событиях

```json
{
  "pid_data": {
    "kp": 1.5,           // Пропорциональный коэффициент
    "ki": 0.2,           // Интегральный коэффициент  
    "kd": 0.8,           // Дифференциальный коэффициент
    "setpoint": 6.5,     // Заданное значение
    "current_value": 6.0, // Текущее значение
    "error": 0.5,        // Ошибка (setpoint - current)
    "output": 1.5,       // Выход PID
    "integral": 0.0,     // Интегральная составляющая
    "derivative": 0.0,    // Дифференциальная составляющая
    "enabled": true      // Включен ли PID
  }
}
```

## ✅ Результат

Полная цепочка событий насосов работает корректно:

1. **Ноды** отправляют события с данными PID
2. **ROOT нода** маршрутизирует через MQTT
3. **Сервер** обрабатывает и переводит сообщения
4. **БД** сохраняет с JSONB данными
5. **Фронтенд** отображает в реальном времени

**Система готова к использованию! 🚀**
