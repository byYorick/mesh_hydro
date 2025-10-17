# 🌐 ПЛАН АРХИТЕКТУРЫ MESH HYDRO SYSTEM V2

**Дата:** 17 октября 2025  
**Версия:** 2.0 Final  
**Базовый проект:** hydro1.0 (для примера компонентов)

---

## 🎯 ОБЩАЯ КОНЦЕПЦИЯ

### Что создаем:
Распределенная mesh-система гидропонного мониторинга с полной автономией критичных узлов.

### Принципиальное отличие от hydro1.0:
- **Было:** Монолитная система (все на одном ESP32-S3)
- **Станет:** Распределенная mesh-сеть (ROOT + N узлов разных типов)

---

## 📊 ТОПОЛОГИЯ СИСТЕМЫ

```
┌────────────────────────────────────────────────────────────┐
│              СЕРВЕР (ПК на Windows/Linux)                  │
│  • Laravel 10 + PHP 8.2                                    │
│  • Vue.js 3 + Vuetify 3 (Material Design)                  │
│  • PostgreSQL 15 (вся история телеметрии)                  │
│  • Mosquitto MQTT Broker                                   │
│  • Nginx (reverse proxy)                                   │
│  • Telegram Bot (уведомления + команды)                    │
│  • SMS Gateway (критичные события → телефон)               │
└─────────────────┬──────────────────────────────────────────┘
                  │
           MQTT over WiFi
           (QoS 1, Topics: hydro/*)
                  │
┌─────────────────▼──────────────────────────────────────────┐
│         ROOT NODE (ESP32-S3 #1) ⭐ КООРДИНАТОР             │
│  • ESP-WIFI-MESH ROOT координатор                          │
│  • WiFi STA → роутер (192.168.x.x)                        │
│  • MQTT клиент (publish/subscribe)                         │
│  • Node Registry (учет всех подключенных узлов)            │
│  • Data Router (маршрутизация NODE ↔ MQTT ↔ Server)       │
│  • Climate Logic (резервная логика если Climate offline)   │
│  • OTA Coordinator (раздача прошивок узлам)                │
│  БЕЗ дисплея! БЕЗ датчиков! БЕЗ исполнителей!             │
└───┬────┬─────────┬──────────┬──────────┬──────────────────┘
    │    │         │          │          │
    │    ESP-WIFI-MESH (2.4 GHz, WPA2)    │
    │    (самоорганизующаяся сеть)        │
    │    │         │          │          │
┌───▼──┐ ┌▼──────┐ ┌▼──────┐ ┌▼──────┐ ┌▼─────┐ ┌▼────────┐
│NODE  │ │NODE   │ │NODE   │ │NODE   │ │NODE  │ │NODE     │
│Displ │ │pH/EC  │ │pH/EC  │ │Climate│ │Relay │ │Water    │
│#1    │ │#1     │ │#2...N │ │#1...N │ │#1    │ │#1...N   │
│──────│ │───────│ │───────│ │───────│ │──────│ │─────────│
│S3 #2 │ │S3 #3  │ │S3 доп.│ │ESP32  │ │ESP32 │ │ESP32-C3 │
│──────│ │───────│ │───────│ │───────│ │──────│ │─────────│
│TFT   │ │pH 0x0A│ │pH 0x0A│ │SHT3x  │ │Форт.1│ │Насос 1  │
│240x  │ │EC 0x08│ │EC 0x08│ │CCS811 │ │Форт.2│ │Насос 2  │
│320   │ │OLED   │ │OLED   │ │Lux    │ │Вент. │ │Клапан 1 │
│LVGL  │ │128x64 │ │128x64 │ │───────│ │LED   │ │Клапан 2 │
│Энкод.│ │───────│ │───────│ │ТОЛЬКО │ │PWM   │ │Клапан 3 │
│──────│ │5 насос│ │5 насос│ │датчики│ │──────│ │─────────│
│Показ │ │PID AI │ │PID AI │ │       │ │Управ.│ │Управ.   │
│ВСЕХ  │ │АВТОН. │ │АВТОН. │ │       │ │климат│ │водой    │
│узлов │ │NVS cfg│ │NVS cfg│ │       │ │      │ │по зонам │
└──────┘ └───────┘ └───────┘ └───────┘ └──────┘ └─────────┘

МАСШТАБИРУЕМОСТЬ:
• pH/EC узлов: 1...N (каждый резервуар/зона)
• Climate узлов: 1...N (разные помещения)
• Water узлов: 1...N (разные системы полива)
• Display узлов: 1...N (несколько мониторов)
• Relay узлов: 1...N (разные зоны климата)
```

