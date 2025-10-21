# NODE pH - Autonomous pH Control Node

**ESP32-C3** - Автономный узел управления pH

---

## 🎯 Назначение

Узел системы Mesh Hydro для:
- Мониторинга pH в питательном растворе
- Автоматической коррекции через 2 перистальтических насоса (UP/DOWN)
- PID управления для точного поддержания параметров
- Автономной работы при потере связи с ROOT
- Emergency protection при критичных значениях

---

## 🔌 Распиновка ESP32-C3

| GPIO | Назначение | Примечание |
|------|------------|------------|
| **I2C шина:** | | |
| 8 | I2C SDA | Data (pH датчик + OLED) |
| 9 | I2C SCL | Clock (100 kHz) |
| **Насосы:** | | |
| 2 | Pump pH UP | GPIO HIGH = вкл |
| 3 | Pump pH DOWN | GPIO HIGH = вкл |
| **Индикация:** | | |
| 4 | LED Status | RGB или обычный LED |
| 5 | Buzzer | 3.3V активный |

---

## 📦 I2C Устройства

| Устройство | Адрес | Описание |
|------------|-------|----------|
| **Trema pH** | 0x4D | Датчик pH (0-14) |
| **OLED SSD1306** | 0x3C | Дисплей 128x64 |

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
cd node_ph
idf.py set-target esp32c3
idf.py build
```

### 3. Прошивка

```bash
idf.py -p COMX flash monitor
```

---

## 📊 Дефолтная конфигурация

```c
pH target: 6.5 (диапазон 5.5-7.5)

PID коэффициенты (консервативные):
  pH UP:   Kp=1.0, Ki=0.05, Kd=0.3
  pH DOWN: Kp=1.0, Ki=0.05, Kd=0.3
```

---

## 📡 Mesh коммуникация

### Discovery:
```json
{
  "type": "discovery",
  "node_id": "ph_xxx",
  "node_type": "ph",
  "actuators": ["pump_ph_up", "pump_ph_down"],
  "sensors": ["ph"]
}
```

### Telemetry:
```json
{
  "type": "telemetry",
  "data": {
    "ph": 6.8,
    "pump_ph_up_ml": 150.5,
    "pump_ph_down_ml": 75.2
  }
}
```

---

**NODE pH - автономный узел управления pH** 🛡️

