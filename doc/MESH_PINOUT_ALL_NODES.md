# 📌 GPIO РАСПИНОВКА ВСЕХ УЗЛОВ MESH HYDRO V2

**Дата:** 17 октября 2025

---

## 🔌 ROOT NODE (ESP32-S3 #1)

**Функция:** Координатор mesh + MQTT gateway

| GPIO | Назначение | Примечание |
|------|------------|------------|
| 9 | I2C1 SCL (OLED) | SSD1306 0x3C |
| 8 | I2C1 SDA (OLED) | SSD1306 0x3C |

**Дисплей:**
- OLED SSD1306 128x64 (индикация сети/статусов)

**Интерфейсы:**
- WiFi (встроенный)
- ESP-WIFI-MESH (встроенный)

**Память:**
- RAM: 512 KB
- PSRAM: 8 MB (опционально, не критично)
- Flash: 4 MB минимум

---

## 📺 NODE DISPLAY (ESP32-S3 #2)

**Функция:** TFT дисплей + LVGL интерфейс

### LCD ILI9341 (SPI):

| GPIO | Назначение | Примечание |
|------|------------|------------|
| 11 | MOSI | SPI Master Out |
| 12 | SCLK | SPI Clock |
| 10 | CS | Chip Select |
| 9 | DC | Data/Command |
| 14 | RST | Reset |
| 15 | Backlight | PWM управление яркостью |

### Энкодер:

| GPIO | Назначение | Примечание |
|------|------------|------------|
| 4 | CLK (A) | Quadrature encoder A |
| 5 | DT (B) | Quadrature encoder B |
| 6 | SW (Button) | Кнопка энкодера (active low) |

**Память:**
- RAM: 512 KB
- PSRAM: 8 MB (используется для LVGL буферов)
- Flash: 4 MB минимум

---

## 🧪 NODE pH/EC (ESP32-S3 #3 и дополнительные)

**Функция:** Автономный контроль pH/EC (КРИТИЧНЫЙ УЗЕЛ!)

### I2C шина:

| GPIO | Назначение | Адрес I2C |
|------|------------|-----------|
| 17 | SCL | - |
| 18 | SDA | - |
| - | Trema pH | 0x0A |
| - | Trema EC | 0x08 |
| - | OLED SSD1306 | 0x3C |

### Насосы (5 штук):

| GPIO | Насос | Назначение |
|------|-------|------------|
| 1 | Насос 1 | pH UP (повышение pH) |
| 2 | Насос 2 | pH DOWN (понижение pH) |
| 3 | Насос 3 | EC A (питательный раствор A) |
| 4 | Насос 4 | EC B (питательный раствор B) |
| 5 | Насос 5 | EC C (питательный раствор C) |

**Управление:** GPIO HIGH = насос ВКЛ, GPIO LOW = насос ВЫКЛ

### Индикация и управление:

| GPIO | Назначение | Примечание |
|------|------------|------------|
| 15 | LED статус | RGB или 3 LED (красный/желтый/зеленый) |
| 16 | Buzzer | Пищалка 3.3V (аварийные сигналы) |
| 19 | Кнопка MODE | Переключение режимов (с подтяжкой вверх) |

**Память:**
- RAM: 512 KB
- PSRAM: 8 MB (для локального буфера данных)
- Flash: 4 MB минимум (OTA dual partition)

---

## 🌡️ NODE CLIMATE (ESP32)

**Функция:** Датчики климата (только измерения!)

### I2C шина:

| GPIO | Назначение | Адрес I2C |
|------|------------|-----------|
| 17 | SCL | - |
| 18 | SDA | - |
| - | SHT3x | 0x44 |
| - | CCS811 | 0x5A |
| - | Trema Lux | 0x12 |

### OLED дисплей (I2C1):

| GPIO | Назначение |
|------|------------|
| 26 | SCL |
| 25 | SDA |
| - | SSD1306 | 0x3C |

**Примечание:** БЕЗ исполнительных устройств! Только датчики.

**Память:**
- RAM: 320 KB (ESP32 стандартный)
- Flash: 4 MB

---

## 🔌 NODE RELAY (ESP32)

**Функция:** Управление климатом (форточки, вентиляция, свет)

### Актуаторы:

| GPIO | Назначение | Управление | Напряжение |
|------|------------|------------|------------|
| 1 | Линейный актуатор форточка 1 | Реле (вкл/выкл) | 12V DC |
| 2 | Линейный актуатор форточка 2 | Реле (вкл/выкл) | 12V DC |
| 3 | Вентилятор | Реле | 220V AC или 12V DC |
| 7 | LED свет | PWM (LEDC) | 12V DC через MOSFET |

**Линейные актуаторы:**
- Ход: 50-100 мм
- GPIO HIGH = актуатор ВЫДВИГАЕТСЯ (форточка ОТКРЫВАЕТСЯ)
- GPIO LOW = актуатор ВТЯГИВАЕТСЯ (форточка ЗАКРЫВАЕТСЯ)
- Время полного хода: 10-30 секунд (настраивается в NVS)
- Концевики: встроенные в актуатор

**PWM свет (GPIO 7):**
- Частота: 5000 Hz
- Разрешение: 8 бит (0-255)
- Яркость: 0-100% (плавная регулировка)
- Управление: через LED Controller (LEDC)

**Схема PWM подключения:**
```
ESP32 GPIO7 → LEDC PWM
    ↓
MOSFET Gate (IRF540N/IRLZ44N)
    ↓
LED лента/панель 12V (+)
    ↓
12V Power Supply (-)
```