---

## 🔧 ИМЕЮЩИЕСЯ РЕСУРСЫ

### ESP32-S3 (3 штуки):
| # | Назначение | Обоснование |
|---|------------|-------------|
| **#1** | ROOT | Координатор mesh + MQTT, максимальная надежность |
| **#2** | NODE Display | LVGL требует PSRAM 8MB |
| **#3** | NODE pH/EC #1 | Критичный узел, автономная работа, PID |

### Докупить минимум:
- 1× ESP32 обычный (~$3-4) → NODE Climate
- 1× ESP32 обычный (~$3-4) → NODE Relay
- 1× ESP32-C3 (~$2-3) → NODE Water
- 2× Линейные актуаторы 12V для форточек (~$10-15 каждый)
- 1× OLED SSD1306 128x64 для pH/EC (~$3-5)
- 1× LED RGB + Buzzer + Кнопка (~$2-3)

**Итого:** ~$35-50 для базовой системы

---

## 🏗️ КОНФИГУРАЦИЯ УЗЛОВ

### ROOT NODE (ESP32-S3 #1) - Координатор

**Оборудование:**
- Только ESP32-S3, никаких датчиков/дисплея!

**Задачи:**
1. Координация ESP-WIFI-MESH (ROOT role)
2. WiFi STA подключение к домашнему роутеру
3. MQTT клиент (публикация телеметрии, подписка на команды)
4. Реестр всех узлов (node_registry)
5. Маршрутизация данных: NODE → MQTT и MQTT → NODE
6. Резервная логика климата (если Climate node offline)
7. Координация OTA обновлений

**Память (комфортно!):**
- Heap занято: ~155 KB
- Heap свободно: ~245 KB ✅
- Нет LVGL, нет датчиков → очень легкий!

**GPIO:** Не используются (только WiFi/Mesh)

---

### NODE Display (ESP32-S3 #2) - Интерфейс

**Оборудование:**
- TFT ILI9341 240x320 (SPI)
- Ротационный энкодер с кнопкой

**Задачи:**
1. Отображение данных ВСЕХ узлов на TFT
2. Навигация через энкодер
3. Отправка команд узлам через ROOT
4. Показ графиков (опционально)

**Память:**
- LVGL буферы → PSRAM 8MB
- Heap занято: ~140 KB
- Достаточно для ESP32-S3 ✅

**GPIO:**
- LCD: 9, 10, 11, 12, 14, 15
- Encoder: 4, 5, 6

**Экраны UI:**
1. Dashboard всех узлов (карточки)
2. Детали узла (полная информация)
3. Список узлов (прокрутка)
4. Системное меню
5. Mesh топология (опционально)

---

### NODE pH/EC (ESP32-S3 #3+) - КРИТИЧНЫЙ! ⚠️

**Оборудование:**
- Trema pH (I2C 0x0A)
- Trema EC (I2C 0x08)
- OLED SSD1306 128x64 (I2C 0x3C)
- 5 перистальтических насосов (GPIO 1,2,3,4,5)
- LED RGB индикатор (GPIO 15)
- Buzzer пищалка (GPIO 16)
- Кнопка MODE (GPIO 19)

**Задачи:**
1. Чтение pH/EC каждую секунду
2. Адаптивный PID контроль (с самообучением)
3. Отправка телеметрии на ROOT
4. **АВТОНОМНАЯ РАБОТА при потере связи!**
5. Локальное хранение данных (ring buffer 1000 записей)
6. Локальные алерты (OLED + LED + Buzzer)
7. Ручное управление (кнопка MODE)

**Память:**
- Adaptive PID + датчики + насосы: ~120 KB
- Локальный буфер: ~60 KB
- PSRAM для истории (опционально)
- Достаточно для S3 ✅

**GPIO:**
- I2C: 17 (SCL), 18 (SDA)
- Насосы: 1, 2, 3, 4, 5
- Индикация: 15 (LED), 16 (Buzzer), 19 (Button)

