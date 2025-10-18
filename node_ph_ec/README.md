# NODE pH/EC

ESP32-S3 - Критичный узел с автономной работой

## 🎯 Назначение

**NODE pH/EC** - **самый критичный узел системы** с полной автономной работой.

### ⚠️ Ключевые функции:
- 📊 Измерение pH и EC (датчики Trema I2C)
- 💧 Управление 5 насосами (pH UP/DOWN, EC A/B/C)
- 🤖 Адаптивный PID контроллер
- 📺 OLED дисплей SSD1306 128x64
- 🔋 **АВТОНОМНАЯ РАБОТА** при потере связи с ROOT
- 💾 Локальный ring buffer (1000 записей)
- 🔴 LED + Buzzer индикация
- 🔘 Кнопка MODE (ручное управление)

### 🚨 КРИТИЧНЫЕ ПРАВИЛА:

1. ✅ **ВСЕГДА автономная работа**
   - Если ROOT offline > 30 сек → автономный режим
   - PID контроллер НИКОГДА не останавливается

2. ✅ **ВСЕГДА сохранение в NVS**
   - Все настройки (pH target, EC target, PID параметры)
   - Конфигурация выживет при перезагрузке

3. ✅ **ВСЕГДА локальный buffer**
   - Данные не теряются при offline
   - Синхронизация при восстановлении связи

4. ✅ **ВСЕГДА watchdog**
   - Автоматическая перезагрузка при зависании

## 📦 Компоненты

### Из common/:
- `mesh_manager` - Mesh NODE режим
- `mesh_protocol` - JSON протокол
- `node_config` - NVS конфигурация

### Собственные (components/):
- ✅ `node_controller` - главная логика узла
- ✅ `connection_monitor` - мониторинг связи с ROOT
- ✅ `local_storage` - ring buffer для offline данных
- ✅ `oled_display` - SSD1306 128x64 дисплей
- ✅ `buzzer_led` - LED + Buzzer + Button
- 🔄 `sensor_manager` - **TODO: портировать из hydro1.0**
- 🔄 `pump_manager` - **TODO: портировать из hydro1.0**
- 🔄 `adaptive_pid` - **TODO: портировать из hydro1.0**

## 🔌 Распиновка

| GPIO | Назначение | Примечание |
|------|------------|------------|
| **I2C шина:** | | |
| 17 | SCL | pH, EC, OLED |
| 18 | SDA | pH, EC, OLED |
| - | pH (0x0A) | Trema датчик |
| - | EC (0x08) | Trema датчик |
| - | OLED (0x3C) | SSD1306 128x64 |
| **Насосы:** | | |
| 1 | pH UP | HIGH = вкл |
| 2 | pH DOWN | HIGH = вкл |
| 3 | EC A | HIGH = вкл |
| 4 | EC B | HIGH = вкл |
| 5 | EC C | HIGH = вкл |
| **Индикация:** | | |
| 15 | LED статус | Зеленый/желтый/красный |
| 16 | Buzzer | Пищалка 3.3V |
| 19 | Кнопка MODE | Pull-up (LOW=нажата) |

## 🚀 Сборка и прошивка

```bash
cd node_ph_ec
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
```

## 📊 Ожидаемый вывод

```
I PH_EC: ========================================
I PH_EC: === NODE pH/EC Starting ===
I PH_EC: ========================================
I PH_EC: [Step 1/11] Initializing NVS...
I PH_EC: [Step 2/11] Loading configuration...
I PH_EC: Loaded: ph_ec_001 (Zone 1)
I PH_EC: Targets: pH=6.80, EC=2.00
I PH_EC: [Step 3/11] Initializing LED/Buzzer/Button...
I buzzer_led: Buzzer/LED/Button initialized
I PH_EC: [Step 4/11] Initializing OLED Display...
I oled_display: OLED Display initialized
I PH_EC: [Step 5/11] Initializing Sensors...
I sensor_manager: Sensor Manager initialized (STUB)
I PH_EC: [Step 6/11] Initializing Pumps...
I pump_manager: Pump Manager initialized (STUB)
I PH_EC: [Step 7/11] Initializing Adaptive PID...
I adaptive_pid: Adaptive PID initialized (STUB)
I PH_EC: [Step 8/11] Initializing Local Storage...
I local_storage: Local Storage initialized (buffer size: 1000)
I PH_EC: [Step 9/11] Initializing Connection Monitor...
I conn_monitor: Connection Monitor initialized
I PH_EC: [Step 10/11] Initializing Mesh (NODE mode)...
I mesh_manager: NODE mode
I mesh_manager: Mesh started
I mesh_manager: Parent connected
I PH_EC: [Step 11/11] Starting Node Controller...
I node_controller: Node Controller initialized
I node_controller: Node ID: ph_ec_001, Zone: Zone 1
I node_controller: Main task running (cycle: 10 seconds)
I PH_EC: ========================================
I PH_EC: === NODE pH/EC Running ===
I PH_EC: Node ID: ph_ec_001
I PH_EC: Autonomous: ENABLED
I PH_EC: ========================================
```

