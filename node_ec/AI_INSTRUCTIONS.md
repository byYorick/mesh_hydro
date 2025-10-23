# AI INSTRUCTIONS - NODE EC

## 🎯 Основная задача ноды

NODE EC - это **автономный узел управления EC (электропроводностью)** в системе Mesh Hydro.

**Задачи:**
1. Мониторинг EC датчика Trema (I2C 0x64)
2. PID управление 3 насосами (EC A, EC B, EC C)
3. Отправка данных в mesh сеть (ROOT node)
4. Автономная работа при потере связи
5. Emergency protection при критичных значениях

---

## 🏗️ Архитектура

### Компоненты:

1. **ec_sensor** - драйвер Trema EC (I2C 0x64)
2. **pump_controller** - управление 3 насосами (GPIO 2, 3, 4)
3. **pid_controller** - PID алгоритм для коррекции EC
4. **ec_manager** - главный менеджер узла
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
- **Насосы**: GPIO2 (EC A), GPIO3 (EC B), GPIO4 (EC C)
- **LED**: GPIO5
- **Buzzer**: GPIO6

### I2C устройства:
- **EC Sensor**: 0x64 (Trema EC)
- **OLED**: 0x3C (SSD1306 128x64)

---

## 📊 PID управление

### Логика:
- **Цель**: Поддерживать EC = target (дефолт 2.5 mS/cm)
- **Если EC < target**: Включить насосы A, B, C (пропорционально)

### Распределение насосов:
```c
// PID выход распределяется на 3 насоса:
// A = 50% от нужного объема
// B = 40% от нужного объема
// C = 10% от нужного объема (микроэлементы)
```

### PID параметры (консервативные):
```c
Kp = 0.8  // Пропорциональный коэффициент (консервативный)
Ki = 0.02 // Интегральный коэффициент (консервативный)
Kd = 0.2  // Дифференциальный коэффициент (консервативный)
```

### Safety:
- Максимальное время работы насоса: 60 сек
- Cooldown между включениями: 30 сек
- Emergency stop при EC > 4.0

---

## 📡 Mesh протокол

### 1. Discovery (при старте):
```json
{
  "type": "discovery",
  "node_id": "ec_XXXXXX",
  "node_type": "ec",
  "actuators": ["pump_ec_a", "pump_ec_b", "pump_ec_c"],
  "sensors": ["ec"],
  "heap_free": 234567,
  "wifi_rssi": -45
}
```

### 2. Telemetry (каждые 30 сек):
```json
{
  "type": "telemetry",
  "node_id": "ec_XXXXXX",
  "data": {
    "ec": 2.3,
    "ec_target": 2.5,
    "pump_ec_a_ml": 250.5,
    "pump_ec_b_ml": 245.2,
    "pump_ec_c_ml": 50.1,
    "pump_ec_a_total_ml": 15234.5,
    "pump_ec_b_total_ml": 12421.1,
    "pump_ec_c_total_ml": 2134.3
  }
}
```

### 3. Heartbeat (каждые 60 сек):
```json
{
  "type": "heartbeat",
  "node_id": "ec_XXXXXX",
  "uptime": 3600,
  "heap_free": 234567,
  "autonomous": false
}
```

### 4. Команды от ROOT:
```json
{
  "type": "command",
  "node_id": "ec_XXXXXX",
  "command": "set_ec_target",
  "params": {"target": 2.0}
}
```

---

## 🔐 Критичные особенности

### ✅ Автономная работа
NODE EC **ВСЕГДА** работает, даже если ROOT offline.
Используются настройки из NVS.

### ✅ Emergency Protection
При критичных значениях (EC > 4.0):
- Все насосы останавливаются
- Emergency flag = true
- SOS сообщение на ROOT (если online)

### ✅ Graceful Degradation
Если датчик не подключен:
- Используется дефолтное значение EC=2.0
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
В `ec_sensor.c` есть режим без реального датчика:
```c
// Если датчик не найден - используем дефолт
*ec = 2.0f;
```

### Проверка цепочки:
1. NODE отправляет discovery → ROOT → MQTT → Backend → Frontend
2. Telemetry: NODE → ROOT → MQTT → Backend → WebSocket → Frontend
3. Command: Frontend → Backend → MQTT → ROOT → NODE

---

**NODE EC - критичный узел. Автономия превыше всего!** 🛡️