**OLED дисплей показывает:**
```
┌──────────────────┐
│ pH/EC Zone 1     │  ← ID зоны
│ pH: 6.5  EC: 1.8 │  ← Показания
│ ● ONLINE  AUTO   │  ← Статус + режим
│ ↑12ml ↓5ml →45ml │  ← Насосы (час)
└──────────────────┘
```

**Режимы работы:**
- **ONLINE** - связь с ROOT есть (LED зеленый)
- **DEGRADED** - связь нестабильная (LED желтый)
- **AUTONOMOUS** - автономный режим (LED желтый мигает)
- **EMERGENCY** - критичная ситуация (LED красный + buzzer)

**Автономная работа:**
- Timeout без связи: 30 секунд
- При потере связи: загружает конфигурацию из NVS
- PID продолжает работу по локальным настройкам
- Данные буферизуются (до 1000 записей)
- При восстановлении: синхронизация буфера с ROOT

**Кнопка MODE:**
- 1 нажатие (< 1с): AUTO ↔ MANUAL
- 2 нажатия: Сброс buzzer
- Долгое (> 3с): Сброс emergency режима
- Очень долгое (> 10с): Factory reset

**NVS хранит:**
- Целевые значения (ph_target, ec_target)
- Рабочие диапазоны (min/max)
- Аварийные пороги
- PID коэффициенты для каждого насоса
- Калибровочные данные
- Статистика работы

---

### NODE Climate (ESP32) - Датчики климата

**Оборудование:**
- SHT3x (I2C 0x44) - температура, влажность
- CCS811 (I2C 0x5A) - CO2
- Trema Lux (I2C 0x12) - освещенность

**Задачи:**
1. Чтение датчиков каждые 5 секунд
2. Отправка данных на ROOT
3. **ТОЛЬКО датчики** (решения принимает ROOT!)

**Память:**
- Heap занято: ~80 KB
- ESP32 хватает ✅

**GPIO:**
- I2C: 17 (SCL), 18 (SDA)

**Логика климата на ROOT:**
```c
// ROOT принимает решения!
if (co2 > 800 || temp > 28 || humidity > 75) {
    send_command("relay_001", "start_fan");
    send_command("relay_001", "open_windows");
}
```

---

### NODE Relay (ESP32) - Управление климатом

**Оборудование:**
- 2× Линейные актуаторы 12V (форточки)
- 1× Вентилятор (реле 220V или 12V)
- 1× LED свет (PWM диммирование)

**Задачи:**
1. Управление форточками (открыть/закрыть)
2. Управление вентилятором (вкл/выкл)
3. Управление светом (PWM 0-100%)
4. Управление по расписанию + датчику lux

**GPIO:**
- GPIO 1: Актуатор форточка 1 (реле 12V)
- GPIO 2: Актуатор форточка 2 (реле 12V)
- GPIO 3: Вентилятор (реле)
- GPIO 7: LED свет (PWM канал)

**Линейные актуаторы:**
- Тип: 12V DC, ход 50-100мм
- Управление: GPIO HIGH = открыто, LOW = закрыто
- Время полного хода: 10-30 секунд
- Концевики: встроенные

**PWM свет:**
- Частота: 5000 Hz
- Разрешение: 8 бит (0-255)
- Яркость: 0-100%
- Управление: таймер + lux датчик

**Логика освещения (комбинированная):**
```c
// Таймер: включить в 08:00, выключить в 22:00
if (hour >= 8 && hour < 22) {
    // Проверка освещенности
    if (lux < 500) {
        light_set_brightness(80);  // 80% если темно
    } else {
        light_set_brightness(30);  // 30% если светло
    }
} else {
    light_set_brightness(0);  // Выкл ночью
}
```

**Логика вентиляции (комбинированная):**
```c
bool should_ventilate = false;

if (co2 > 800) should_ventilate = true;
if (temp > 28.0) should_ventilate = true;
if (humidity > 75.0) should_ventilate = true;

if (should_ventilate) {
    fan_on();
    windows_open();
} else {
    fan_off();
    windows_close();
}
```

---

### NODE Water (ESP32-C3) - Управление водой

**Оборудование:**
- 2× Насосы воды (подача/слив)
- 3× Соленоидные клапаны (зоны)

**Задачи:**
1. Подача воды по командам
2. Слив воды
3. Распределение по зонам (клапаны)
4. Контроль уровня (опционально)

**GPIO:**
- GPIO 1: Насос подачи
- GPIO 2: Насос слива
- GPIO 3,4,5: Клапаны зон 1,2,3
- GPIO 6: Датчик уровня (опц.)