## 🧪 Тестирование

### Тест 1: Автономный режим

1. Прошей узел (ROOT не нужен)
2. Подожди 30 секунд

**Ожидается:**
- ⚡ LED желтый мигающий
- 🔊 2 сигнала buzzer
- 📺 OLED: "AUTONOMOUS"
- ✅ PID продолжает работу
- ✅ Данные буферизуются в local_storage

### Тест 2: Подключение к ROOT

1. Прошей и запусти ROOT узел
2. Прошей NODE pH/EC

**Ожидается:**
```
I mesh_manager: Parent connected
I conn_monitor: State changed: AUTONOMOUS → ONLINE
I node_controller: Exiting autonomous mode
I PH_EC: ● LED зеленый
I PH_EC: Telemetry sent to ROOT
```

### Тест 3: Кнопка MODE

- Короткое нажатие (<1 сек) - переключение AUTO/MANUAL
- Среднее (1-3 сек) - сброс Buzzer
- Долгое (3-10 сек) - сброс Emergency
- Очень долгое (>10 сек) - Factory Reset

## ✅ Статус реализации

| Компонент | Статус | Описание |
|-----------|--------|----------|
| node_controller | ✅ ГОТОВ | Главная логика с автономией |
| connection_monitor | ✅ ГОТОВ | 4 состояния (ONLINE/DEGRADED/AUTONOMOUS/EMERGENCY) |
| local_storage | ✅ ГОТОВ | Ring buffer 1000 записей |
| oled_display | ✅ ГОТОВ | SSD1306 (заглушка, нужен драйвер) |
| buzzer_led | ✅ ГОТОВ | LED + Buzzer + Button |
| sensor_manager | 🔄 ЗАГЛУШКА | TODO: портировать из hydro1.0 |
| pump_manager | 🔄 ЗАГЛУШКА | TODO: портировать из hydro1.0 |
| adaptive_pid | 🔄 ЗАГЛУШКА | TODO: портировать из hydro1.0 |
| app_main.c | ✅ ГОТОВ | 11-шаговая инициализация |
| CMakeLists.txt | ✅ ГОТОВ | Все зависимости |
| sdkconfig | ✅ ГОТОВ | ESP32-S3, watchdog, OTA |

**Базовая структура: 100%** ✅

**Портирование компонентов: 0%** 🔄 (требуется доступ к hydro1.0)

## 📚 Документация

- `AI_INSTRUCTIONS.md` - полная инструкция (490+ строк)
- `components/*/` - документация компонентов
- `../common/AI_INSTRUCTIONS.md` - общие компоненты
- `../doc/MESH_HYDRO_V2_FINAL_PLAN.md` - общий план

## 🔧 Следующие шаги

### Для полной реализации нужно:

1. **Портировать из hydro1.0:**
   - `sensor_manager` (чтение Trema pH/EC датчиков)
   - `pump_manager` (управление 5 насосами с safety)
   - `adaptive_pid` (AI PID контроллер)

2. **Добавить SSD1306 драйвер:**
   - Можно взять из ESP-IDF component registry
   - Или портировать из hydro1.0

3. **Тестирование:**
   - С реальными датчиками
   - Автономный режим (24 часа)
   - Emergency ситуации
   - Восстановление связи

## 📝 Примечания

**Текущее состояние:** Проект компилируется, но датчики и насосы возвращают заглушки. PID контроллер не работает.

**Для production:** Нужно портировать 3 компонента из hydro1.0.

**Приоритет:** 🔴 МАКСИМАЛЬНЫЙ

**Время реализации:** 4-5 дней (с портированием)
