# NODE pH/EC - Critical Control Node

**ESP32-S3 #3** - Автономный узел управления pH и электропроводностью (EC)

---

## 🎯 Назначение

Критичный узел системы Mesh Hydro для:
- Мониторинга pH и EC в питательном растворе
- Автоматической коррекции через 5 перистальтических насосов
- PID управления для точного поддержания параметров
- Автономной работы при потере связи с ROOT
- Emergency protection при критичных значениях

---

## 📦 Компоненты

| Компонент | Описание |
|-----------|----------|
| **pH Sensor** | Trema pH (I2C 0x4D) |
| **EC Sensor** | Trema EC (I2C 0x64) |
| **5x Pumps** | Перистальтические насосы (PWM) |
| **PID** | Контроллеры для pH UP/DOWN и EC |
| **Manager** | Главный менеджер с автономностью |

---

## 🔌 Распиновка ESP32-S3 #3

| GPIO | Назначение | Примечание |
|------|------------|------------|
| **I2C шина:** | | |
| 17 | I2C SCL | Clock (100 kHz) |
| 18 | I2C SDA | Data |
| **PWM насосы:** | | |
| 4 | Pump pH UP | PWM 1000 Hz |
| 5 | Pump pH DOWN | PWM 1000 Hz |
| 6 | Pump EC A | PWM 1000 Hz |
| 7 | Pump EC B | PWM 1000 Hz |
| 15 | Pump EC C | PWM 1000 Hz |

---

## 🏗️ Архитектура

```
node_ph_ec/
├── components/
│   ├── ph_sensor/         # Драйвер pH датчика
│   ├── ec_sensor/         # Драйвер EC датчика
│   ├── pump_controller/   # Управление 5 насосами
│   ├── pid_controller/    # PID алгоритм
│   └── ph_ec_manager/     # Главный менеджер
├── main/
│   ├── app_main.c         # Точка входа
│   └── CMakeLists.txt
├── CMakeLists.txt         # Главный CMakeLists
├── sdkconfig.defaults     # Конфигурация
├── partitions.csv         # Разделы flash
└── README.md              # Этот файл
```

---

## 🚀 Быстрый старт

### 1. WiFi конфигурация

Отредактируй `sdkconfig.defaults`:
```
CONFIG_ESP_WIFI_SSID="YourWiFi"
CONFIG_ESP_WIFI_PASSWORD="YourPassword"
```

### 2. Сборка

```bash
cd node_ph_ec
idf.py set-target esp32s3
idf.py build
```

### 3. Прошивка

```bash
idf.py -p COM6 flash monitor
```

**⚠️ Замени COM6 на свой порт!**

---

## 📊 Дефолтная конфигурация

```c
pH target: 6.5 (диапазон 5.5-7.5)
EC target: 2.5 (диапазон 1.5-4.0)

PID коэффициенты:
  pH UP:   Kp=2.0, Ki=0.1, Kd=0.5
  pH DOWN: Kp=2.0, Ki=0.1, Kd=0.5
  EC A/B/C: Kp=1.5, Ki=0.05, Kd=0.3
```

Все параметры сохраняются в NVS и могут быть изменены через Dashboard.

---

## 🔐 Критичные особенности

### ✅ Автономная работа

NODE pH/EC **ВСЕГДА** работает, даже если:
- ROOT offline
- MQTT offline
- WiFi offline

**Используются настройки из NVS**

### ✅ Emergency Protection

При критичных значениях (pH < 5.5 или pH > 7.5):
- Все насосы останавливаются (emergency stop)
- Emergency flag = true в telemetry
- SOS сообщение на ROOT (если online)

### ✅ Graceful Degradation

Если датчики не подключены:
- Используются дефолтные значения (pH=7.0, EC=2.0)
- Узел **НЕ падает**, продолжает работать
- Логи: `WARNING: Sensor not found - using default values`

---

## 📡 Mesh коммуникация

### Discovery (при старте):
```json
{
  "type": "discovery",
  "node_id": "ph_ec_xxx",
  "node_type": "ph_ec",
  "actuators": ["pump_ph_up", "pump_ph_down", ...],
  "heap_free": 234567,
  "wifi_rssi": -45
}
```

### Telemetry (каждые 30 сек):
```json
{
  "type": "telemetry",
  "data": {
    "ph": 6.8,
    "ec": 2.3,
    "pump_ph_up_ml": 150.5,
    "rssi_to_parent": -45
  }
}
```

### Heartbeat (каждые 60 сек):
```json
{
  "type": "heartbeat",
  "uptime": 3600,
  "heap_free": 234567,
  "autonomous": false
}
```

---

## 🧪 Тестирование

### Тест 1: Базовая работа

```bash
idf.py monitor
```

**Ожидается:**
- Узел инициализируется
- Подключается к mesh
- Отправляет discovery
- Появляется на Dashboard

### Тест 2: Управление pH

**Симулировать низкий pH в коде (временно):**
```c
s_current_ph = 6.0f; // Ниже target 6.5
```

**Ожидается:**
```
I ph_ec_mgr: pH UP: 0.50 ml (current=6.00, target=6.50)
I pump_ctrl: Pump 0 START (250 ms)
I pump_ctrl: Pump 0 STOP (0.50 ml, 250 ms)
```

### Тест 3: Проверка цепочки данных

См. **DATA_CHAIN_VERIFICATION_GUIDE.md**

---

## 📚 Документация

- `AI_INSTRUCTIONS.md` - инструкции для AI
- `DATA_CHAIN_VERIFICATION_GUIDE.md` - проверка цепочки данных
- `NODE_PH_EC_IMPLEMENTATION_COMPLETE.md` - детали реализации
- `doc/MESH_PINOUT_ALL_NODES.md` - полная распиновка

---

## ⚠️ Важные замечания

1. **Router credentials** в `sdkconfig.defaults` должны совпадать с ROOT node
2. **Mesh ID и пароль** должны совпадать со всеми узлами
3. **I2C адреса** датчиков:
   - pH: 0x4D (может отличаться - проверь документацию)
   - EC: 0x64 (может отличаться - проверь документацию)
4. **Калибровка насосов** (мл/сек) нужна для точного дозирования
5. **Emergency пороги** настраиваются в NVS

---

**NODE pH/EC - критичный узел. Автономия и безопасность превыше всего!** 🛡️