---

## 📡 ПРОТОКОЛ MESH ОБМЕНА (JSON)

### 1. Телеметрия (NODE → ROOT)

**pH/EC узел:**
```json
{
  "type": "telemetry",
  "node_id": "ph_ec_001",
  "timestamp": 1697548800,
  "data": {
    "ph": 6.5,
    "ec": 1.8,
    "pumps": {
      "ph_up": {"status": "idle", "total_ml": 1250, "daily_ml": 125},
      "ph_down": {"status": "idle", "total_ml": 450, "daily_ml": 45},
      "ec_a": {"status": "running", "ms_left": 3000, "total_ml": 2300},
      "ec_b": {"status": "idle", "total_ml": 2300},
      "ec_c": {"status": "idle", "total_ml": 1150}
    },
    "pid": {
      "auto": true,
      "ph_tgt": 6.8,
      "ec_tgt": 2.0
    },
    "conn": "online"
  }
}
```

**Climate узел:**
```json
{
  "type": "telemetry",
  "node_id": "climate_001",
  "timestamp": 1697548801,
  "data": {
    "temp": 24.5,
    "hum": 65.0,
    "co2": 450,
    "lux": 520
  }
}
```

### 2. Конфигурация (ROOT → NODE)

```json
{
  "type": "config",
  "node_id": "ph_ec_001",
  "timestamp": 1697548802,
  "config": {
    "ph_target": 6.8,
    "ec_target": 2.0,
    "ph_min": 6.0,
    "ph_max": 7.5,
    "ec_min": 1.2,
    "ec_max": 2.5,
    "pid": {
      "ph_up": {"kp": 1.0, "ki": 0.5, "kd": 0.1},
      "ph_down": {"kp": 1.0, "ki": 0.5, "kd": 0.1}
    },
    "auto_mode": true
  }
}
```

### 3. Команда (ROOT → NODE или Display → ROOT → NODE)

```json
{
  "type": "command",
  "node_id": "relay_001",
  "timestamp": 1697548803,
  "command": "set_light_brightness",
  "params": {
    "brightness": 75
  }
}
```

**Список команд:**
- `set_target` - изменить целевое значение
- `set_mode` - AUTO/MANUAL
- `run_pump` - запустить насос (pump, duration_ms)
- `calibrate` - калибровка датчика
- `reset_pid` - сброс PID
- `emergency_stop` - остановка всех исполнителей
- `restart` - перезагрузка узла
- `ota_update` - начать OTA (url, md5)
- `set_light_brightness` - яркость света (0-100)
- `open_windows` - открыть форточки
- `close_windows` - закрыть форточки
- `start_fan` - включить вентилятор
- `stop_fan` - выключить вентилятор

### 4. Событие (NODE → ROOT)

```json
{
  "type": "event",
  "node_id": "ph_ec_001",
  "timestamp": 1697548804,
  "level": "critical",
  "message": "pH критично низкий: 4.8",
  "data": {
    "ph": 4.8,
    "action": "emergency_correction"
  }
}
```

**Уровни:**
- `info` - информация
- `warning` - предупреждение
- `critical` - критично (→ SMS!)
- `emergency` - авария (→ SMS!)

### 5. Heartbeat (NODE → ROOT каждые 10 сек)

```json
{
  "type": "heartbeat",
  "node_id": "ph_ec_001",
  "timestamp": 1697548805,
  "uptime": 36000,
  "heap": 145000,
  "conn": "online"
}
```

---

## 💾 NVS КОНФИГУРАЦИЯ (ДЛЯ ВСЕХ УЗЛОВ!)

### Общий компонент: common/node_config/

**Что хранится:**

**NODE pH/EC:**
```c
typedef struct {
    // Базовая информация
    char node_id[32];              // "ph_ec_001"
    char zone[32];                 // "Zone 1"
    
    // Целевые значения
    float ph_target;               // 6.8
    float ec_target;               // 2.0
    
    // Рабочие диапазоны
    float ph_min, ph_max;          // 6.0 - 7.5
    float ec_min, ec_max;          // 1.2 - 2.5
    
    // Аварийные пороги
    float ph_emerg_low;            // 5.0
    float ph_emerg_high;           // 8.0
    float ec_emerg_high;           // 3.0
    
    // PID параметры (5 насосов)
    struct {
        float kp, ki, kd;
        bool enabled;
    } pump_pid[5];
    
    // Safety
    uint32_t max_pump_time_ms;     // 30000
    uint32_t cooldown_ms;          // 60000
    uint32_t max_daily_ml;         // 500
    
    // Автономный режим
    bool autonomous_enabled;       // true
    uint32_t mesh_timeout_ms;      // 30000
    
    // Калибровка
    float ph_cal_offset;
    float ec_cal_offset;
    
    // Мета-данные
    uint32_t config_version;
    uint64_t last_updated;
} ph_ec_node_config_t;
```