**Память:**
- RAM: 320 KB
- Flash: 4 MB

---

## 💧 NODE WATER (ESP32-C3)

**Функция:** Управление насосами и клапанами воды

### Насосы и клапаны:

| GPIO | Назначение | Тип | Напряжение |
|------|------------|-----|------------|
| 1 | Насос подачи воды | Реле | 12V DC |
| 2 | Насос слива | Реле | 12V DC |
| 3 | Клапан зона 1 | Соленоидный клапан | 12V DC |
| 4 | Клапан зона 2 | Соленоидный клапан | 12V DC |
| 5 | Клапан зона 3 | Соленоидный клапан | 12V DC |
| 6 | Датчик уровня (опц.) | Digital/Analog | 3.3V |

**Память:**
- RAM: 400 KB (ESP32-C3)
- Flash: 4 MB

---

## 🔗 I2C АДРЕСА УСТРОЙСТВ

### Сводная таблица:

| Устройство | Адрес | Узел | Альтернативный |
|------------|-------|------|----------------|
| **Датчики pH/EC:** | | | |
| Trema pH | 0x0A | pH/EC | - |
| Trema EC | 0x08 | pH/EC | - |
| OLED SSD1306 | 0x3C | pH/EC | 0x3D |
| **Датчики Climate:** | | | |
| SHT3x | 0x44 | Climate | 0x45 |
| CCS811 | 0x5A | Climate | 0x5B |
| Trema Lux | 0x12 | Climate | - |

**Конфликты адресов:** НЕТ (каждый узел имеет свою I2C шину)

---

## ⚡ ПИТАНИЕ УЗЛОВ

### Рекомендации:

| Узел | Потребление | БП | Примечание |
|------|-------------|-----|------------|
| ROOT | ~200 mA | 5V 1A | Только WiFi/Mesh |
| Display | ~300-500 mA | 5V 2A | TFT подсветка + LVGL |
| pH/EC | ~500-800 mA | 5V 2A + 12V 2A | Насосы 12V отдельно! |
| Climate | ~150 mA | 5V 1A | Только датчики |
| Relay | ~200 mA + нагрузка | 5V 1A + 12V/220V | Актуаторы 12V, вент. 220V |
| Water | ~150 mA + нагрузка | 5V 1A + 12V 2A | Насосы 12V |

**Важно:**
- ESP32 питание: 5V (через USB или стабилизатор)
- Насосы/актуаторы: 12V (отдельный БП!)
- Вентилятор: 220V (через реле SSR)
- Изоляция: Оптопары между ESP32 (3.3V) и нагрузкой (12V/220V)

---

## 🛠️ СХЕМЫ ПОДКЛЮЧЕНИЯ

### Насосы (через оптопару + MOSFET):

```
ESP32 GPIO → Резистор 220Ω → Оптопара PC817 (LED+)
                              Оптопара PC817 (LED-) → GND

Оптопара PC817 (Transistor C) → MOSFET Gate (IRLZ44N)
Оптопара PC817 (Transistor E) → GND

MOSFET Source → GND
MOSFET Drain → Насос 12V (-)
Насос 12V (+) → 12V Power Supply (+)
```

### PWM свет (через MOSFET):

```
ESP32 GPIO7 (PWM) → Резистор 10kΩ → MOSFET Gate (IRF540N)
MOSFET Source → GND
MOSFET Drain → LED лента 12V (-)
LED лента 12V (+) → 12V Power Supply (+)

Резистор 10kΩ между Gate и Source (pull-down)
```

### Линейные актуаторы:

```
ESP32 GPIO → Реле (3.3V coil)
Реле NO → Линейный актуатор (+) 12V
Реле COM → 12V Power Supply (+)
Линейный актуатор (-) → GND
```

---

## 📚 ССЫЛКИ НА КОМПОНЕНТЫ ИЗ HYDRO1.0

### Что портируем БЕЗ изменений:

| Компонент | Источник | Назначение в mesh |
|-----------|----------|-------------------|
| sensor_manager | components/sensor_manager/ | common/sensor_base/ |
| pump_manager | components/pump_manager/ | node_ph_ec/components/ |
| adaptive_pid | components/adaptive_pid/ | node_ph_ec/components/ |
| lvgl_ui | components/lvgl_ui/ | node_display/components/ |
| lcd_ili9341 | components/lcd_ili9341/ | node_display/components/ |
| encoder | components/encoder/ | node_display/components/ |
| config_manager | components/config_manager/ | common/node_config/ |
| error_handler | components/error_handler/ | common/ (для всех) |
| i2c_bus | components/i2c_bus/ | common/ (для всех) |

### Что создаем НОВОЕ:

- mesh_manager - управление ESP-WIFI-MESH
- mesh_protocol - протокол JSON
- node_registry - реестр узлов (ROOT)
- mqtt_client - MQTT клиент (ROOT)
- data_router - маршрутизация (ROOT)
- oled_display - OLED SSD1306 (pH/EC)
- connection_monitor - мониторинг связи (pH/EC)
- local_storage - локальный буфер (pH/EC)
- climate_logic - резервная логика (ROOT)
- vent_actuators - линейные актуаторы (Relay)
- light_controller - PWM диммирование (Relay)

---

## ✅ ИТОГО

**План сохранен!**

Файлы:
- MESH_HYDRO_V2_FINAL_PLAN.md (полный план с кодом)
- MESH_ARCHITECTURE_PLAN.md (архитектура)
- MESH_PINOUT_ALL_NODES.md (GPIO распиновка)

**Следующий шаг:** Начать реализацию когда будете готовы!

