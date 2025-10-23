# AI INSTRUCTIONS - NODE pH

## 🎯 Основная задача ноды

NODE pH - это **автономный узел управления pH** в системе Mesh Hydro.

**Задачи:**
1. Мониторинг pH датчика Trema (I2C 0x4D)
2. PID управление 2 насосами (pH UP, pH DOWN)
3. Отправка данных в mesh сеть (ROOT node)
4. Автономная работа при потере связи
5. Emergency protection при критичных значениях

---

## 🏗️ Архитектура

### Компоненты:

1. **ph_sensor** - драйвер Trema pH (I2C 0x4D)
2. **pump_controller** - управление 2 насосами (GPIO 2, 3)
3. **pid_controller** - PID алгоритм для коррекции pH
4. **ph_manager** - главный менеджер узла
5. **oled_display** - OLED SSD1306 для локального отображения
6. **connection_monitor** - мониторинг связи с ROOT
7. **local_storage** - хранение данных при offline
8. **buzzer_led** - звуковая и световая индикация

### Common компоненты (из ../common/):
- mesh_manager - ESP-MESH API
- mesh_protocol - протокол сообщений
- node_config - NVS конфигурация
- mesh_config - централизованная конфигурация

---

## 🔌 Аппаратная конфигурация

### ESP32-C3:
- **I2C**: GPIO8 (SDA), GPIO9 (SCL)
- **Насосы**: GPIO2 (pH UP), GPIO3 (pH DOWN)
- **LED**: GPIO4
- **Buzzer**: GPIO5

### I2C устройства:
- **pH Sensor**: 0x4D (Trema pH)
- **OLED**: 0x3C (SSD1306 128x64)

---

## 📊 PID управление

### Логика:
- **Цель**: Поддерживать pH = target (дефолт 6.5)
- **Если pH < target**: Включить насос pH UP
- **Если pH > target**: Включить насос pH DOWN

### PID параметры (консервативные):
```c
Kp = 1.0  // Пропорциональный коэффициент (консервативный)
Ki = 0.05 // Интегральный коэффициент (консервативный)
Kd = 0.3  // Дифференциальный коэффициент (консервативный)
```

### Safety:
- Максимальное время работы насоса: 60 сек
- Cooldown между включениями: 30 сек
- Emergency stop при pH < 5.5 или pH > 7.5

---

## 📡 Mesh протокол

### 1. Discovery (при старте):
```json
{
  "type": "discovery",
  "node_id": "ph_XXXXXX",
  "node_type": "ph",
  "actuators": ["pump_ph_up", "pump_ph_down"],
  "sensors": ["ph"],
  "heap_free": 234567,
  "wifi_rssi": -45
}
```

### 2. Telemetry (каждые 30 сек):
```json
{
  "type": "telemetry",
  "node_id": "ph_XXXXXX",
  "data": {
    "ph": 6.8,
    "ph_target": 6.5,
    "pump_ph_up_ml": 150.5,
    "pump_ph_down_ml": 75.2,
    "pump_ph_up_total_ml": 5234.5,
    "pump_ph_down_total_ml": 3421.1
  }
}
```

### 3. Heartbeat (каждые 60 сек):
```json
{
  "type": "heartbeat",
  "node_id": "ph_XXXXXX",
  "uptime": 3600,
  "heap_free": 234567,
  "autonomous": false
}
```

### 4. Команды от ROOT:
```json
{
  "type": "command",
  "node_id": "ph_XXXXXX",
  "command": "set_ph_target",
  "params": {"target": 6.0}
}
```

---

## 🔐 Критичные особенности

### ✅ Автономная работа
NODE pH **ВСЕГДА** работает, даже если ROOT offline.
Используются настройки из NVS.

### ✅ Emergency Protection
При критичных значениях (pH < 5.5 или > 7.5):
- Все насосы останавливаются
- Emergency flag = true
- SOS сообщение на ROOT (если online)

### ✅ Graceful Degradation
Если датчик не подключен:
- Используется дефолтное значение pH=7.0
- Узел **НЕ падает**
- Логи: WARNING

---

## 🛠️ При модификации кода

1. **НЕ удаляй** автономный режим
2. **НЕ удаляй** emergency protection
3. **Всегда проверяй** возвраты функций (esp_err_t)
4. **Логируй** важные события (ESP_LOGI)
5. **Используй** NVS для сохранения настроек

---

## 🧪 Тестирование

### Mock режим:
В `ph_sensor.c` есть режим без реального датчика:
```c
// Если датчик не найден - используем дефолт
*ph = 7.0f;
```

### Проверка цепочки:
1. NODE отправляет discovery → ROOT → MQTT → Backend → Frontend
2. Telemetry: NODE → ROOT → MQTT → Backend → WebSocket → Frontend
3. Command: Frontend → Backend → MQTT → ROOT → NODE

---

**NODE pH - критичный узел. Автономия превыше всего!** 🛡️