**NODE Relay:**
```c
typedef struct {
    char node_id[32];
    
    // Свет
    struct {
        uint8_t brightness;        // 0-100%
        char on_time[6];           // "08:00"
        char off_time[6];          // "22:00"
        bool lux_control;          // true
        uint16_t lux_threshold;    // 500
    } light;
    
    // Вентиляция (пороги)
    uint16_t co2_threshold;        // 800 ppm
    float temp_threshold;          // 28.0°C
    float humidity_threshold;      // 75.0%
    
    // Форточки
    uint32_t window_open_time_ms;  // Время открытия
    uint32_t window_close_time_ms; // Время закрытия
} relay_node_config_t;
```

**API:**
```c
esp_err_t node_config_init(void);
esp_err_t node_config_load(void *config, size_t size, const char *namespace);
esp_err_t node_config_save(const void *config, size_t size, const char *namespace);
esp_err_t node_config_reset_default(void *config, const char *node_type);

// Обработка обновления конфигурации от ROOT
void node_config_handle_update(cJSON *new_config);
```

---

## 🌐 MQTT ТОПИКИ

### Публикация (ROOT → Server):

```
hydro/telemetry/{node_id}      # Телеметрия узла (QoS 1)
hydro/event/{node_id}          # События узла (QoS 1)
hydro/heartbeat/{node_id}      # Heartbeat (QoS 0)
hydro/status/root              # Статус ROOT (QoS 1)
hydro/nodes/list               # Список узлов (раз в минуту, QoS 1)
hydro/topology                 # Топология mesh (QoS 0)
```

### Подписка (Server → ROOT):

```
hydro/command/{node_id}        # Команда для узла (QoS 1)
hydro/config/{node_id}         # Конфигурация узла (QoS 1)
hydro/control/root             # Управление ROOT (QoS 1)
hydro/broadcast                # Broadcast всем (QoS 1)
hydro/ota/start/{node_id}      # Начало OTA (QoS 1)
```

---

## 🗄️ БАЗА ДАННЫХ (PostgreSQL)

### Схема БД:

```sql
-- Пользователи (Laravel Sanctum)
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255),
    email VARCHAR(255) UNIQUE,
    password VARCHAR(255),
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

-- Узлы
CREATE TABLE nodes (
    id SERIAL PRIMARY KEY,
    node_id VARCHAR(32) UNIQUE,
    mac VARCHAR(17),
    type VARCHAR(16),  -- ph_ec, climate, water, relay, display
    name VARCHAR(64),
    zone VARCHAR(32),  -- Зона/помещение
    online BOOLEAN DEFAULT false,
    last_seen TIMESTAMP,
    config JSONB,
    last_telemetry JSONB,
    firmware_version VARCHAR(16),
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

-- Телеметрия (ВСЯ ИСТОРИЯ!)
CREATE TABLE telemetry (
    id BIGSERIAL PRIMARY KEY,
    node_id VARCHAR(32),
    timestamp TIMESTAMP,
    data JSONB,
    created_at TIMESTAMP
);
CREATE INDEX idx_telemetry_node_time ON telemetry(node_id, timestamp DESC);

-- События
CREATE TABLE events (
    id SERIAL PRIMARY KEY,
    node_id VARCHAR(32),
    level VARCHAR(16),  -- info, warning, critical, emergency
    message TEXT,
    data JSONB,
    timestamp TIMESTAMP,
    acknowledged BOOLEAN DEFAULT false,
    created_at TIMESTAMP
);

-- Команды (история)
CREATE TABLE commands (
    id SERIAL PRIMARY KEY,
    node_id VARCHAR(32),
    command VARCHAR(64),
    params JSONB,
    result VARCHAR(16),
    sent_at TIMESTAMP,
    executed_at TIMESTAMP
);

-- OTA обновления
CREATE TABLE ota_updates (
    id SERIAL PRIMARY KEY,
    firmware_file VARCHAR(255),
    version VARCHAR(16),
    target_node_type VARCHAR(16),
    description TEXT,
    file_size BIGINT,
    md5_hash VARCHAR(32),
    created_at TIMESTAMP
);

CREATE TABLE ota_node_status (
    id SERIAL PRIMARY KEY,
    node_id VARCHAR(32),
    update_id INTEGER,
    status VARCHAR(16),  -- pending, updating, success, failed
    progress INTEGER,    -- 0-100%
    error_message TEXT,
    started_at TIMESTAMP,
    completed_at TIMESTAMP
);
```

