# NODE Climate

ESP32 - Датчики климата (температура, влажность, CO2, освещенность)

## 🎯 Назначение

**NODE Climate** - узел мониторинга климатических условий в теплице.

### 🌡️ Ключевые функции:
- 📊 Чтение датчиков каждые 10 секунд
- 📤 Отправка телеметрии на ROOT
- 🔁 Retry логика (3 попытки при ошибках)
- 🔄 Компенсация CO2 по температуре/влажности

### ⚠️ ВАЖНО:
- ❌ **НЕТ ИСПОЛНИТЕЛЕЙ!** - только датчики
- ❌ **НЕТ ЛОГИКИ УПРАВЛЕНИЯ!** - решения принимает ROOT
- ✅ **ТОЛЬКО МОНИТОРИНГ** - измерение параметров

## 📦 Датчики

| Датчик | Интерфейс | I2C адрес | Назначение |
|--------|-----------|-----------|------------|
| **SHT3x** | I2C | 0x44 | Температура + влажность |
| **CCS811** | I2C | 0x5A | CO2 + TVOC |
| **Trema Lux** | I2C | 0x12 | Освещенность (lux) |

## 🔌 Распиновка

| GPIO | Назначение | Примечание |
|------|------------|------------|
| **I2C шина:** | | |
| 17 | I2C SCL | Clock |
| 18 | I2C SDA | Data |
| **Индикация (опц.):** | | |
| 15 | LED статус | Зеленый=OK, желтый=degraded |

## 📦 Компоненты

### Из common/:
- `mesh_manager` - Mesh NODE режим
- `mesh_protocol` - JSON протокол
- `node_config` - NVS конфигурация

### Собственные (components/):
- ✅ `sht3x_driver` - Sensirion SHT3x (temp + humidity)
- ✅ `ccs811_driver` - AMS CCS811 (CO2 + TVOC)
- ✅ `lux_sensor` - Trema Lux (освещенность)
- ✅ `climate_controller` - главная логика

## 🚀 Сборка и прошивка

### ✅ WiFi настроен
В файле `main/app_main.c` (строки 168-169) указаны данные WiFi роутера:
```c
.router_ssid = "Yorick",
.router_password = "pro100parol",
```
⚠️ **При необходимости измените на ваш WiFi!** Эти данные должны совпадать с ROOT узлом.

### Быстрая сборка (с автонастройкой):
```bash
.\build.bat
```

### Ручная сборка:
```bash
cd node_climate
idf.py set-target esp32
idf.py build
idf.py -p COM4 flash monitor
```

## 📊 Ожидаемый вывод

```
I CLIMATE: ========================================
I CLIMATE: === NODE Climate Starting ===
I CLIMATE: ========================================
I CLIMATE: [Step 1/7] Initializing NVS...
I CLIMATE: [Step 2/7] Loading configuration...
I CLIMATE: Loaded: climate_001 (Greenhouse Zone 1)
I CLIMATE: Targets: Temp=24.0°C, Hum=65%, CO2=800ppm, Lux=500
I CLIMATE: [Step 3/7] Initializing I2C...
I CLIMATE: I2C initialized (SDA=18, SCL=17)
I CLIMATE: [Step 4/7] Initializing Sensors...
I sht3x: SHT3x initialized successfully
I ccs811: CCS811 initialized successfully
I lux_sensor: Lux sensor initialized successfully
I CLIMATE: [Step 5/7] Initializing Mesh (NODE mode)...
I mesh_manager: NODE mode
I mesh_manager: Parent connected
I CLIMATE: [Step 6/7] Initializing Climate Controller...
I climate_ctrl: Climate Controller initialized
I CLIMATE: [Step 7/7] Starting Climate Controller...
I climate_ctrl: Main task running (interval: 10000 ms)
I CLIMATE: ========================================
I CLIMATE: === NODE Climate Running ===
I CLIMATE: Node ID: climate_001
I CLIMATE: Read interval: 10000 ms
I CLIMATE: ========================================
I climate_ctrl: Telemetry sent: 24.5°C, 65%, 450ppm, 1200lux
```

## 🧪 Тестирование

### Тест 1: Чтение датчиков

После прошивки датчики должны читаться каждые 10 секунд:
```
I climate_ctrl: Telemetry sent: 24.3°C, 67%, 445ppm, 1180lux
I climate_ctrl: Telemetry sent: 24.4°C, 66%, 448ppm, 1190lux
```

### Тест 2: Подключение к ROOT

При подключении к ROOT должно появиться:
```
I mesh_manager: Parent connected
```

В логах ROOT:
```
I node_registry: New node added: climate_001
I data_router: Telemetry from climate_001 → MQTT
```

### Тест 3: Проверка через MQTT

На сервере:
```bash
mosquitto_sub -h 192.168.1.100 -t "hydro/telemetry" -v
```

Должны появляться сообщения:
```json
{
  "type": "telemetry",
  "node_id": "climate_001",
  "data": {
    "temp": 24.5,
    "humidity": 65.0,
    "co2": 450,
    "lux": 1200
  }
}
```

## ✅ Статус реализации

| Компонент | Статус | Описание |
|-----------|--------|----------|
| sht3x_driver | ✅ ГОТОВ | Полная реализация I2C драйвера |
| ccs811_driver | ✅ ГОТОВ | С компенсацией temp/humidity |
| lux_sensor | ✅ ГОТОВ | Trema Lux датчик |
| climate_controller | ✅ ГОТОВ | Retry логика, телеметрия |
| app_main.c | ✅ ГОТОВ | 7-шаговая инициализация |
| CMakeLists.txt | ✅ ГОТОВ | Все зависимости |
| sdkconfig | ✅ ГОТОВ | ESP32, I2C, watchdog |

**Готовность: 100%** ✅

**ПОЛНОСТЬЮ ГОТОВ К ПРОШИВКЕ И ТЕСТИРОВАНИЮ!**

## 📚 Документация

- `AI_INSTRUCTIONS.md` - полная инструкция (370+ строк)
- `../common/AI_INSTRUCTIONS.md` - общие компоненты
- `../doc/MESH_HYDRO_V2_FINAL_PLAN.md` - общий план

## 🔧 Примечания

- Датчики на одной I2C шине (адреса различаются)
- CCS811 требует прогрева (первые 20 минут данные неточные)
- Retry логика обеспечивает стабильность
- Watchdog защищает от зависания

**Приоритет:** 🟡 СРЕДНИЙ (Фаза 7)

**Время реализации:** ЗАВЕРШЕНО! (2 дня)
