# NODE EC - Autonomous EC Control Node

**ESP32-C3** - Автономный узел управления EC (электропроводностью)

---

## 🎯 Назначение

Узел системы Mesh Hydro для:
- Мониторинга EC в питательном растворе
- Автоматической коррекции через 3 перистальтических насоса (A/B/C)
- PID управления для точного поддержания параметров
- Автономной работы при потере связи с ROOT
- Emergency protection при критичных значениях

---

## 🔌 Распиновка ESP32-C3

| GPIO | Назначение | Примечание |
|------|------------|------------|
| **I2C шина:** | | |
| 8 | I2C SDA | Data (EC датчик + OLED) |
| 9 | I2C SCL | Clock (100 kHz) |
| **Насосы:** | | |
| 2 | Pump EC A | GPIO HIGH = вкл |
| 3 | Pump EC B | GPIO HIGH = вкл |
| 4 | Pump EC C | GPIO HIGH = вкл |
| **Индикация:** | | |
| 5 | LED Status | RGB или обычный LED |
| 6 | Buzzer | 3.3V активный |

---

## 📦 I2C Устройства

| Устройство | Адрес | Описание |
|------------|-------|----------|
| **Trema EC** | 0x64 | Датчик EC (0-5.0 mS/cm) |
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
cd node_ec
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
EC target: 2.5 (диапазон 1.5-4.0 mS/cm)

PID коэффициенты (консервативные):
  EC A/B/C: Kp=0.8, Ki=0.02, Kd=0.2
```

---

## 📡 Mesh коммуникация

### Discovery:
```json
{
  "type": "discovery",
  "node_id": "ec_xxx",
  "node_type": "ec",
  "actuators": ["pump_ec_a", "pump_ec_b", "pump_ec_c"],
  "sensors": ["ec"]
}
```

### Telemetry:
```json
{
  "type": "telemetry",
  "data": {
    "ec": 2.3,
    "pump_ec_a_ml": 250.5,
    "pump_ec_b_ml": 245.2,
    "pump_ec_c_ml": 50.1
  }
}
```

---

**NODE EC - автономный узел управления EC** 🛡️