---

## 📱 УВЕДОМЛЕНИЯ

### SMS (только критичные!):

**Когда отправлять:**
- pH < 5.0 или > 8.0
- EC > 3.0
- NODE pH/EC offline > 5 минут
- ROOT offline > 2 минуты
- Emergency режим активирован

**SMS Gateway:** Twilio или SMS.ru

**Пример SMS:**
```
⚠ HYDRO ALERT
pH крит.: 4.8
Зона: 1
17.10 14:35
Авто коррекция
```

### Telegram (уведомления + статус):

**Уведомления:**
- Все события (info, warning, critical, emergency)
- Смена режимов узлов
- Результаты команд

**Команды:**
- `/status` - общий статус
- `/ph` - pH всех зон
- `/ec` - EC всех зон
- `/climate` - климатические данные
- `/nodes` - список узлов
- `/alerts` - текущие тревоги

---

## 🔄 OTA ОБНОВЛЕНИЯ

### Процесс (Hybrid):

**ROOT узел:**
1. Скачивает прошивку с сервера по HTTP
2. Проверяет MD5
3. Устанавливает
4. Перезагружается

**Остальные узлы:**
1. ROOT получает команду OTA от сервера
2. ROOT раздает прошивку через mesh (chunks 1KB)
3. NODE скачивает, проверяет MD5
4. NODE устанавливает, перезагружается
5. NODE отправляет новую версию на ROOT
6. ROOT обновляет статус на сервере

### Безопасность OTA:
- ✅ MD5 проверка
- ✅ Dual partition (rollback при ошибке)
- ✅ Поэтапное обновление (сначала тест на одном)
- ❌ Не обновлять все одновременно

---

## 📊 ВЕТКУРАЦИЯ ПОТОКОВ ДАННЫХ

### Телеметрия (NODE → Сервер):

```
NODE pH/EC #1
  ↓ mesh packet (JSON ~500 bytes)
ROOT NODE
  ↓ MQTT publish: hydro/telemetry/ph_ec_001
Mosquitto Broker (1883)
  ↓ MQTT subscribe
Laravel MqttService (php artisan mqtt:listen)
  ↓ DB Insert
PostgreSQL (table: telemetry)
  ↓ Laravel Broadcasting
WebSocket (port 6001)
  ↓ WebSocket client
Vue.js Dashboard
  ↓ Update UI (real-time!)
```

**Частота отправки:**
- pH/EC: каждые 2 секунды
- Climate: каждые 5 секунд
- Water: каждые 10 секунд
- Heartbeat: каждые 10 секунд

### Команда (Веб-интерфейс → NODE):

```
Vue.js (кнопка "Set pH 6.5")
  ↓ HTTP POST /api/nodes/ph_ec_001/command
Laravel API (NodeController)
  ↓ MqttService->sendCommand()
MQTT publish: hydro/command/ph_ec_001
  ↓ MQTT subscribe
ROOT NODE
  ↓ mesh_send_to_node(mac, json)
NODE pH/EC #1
  ↓ node_controller_handle_command()
Применение: ph_target = 6.5
  ↓ NVS save
  ↓ Response
ROOT NODE
  ↓ MQTT publish: hydro/response/ph_ec_001
Сервер → WebSocket → Vue.js (toast "✅ Команда выполнена")
```

---

## 🔧 КОМПОНЕНТЫ ДЛЯ РЕАЛИЗАЦИИ

### Общие (common/):

1. **mesh_manager** - управление ESP-WIFI-MESH
2. **mesh_protocol** - парсинг/создание JSON
3. **node_config** - сохранение/загрузка настроек в NVS
4. **sensor_base** - базовые функции датчиков (retry, validation)
5. **actuator_base** - базовые функции исполнителей (safety, stats)
6. **ota_manager** - OTA обновления

### ROOT (root_node/components/):

1. **node_registry** - реестр всех узлов
2. **mqtt_client** - MQTT клиент
3. **data_router** - маршрутизация данных
4. **climate_logic** - резервная логика климата
5. **ota_coordinator** - координация OTA

### NODE pH/EC (node_ph_ec/components/):

1. **oled_display** - OLED SSD1306 дисплей
2. **connection_monitor** - мониторинг связи
3. **local_storage** - локальный буфер данных
4. **buzzer_led** - индикация + кнопка
5. **node_controller** - логика узла
6. Портированные из hydro1.0:
   - sensor_manager (pH, EC)
   - pump_manager (5 насосов)
   - adaptive_pid (AI PID)

### NODE Display (node_display/components/):

1. **display_controller** - запрос данных от ROOT
2. **ui_screens** - экраны LVGL
3. Портированные из hydro1.0:
   - lvgl_ui
   - lcd_ili9341
   - encoder

### Сервер (server/):

**Backend (Laravel):**
- Models: Node, Telemetry, Event, Command, OtaUpdate
- Controllers: Node, Telemetry, Command, Dashboard, Ota
- Services: MqttService, TelegramService, SmsService
- Commands: mqtt:listen, telegram:bot, data:aggregate

**Frontend (Vue.js):**
- Dashboard.vue - главный дашборд (Material Design)
- NodeCard.vue - карточка узла
- NodeDetail.vue - детали узла
- LineChart.vue - графики (Chart.js)
- LightDimmer.vue - PWM слайдер
- OtaManager.vue - управление OTA

---

## ⏱️ ОЦЕНКА ВРЕМЕНИ РАЗРАБОТКИ

| Фаза | Дни | Компоненты |
|------|-----|------------|
| 1. Базовая mesh-сеть | 3-4 | mesh_manager, mesh_protocol, node_config |
| 2. ROOT узел | 3-4 | node_registry, mqtt_client, data_router |
| 3. Сервер базовый | 3-4 | Laravel, PostgreSQL, MqttService |
| 4. NODE pH/EC (критичный!) | 4-5 | Все компоненты + автономия |
| 5. Vue.js Dashboard | 3-4 | Material Design интерфейс |
| 6. NODE Display | 3-4 | LVGL экраны |
| 7. Climate + Relay | 3-4 | Датчики + актуаторы |
| 8. Water | 2-3 | Насосы + клапаны |
| 9. Telegram + SMS | 2-3 | Боты и уведомления |
| 10. OTA | 3-4 | Система обновлений |
| 11. Auth | 1-2 | Laravel Sanctum |
| 12. Backup | 1-2 | Export/Import + auto backup |
| 13. Документация | 2-3 | Все MD файлы |
| 14. Тестирование | 3-5 | Полное тестирование |
| **ИТОГО:** | **36-51 день** | |

**Реалистично:** 40-45 дней (1.5-2 месяца)

---

## 🎯 ПРИОРИТЕТЫ РАЗРАБОТКИ

### Этап 1: Базовая функциональность (2-3 недели)
1. Mesh-сеть (ROOT + 1 тестовый NODE)
2. MQTT интеграция
3. Базовый сервер (Laravel + БД)
4. NODE pH/EC с автономией ⚠️ КРИТИЧНО!

### Этап 2: Полная система (3-4 недели)
5. Vue.js Dashboard
6. NODE Display (TFT)
7. NODE Climate + Relay
8. NODE Water
9. Telegram бот

### Этап 3: Продвинутые функции (2-3 недели)
10. OTA система
11. SMS уведомления
12. Аутентификация
13. Backup/Restore
14. Документация

---

## 📝 СЛЕДУЮЩИЕ ШАГИ

1. ✅ Структура проекта создана
2. ⏭️ Реализовать common/mesh_manager
3. ⏭️ Реализовать common/mesh_protocol
4. ⏭️ Реализовать common/node_config
5. ⏭️ Протестировать базовую mesh-сеть

---

**Проект спланирован и готов к реализации!** 🚀

**См. также:**
- [MESH_HYDRO_V2_FINAL_PLAN.md](MESH_HYDRO_V2_FINAL_PLAN.md) - полный детальный план
- Текущий hydro1.0 - для примеров компонентов

