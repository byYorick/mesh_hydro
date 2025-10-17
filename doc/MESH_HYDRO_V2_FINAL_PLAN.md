# 🌐 ФИНАЛЬНЫЙ ПЛАН: Mesh Hydro System V2

**Дата:** 17 октября 2025  
**Автор:** AI Assistant для Hydro Team  
**Версия плана:** 2.0 Final

---

## 🎯 КРАТКИЙ ОБЗОР

**Что создаем:**
Распределенная mesh-система гидропонного мониторинга с:
- 1× ROOT узел (ESP32-S3) - координатор mesh + MQTT
- 1× NODE Display (ESP32-S3) - TFT экран + LVGL
- N× NODE pH/EC (ESP32-S3) - критичные узлы с автономией
- N× NODE Climate (ESP32) - датчики климата
- 1× NODE Relay (ESP32) - управление форточками и вентиляцией  
- N× NODE Water (ESP32-C3) - управление водой
- 1× Веб-сервер (Laravel + Vue.js + PostgreSQL + Mosquitto)

**Ключевые решения:**
- ✅ ROOT на ESP32-S3 #1 (надежность)
- ✅ Display на ESP32-S3 #2 (LVGL требует PSRAM)
- ✅ pH/EC на ESP32-S3 #3+ (автономная работа)
- ✅ Линейные актуаторы для форточек (реле вкл/выкл)
- ✅ Логика климата на ROOT (если Climate node недоступен)
- ✅ OLED SSD1306 128x64 на каждом pH/EC узле
- ✅ Независимые зоны pH/EC (свои настройки)
- ✅ Telegram бот (уведомления + статус)
- ✅ SMS уведомления (критичные события)
- ✅ PWM диммирование света
- ✅ Комбинированная вентиляция (CO2 + температура + влажность)

---

## 📐 АРХИТЕКТУРА СИСТЕМЫ

### Топология сети

```
┌──────────────────────────────────────────────────────────────┐
│                  СЕРВЕР (Linux/Windows)                      │
│  • Laravel 10 API + Vue.js 3 Dashboard                       │
│  • PostgreSQL 15 (вся история)                               │
│  • Mosquitto MQTT Broker                                     │
│  • Telegram Bot (уведомления + статус)                       │
│  • SMS Gateway (критичные события)                           │
│  • Nginx reverse proxy                                       │
└────────────────────┬─────────────────────────────────────────┘
                     │
              MQTT (WiFi TCP/IP)
                     │
┌────────────────────▼─────────────────────────────────────────┐
│         ROOT NODE (ESP32-S3 #1) ⭐ КООРДИНАТОР               │
│  • ESP-WIFI-MESH ROOT                                        │
│  • WiFi STA → роутер (для MQTT)                             │
│  • MQTT клиент (публикация/подписка)                        │
│  • Node Registry (учет всех узлов)                          │
│  • Data Router (NODE ↔ MQTT)                                │
│  • Логика климата (если Climate node offline)               │
│  • OTA координатор (раздача прошивок)                       │
│  БЕЗ дисплея! БЕЗ датчиков!                                 │
└────┬───────┬─────────┬─────────┬────────┬──────────────────┘
     │       │         │         │        │
     │   ESP-WIFI-MESH (2.4 GHz, WPA2)   │
     │       │         │         │        │
┌────▼───┐ ┌─▼─────┐ ┌─▼─────┐ ┌▼─────┐ ┌▼──────┐ ┌─▼──────┐
│ NODE   │ │ NODE  │ │ NODE  │ │NODE  │ │ NODE  │ │ NODE   │
│Display │ │pH/EC  │ │pH/EC  │ │Climat│ │ Relay │ │ Water  │
│   #1   │ │  #1   │ │#2...N │ │#1..N │ │  #1   │ │ #1...N │
│────────│ │───────│ │───────│ │──────│ │───────│ │────────│
│ESP32-S3│ │ESP32  │ │ESP32  │ │ESP32 │ │ESP32  │ │ESP32-C3│
│⭐ #2   │ │-S3 ⭐3│ │-S3    │ │обычн.│ │обычн. │ │дешевый │
│────────│ │───────│ │───────│ │──────│ │───────│ │────────│
│TFT     │ │pH I2C │ │pH I2C │ │SHT3x │ │Акт.1  │ │Насос 1 │
│240x320 │ │EC I2C │ │EC I2C │ │CCS811│ │Форт.1 │ │Насос 2 │
│LVGL    │ │OLED   │ │OLED   │ │Lux   │ │Акт.2  │ │Клапан1 │
│Энкодер │ │128x64 │ │128x64 │ │──────│ │Форт.2 │ │Клапан2 │
│────────│ │───────│ │───────│ │Данные│ │Вент.  │ │Клапан3 │
│Показ   │ │5 насос│ │5 насос│ │только│ │PWM LED│ │───────│
│всех    │ │PID AI │ │PID AI │ │      │ │───────│ │Подача/ │
│узлов   │ │АВТОНОМ│ │АВТОНОМ│ │      │ │Управл.│ │слив по │
│        │ │NVS cfg│ │NVS cfg│ │      │ │климат.│ │зонам   │
└────────┘ └───────┘ └───────┘ └──────┘ └───────┘ └────────┘
```

**Масштабирование:**
- pH/EC узлов может быть **неограниченное количество** (разные резервуары/зоны)
- Climate узлов тоже **несколько** (разные помещения)
- Water узлов **несколько** (разные системы полива)
- Display узлов можно добавить **несколько** (разные локации)

---

## 🔧 КОНФИГУРАЦИЯ УСТРОЙСТВ

### Имеющиеся ESP32-S3 (3 штуки):

| # | Назначение | Обоснование |
|---|------------|-------------|
| **#1** | ROOT | Координатор mesh + MQTT, максимальная надежность |
| **#2** | NODE Display | LVGL требует PSRAM 8MB |
| **#3** | NODE pH/EC #1 | Критичный узел, автономная работа |

### Дополнительно докупить:

| Устройство | Количество | Цена | Итого | Назначение |
|------------|------------|------|-------|------------|
| ESP32 обычный | 1+ | $3-4 | $3-4+ | NODE Climate (датчики) |
| ESP32 обычный | 1 | $3-4 | $3-4 | NODE Relay (форточки+вент) |
| ESP32-C3 | 1+ | $2-3 | $2-3+ | NODE Water (насосы воды) |
| **Опционально:** | | | | |
| ESP32-S3 | N | $8-10 | N×$10 | Дополнительные pH/EC узлы |

**Минимум для старта:** ~$8-11 дополнительно

---

## 📋 ДЕТАЛЬНАЯ СТРУКТУРА ПРОЕКТА

```
mesh_hydro_v2/
│
├── README.md                          # Общее описание
├── ARCHITECTURE.md                    # Архитектура системы
├── SETUP_GUIDE.md                     # Инструкция развертывания
├── AI_INSTRUCTIONS.md                 # Инструкция для AI
│
├── common/                            # Общие компоненты (симлинки во все узлы)
│   │
│   ├── mesh_manager/                  # ESP-WIFI-MESH управление
│   │   ├── mesh_manager.h            # API: init_root/node, send, receive
│   │   ├── mesh_manager.c            # Реализация для ROOT и NODE режимов
│   │   └── CMakeLists.txt
│   │
│   ├── mesh_protocol/                 # Протокол JSON обмена
│   │   ├── mesh_protocol.h           # API: parse, create_telemetry/command/event
│   │   ├── mesh_protocol.c           # JSON парсинг (cJSON)
│   │   └── CMakeLists.txt
│   │
│   ├── node_config/                   # NVS конфигурация ⭐
│   │   ├── node_config.h             # Сохранение/загрузка настроек
│   │   ├── node_config.c             # Работа с NVS для каждого типа узла
│   │   └── CMakeLists.txt
│   │
│   ├── ota_manager/                   # OTA обновления ⭐
│   │   ├── ota_manager.h             # API: check_update, start_update
│   │   ├── ota_manager.c             # OTA через mesh или HTTP
│   │   └── CMakeLists.txt
│   │
│   ├── sensor_base/                   # Базовые функции датчиков
│   │   ├── sensor_base.h             # Retry, validation, caching
│   │   ├── sensor_base.c
│   │   └── CMakeLists.txt
│   │
│   └── actuator_base/                 # Базовые функции исполнителей
│       ├── actuator_base.h           # Safety checks, stats
│       ├── actuator_base.c
│       └── CMakeLists.txt
│
├── root_node/                         # ROOT координатор
│   ├── main/
│   │   ├── app_main.c                # Точка входа ROOT
│   │   ├── root_config.h             # Конфигурация ROOT
│   │   └── CMakeLists.txt
│   │
│   ├── components/
│   │   ├── node_registry/            # Реестр всех узлов
│   │   │   ├── node_registry.h      # API: add, remove, update, get
│   │   │   ├── node_registry.c      # Хранение узлов в памяти + NVS
│   │   │   └── CMakeLists.txt
│   │   │
│   │   ├── mqtt_client/              # MQTT клиент
│   │   │   ├── mqtt_client.h        # API: connect, publish, subscribe
│   │   │   ├── mqtt_client.c        # esp-mqtt библиотека
│   │   │   └── CMakeLists.txt
│   │   │
│   │   ├── data_router/              # Маршрутизация NODE ↔ MQTT
│   │   │   ├── data_router.h        # API: route_node_data, route_mqtt_command
│   │   │   ├── data_router.c        # Логика маршрутизации
│   │   │   └── CMakeLists.txt
│   │   │
│   │   ├── climate_logic/            # Логика климата (резерв) ⭐
│   │   │   ├── climate_logic.h      # Принятие решений если Climate offline
│   │   │   ├── climate_logic.c      # Алгоритмы управления форточками/вент.
│   │   │   └── CMakeLists.txt
│   │   │
│   │   └── ota_coordinator/          # Координация OTA обновлений
│   │       ├── ota_coordinator.h    # Управление OTA для всех узлов
│   │       ├── ota_coordinator.c    # Раздача прошивок через mesh
│   │       └── CMakeLists.txt
│   │
│   ├── sdkconfig.defaults            # Конфигурация ESP-IDF
│   ├── partitions.csv                # Разделы flash (OTA поддержка)
│   └── CMakeLists.txt
│
├── node_display/                      # NODE с TFT дисплеем
│   ├── main/
│   │   ├── app_main.c
│   │   └── display_config.h
│   │
│   ├── components/
│   │   ├── lvgl_ui/                  # Из hydro1.0
│   │   ├── lcd_ili9341/              # Из hydro1.0
│   │   ├── encoder/                  # Из hydro1.0
│   │   │
│   │   ├── display_controller/       # Логика отображения
│   │   │   ├── display_controller.h # Запрос данных от ROOT
│   │   │   └── display_controller.c # Обновление UI
│   │   │
│   │   └── ui_screens/               # Экраны UI
│   │       ├── dashboard_screen.c    # Главный dashboard
│   │       ├── node_detail_screen.c  # Детали узла
│   │       ├── node_list_screen.c    # Список всех узлов
│   │       └── system_menu_screen.c  # Системное меню
│   │
│   └── CMakeLists.txt
│
├── node_ph_ec/                        # NODE pH/EC (критичный!) ⚠️
│   ├── main/
│   │   ├── app_main.c
│   │   └── ph_ec_config.h
│   │
│   ├── components/
│   │   ├── sensor_manager/           # Из hydro1.0 (pH, EC)
│   │   ├── pump_manager/             # Из hydro1.0 (5 насосов)
│   │   ├── adaptive_pid/             # Из hydro1.0 (AI PID)
│   │   │
│   │   ├── oled_display/             # OLED SSD1306 128x64 ⭐
│   │   │   ├── oled_display.h       # API: init, print_status
│   │   │   └── oled_display.c       # I2C 0x3C, показ pH/EC/статус
│   │   │
│   │   ├── node_controller/          # Логика узла
│   │   │   ├── node_controller.h    # Auto/manual режим, команды
│   │   │   └── node_controller.c    # Обработка команд от ROOT
│   │   │
│   │   ├── connection_monitor/       # Мониторинг связи ⭐
│   │   │   ├── connection_monitor.h # Online/Autonomous/Emergency
│   │   │   └── connection_monitor.c # Проверка таймаутов
│   │   │
│   │   ├── local_storage/            # Локальное хранилище ⭐
│   │   │   ├── local_storage.h      # Ring buffer 1000 записей
│   │   │   └── local_storage.c      # Буферизация при потере связи
│   │   │
│   │   └── buzzer_led/               # Индикация
│   │       ├── buzzer_led.h         # LED + Buzzer + Button
│   │       └── buzzer_led.c         # Локальные алерты
│   │
│   ├── sdkconfig.defaults
│   ├── partitions.csv
│   └── CMakeLists.txt
│
├── node_climate/                      # NODE климат (датчики)
│   ├── main/
│   │   ├── app_main.c
│   │   └── climate_config.h
│   │
│   ├── components/
│   │   ├── sensor_manager/           # SHT3x, CCS811, Lux
│   │   │
│   │   └── climate_controller/       # Логика климата
│   │       ├── climate_controller.h # Сбор данных, отправка на ROOT
│   │       └── climate_controller.c # ROOT принимает решения!
│   │
│   └── CMakeLists.txt
│
├── node_relay/                        # NODE управление климатом
│   ├── main/
│   │   ├── app_main.c
│   │   └── relay_config.h
│   │
│   ├── components/
│   │   ├── actuator_manager/         # Управление актуаторами
│   │   │   ├── actuator_manager.h
│   │   │   └── actuator_manager.c
│   │   │
│   │   ├── vent_actuators/           # Линейные актуаторы форточек ⭐
│   │   │   ├── vent_actuators.h     # API: open, close, stop
│   │   │   └── vent_actuators.c     # Реле управление (12V вкл/выкл)
│   │   │
│   │   ├── fan_controller/           # Вентиляция
│   │   │   ├── fan_controller.h     # Вкл/выкл по команде ROOT
│   │   │   └── fan_controller.c
│   │   │
│   │   └── light_controller/         # PWM диммирование ⭐
│   │       ├── light_controller.h   # API: set_brightness(0-100%)
│   │       └── light_controller.c   # PWM управление яркостью
│   │
│   └── CMakeLists.txt
│
├── node_water/                        # NODE управление водой
│   ├── main/
│   │   ├── app_main.c
│   │   └── water_config.h
│   │
│   ├── components/
│   │   ├── pump_manager/             # Насосы воды
│   │   ├── valve_manager/            # Клапаны (зоны)
│   │   └── water_controller/         # Логика воды
│   │
│   └── CMakeLists.txt
│
├── node_template/                     # Шаблон нового узла ⭐
│   ├── main/
│   │   ├── app_main.c               # Пример кода
│   │   └── node_config.h
│   ├── components/
│   │   └── node_controller/
│   ├── README.md                     # Как создать свой узел
│   └── CMakeLists.txt
│
├── server/                            # Веб-сервер
│   │
│   ├── backend/                       # Laravel 10 API
│   │   ├── app/
│   │   │   ├── Http/
│   │   │   │   ├── Controllers/
│   │   │   │   │   ├── NodeController.php        # CRUD узлов
│   │   │   │   │   ├── TelemetryController.php   # История данных
│   │   │   │   │   ├── CommandController.php     # Отправка команд
│   │   │   │   │   ├── DashboardController.php   # Данные dashboard
│   │   │   │   │   ├── AuthController.php        # Laravel Sanctum
│   │   │   │   │   └── OtaController.php         # OTA управление ⭐
│   │   │   │   └── Middleware/
│   │   │   │       └── Authenticate.php
│   │   │   │
│   │   │   ├── Models/
│   │   │   │   ├── Node.php          # Модель узла
│   │   │   │   ├── Telemetry.php     # Модель телеметрии
│   │   │   │   ├── Event.php         # События
│   │   │   │   ├── Command.php       # Команды
│   │   │   │   └── User.php          # Пользователи (Sanctum)
│   │   │   │
│   │   │   ├── Services/
│   │   │   │   ├── MqttService.php           # MQTT bridge ⭐
│   │   │   │   ├── TelegramService.php       # Telegram bot ⭐
│   │   │   │   ├── SmsService.php            # SMS gateway ⭐
│   │   │   │   ├── DataAggregatorService.php # Агрегация данных
│   │   │   │   └── OtaService.php            # Управление OTA
│   │   │   │
│   │   │   ├── Events/
│   │   │   │   ├── TelemetryReceived.php     # WebSocket event
│   │   │   │   ├── NodeStatusChanged.php
│   │   │   │   └── CriticalAlert.php
│   │   │   │
│   │   │   └── Console/
│   │   │       └── Commands/
│   │   │           ├── MqttListener.php       # php artisan mqtt:listen
│   │   │           ├── TelegramBot.php        # php artisan telegram:bot
│   │   │           └── DataAggregator.php     # Агрегация старых данных
│   │   │
│   │   ├── database/
│   │   │   ├── migrations/
│   │   │   │   ├── 001_create_users_table.php
│   │   │   │   ├── 002_create_nodes_table.php
│   │   │   │   ├── 003_create_telemetry_table.php
│   │   │   │   ├── 004_create_events_table.php
│   │   │   │   ├── 005_create_commands_table.php
│   │   │   │   └── 006_create_ota_updates_table.php
│   │   │   │
│   │   │   └── seeders/
│   │   │       └── NodeSeeder.php    # Начальные узлы
│   │   │
│   │   ├── routes/
│   │   │   ├── api.php               # API endpoints
│   │   │   └── web.php
│   │   │
│   │   ├── config/
│   │   │   ├── mqtt.php              # MQTT настройки
│   │   │   ├── telegram.php          # Telegram настройки
│   │   │   └── sms.php               # SMS настройки
│   │   │
│   │   ├── .env.example              # Пример конфигурации
│   │   ├── composer.json
│   │   └── artisan
│   │
│   ├── frontend/                      # Vue.js 3 + Vuetify 3
│   │   ├── src/
│   │   │   ├── components/
│   │   │   │   ├── Dashboard.vue            # Главный dashboard
│   │   │   │   ├── NodeCard.vue             # Карточка узла
│   │   │   │   ├── NodeDetail.vue           # Детали узла
│   │   │   │   ├── LineChart.vue            # График линейный
│   │   │   │   ├── PumpControl.vue          # Управление насосами
│   │   │   │   ├── ClimateControl.vue       # Управление климатом
│   │   │   │   ├── LightDimmer.vue          # PWM диммер света ⭐
│   │   │   │   ├── OtaManager.vue           # Управление OTA ⭐
│   │   │   │   └── AlertsPanel.vue          # Панель уведомлений
│   │   │   │
│   │   │   ├── views/
│   │   │   │   ├── Home.vue                 # Главная (Dashboard)
│   │   │   │   ├── Nodes.vue                # Список узлов
│   │   │   │   ├── NodeDetailView.vue       # Детали узла
│   │   │   │   ├── History.vue              # История (графики)
│   │   │   │   ├── Analytics.vue            # Аналитика
│   │   │   │   ├── Settings.vue             # Настройки
│   │   │   │   ├── Login.vue                # Вход (Sanctum)
│   │   │   │   └── OtaPage.vue              # Страница OTA
│   │   │   │
│   │   │   ├── stores/                      # Pinia stores
│   │   │   │   ├── app.js                   # Общее состояние
│   │   │   │   ├── nodes.js                 # Узлы
│   │   │   │   ├── telemetry.js             # Телеметрия
│   │   │   │   ├── auth.js                  # Аутентификация
│   │   │   │   └── alerts.js                # Уведомления
│   │   │   │
│   │   │   ├── services/
│   │   │   │   ├── api.js                   # Axios API
│   │   │   │   └── websocket.js             # WebSocket real-time
│   │   │   │
│   │   │   ├── router/
│   │   │   │   └── index.js                 # Vue Router
│   │   │   │
│   │   │   ├── plugins/
│   │   │   │   └── vuetify.js               # Vuetify настройки
│   │   │   │
│   │   │   ├── App.vue                      # Главный компонент
│   │   │   └── main.js
│   │   │
│   │   ├── public/
│   │   ├── package.json
│   │   ├── vite.config.js
│   │   └── README.md
│   │
│   ├── mqtt_bridge/                   # MQTT демоны
│   │   ├── mqtt_listener.php         # Подписка на топики
│   │   └── telegram_bot.php          # Telegram бот
│   │
│   ├── nginx/                         # Конфигурация nginx
│   │   └── hydro-system.conf
│   │
│   ├── supervisor/                    # Конфигурация supervisor
│   │   ├── mqtt-listener.conf
│   │   └── telegram-bot.conf
│   │
│   ├── docker-compose.yml             # Опционально: Docker
│   └── README.md
│
├── docs/                              # Документация
│   ├── ARCHITECTURE_DETAILED.md      # Детальная архитектура
│   ├── API_REFERENCE.md              # REST API справка
│   ├── MESH_PROTOCOL.md              # Протокол mesh обмена
│   ├── NODE_CREATION_GUIDE.md        # Создание нового узла
│   ├── PINOUT_ALL_NODES.md           # GPIO всех узлов
│   ├── TROUBLESHOOTING.md            # Решение проблем
│   ├── OTA_GUIDE.md                  # Инструкция OTA
│   ├── TELEGRAM_BOT_COMMANDS.md      # Команды Telegram
│   └── SMS_SETUP.md                  # Настройка SMS
│
└── tools/                             # Утилиты
    ├── flash_all.bat                 # Прошивка всех узлов (Windows)
    ├── flash_all.sh                  # Прошивка всех узлов (Linux)
    ├── monitor_mesh.py               # Мониторинг mesh сети
    ├── mqtt_tester.py                # Тестирование MQTT
    ├── node_configurator.py          # Конфигурация узла
    └── backup_restore.sh             # Backup/Restore БД
```

---

## 🔌 ДЕТАЛЬНАЯ РАСПИНОВКА

### ROOT NODE (ESP32-S3 #1)

| GPIO | Назначение | Примечание |
|------|------------|------------|
| - | Нет GPIO! | Только mesh + MQTT |

**I2C, SPI, Display:** НЕТ!

---

### NODE Display (ESP32-S3 #2)

| GPIO | Назначение | Компонент |
|------|------------|-----------|
| 11 | LCD MOSI | ILI9341 SPI |
| 12 | LCD SCLK | ILI9341 SPI |
| 10 | LCD CS | ILI9341 |
| 9 | LCD DC | ILI9341 |
| 14 | LCD RST | ILI9341 |
| 15 | LCD Backlight | PWM |
| 4 | Encoder A (CLK) | Ротационный энкодер |
| 5 | Encoder B (DT) | Ротационный энкодер |
| 6 | Encoder SW (Button) | Кнопка энкодера |

**Датчики:** НЕТ!  
**Насосы:** НЕТ!

---

### NODE pH/EC (ESP32-S3 #3 и дополнительные)

| GPIO | Назначение | Компонент |
|------|------------|-----------|
| **I2C шина:** | | |
| 17 | I2C SCL | pH, EC, OLED |
| 18 | I2C SDA | pH, EC, OLED |
| - | OLED 0x3C | SSD1306 128x64 |
| - | pH 0x0A | Trema pH |
| - | EC 0x08 | Trema EC |
| **Насосы (5 штук):** | | |
| 1 | Насос pH UP | GPIO HIGH = вкл |
| 2 | Насос pH DOWN | GPIO HIGH = вкл |
| 3 | Насос EC A | GPIO HIGH = вкл |
| 4 | Насос EC B | GPIO HIGH = вкл |
| 5 | Насос EC C | GPIO HIGH = вкл |
| **Индикация:** | | |
| 15 | LED статус | RGB или 3 LED (R/G/Y) |
| 16 | Buzzer | Пищалка 3.3V |
| 19 | Кнопка MODE | Переключение режимов |

**NVS:** Хранит все настройки, PID параметры, калибровки

---

### NODE Climate (ESP32)

| GPIO | Назначение | Компонент |
|------|------------|-----------|
| 17 | I2C SCL | Датчики |
| 18 | I2C SDA | Датчики |
| - | SHT3x 0x44 | Температура, влажность |
| - | CCS811 0x5A | CO2 |
| - | Trema Lux 0x12 | Освещенность |

**Исполнители:** НЕТ! (только датчики)  
**Логика:** Отправляет данные на ROOT, ROOT решает что делать

---

### NODE Relay (ESP32)

| GPIO | Назначение | Компонент |
|------|------------|-----------|
| 1 | Актуатор форточки 1 | Реле линейного актуатора 12V |
| 2 | Актуатор форточки 2 | Реле линейного актуатора 12V |
| 3 | Вентилятор | Реле 220V или 12V |
| 7 | LED свет (PWM) | PWM канал для диммирования ⭐ |

**Управление форточками:**
- GPIO HIGH = форточка ОТКРЫТА (актуатор выдвинут)
- GPIO LOW = форточка ЗАКРЫТА (актуатор втянут)
- Время открытия/закрытия: ~10-30 сек

**Управление светом:**
- PWM частота: 5000 Hz
- Разрешение: 8 бит (0-255)
- Яркость: 0-100% (0 = выкл, 100 = полная мощность)

---

### NODE Water (ESP32-C3)

| GPIO | Назначение | Компонент |
|------|------------|-----------|
| 1 | Насос подачи | Реле 12V |
| 2 | Насос слива | Реле 12V |
| 3 | Клапан зона 1 | Соленоидный клапан |
| 4 | Клапан зона 2 | Соленоидный клапан |
| 5 | Клапан зона 3 | Соленоидный клапан |
| 6 | Датчик уровня (опц.) | Аналоговый или digital |

---

## 📡 ПРОТОКОЛ MESH ОБМЕНА (JSON)

### Типы сообщений:

**1. Телеметрия (NODE → ROOT → MQTT → Server):**

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
      "ec_a": {"status": "running", "remaining_ms": 3000, "total_ml": 2300},
      "ec_b": {"status": "idle", "total_ml": 2300},
      "ec_c": {"status": "idle", "total_ml": 1150}
    },
    "pid": {
      "auto_mode": true,
      "ph_target": 6.8,
      "ec_target": 2.0
    },
    "connection_state": "online"  // online/autonomous/emergency
  }
}
```

**2. Конфигурация (Server → MQTT → ROOT → NODE):**

```json
{
  "type": "config",
  "node_id": "ph_ec_001",
  "timestamp": 1697548801,
  "config": {
    "ph_target": 6.8,
    "ec_target": 2.0,
    "ph_min": 6.0,
    "ph_max": 7.5,
    "ec_min": 1.2,
    "ec_max": 2.5,
    "pid": {
      "ph_up": {"kp": 1.0, "ki": 0.5, "kd": 0.1},
      "ph_down": {"kp": 1.0, "ki": 0.5, "kd": 0.1},
      "ec_a": {"kp": 0.8, "ki": 0.3, "kd": 0.05}
    },
    "auto_mode": true,
    "mesh_timeout_ms": 30000
  }
}
```

**3. Команда (Server/Display → ROOT → NODE):**

```json
{
  "type": "command",
  "node_id": "ph_ec_001",
  "timestamp": 1697548802,
  "command": "run_pump",
  "params": {
    "pump": "ph_up",
    "duration_ms": 5000
  }
}
```

**Другие команды:**
- `set_target` - изменить целевое значение
- `set_mode` - auto/manual режим
- `calibrate` - запустить калибровку
- `reset_pid` - сброс PID
- `emergency_stop` - аварийная остановка всех насосов
- `restart` - перезагрузка узла
- `ota_update` - начать OTA обновление

**4. Событие (NODE → ROOT → MQTT → Server):**

```json
{
  "type": "event",
  "node_id": "ph_ec_001",
  "timestamp": 1697548803,
  "level": "critical",  // info/warning/critical/emergency
  "message": "pH критично низкий: 4.8",
  "data": {
    "ph": 4.8,
    "action_taken": "emergency_correction"
  }
}
```

**5. Heartbeat (NODE → ROOT каждые 10 сек):**

```json
{
  "type": "heartbeat",
  "node_id": "ph_ec_001",
  "timestamp": 1697548804,
  "uptime": 36000,  // секунд
  "heap_free": 145000,  // байт
  "connection_state": "online"
}
```

**6. Запрос данных (Display → ROOT):**

```json
{
  "type": "request",
  "from": "display_001",
  "request": "all_nodes_data"
}
```

**Ответ (ROOT → Display):**

```json
{
  "type": "response",
  "to": "display_001",
  "data": {
    "nodes": [
      {
        "node_id": "ph_ec_001",
        "type": "ph_ec",
        "online": true,
        "data": {"ph": 6.5, "ec": 1.8}
      },
      {
        "node_id": "climate_001",
        "type": "climate",
        "online": true,
        "data": {"temp": 24.0, "humidity": 65.0, "co2": 450, "lux": 500}
      }
    ]
  }
}
```

---

## 🤖 NODE pH/EC - АВТОНОМНАЯ РАБОТА (КРИТИЧНО!)

### OLED дисплей SSD1306 128x64 - что показывать:

```
┌──────────────────┐
│ pH/EC Zone 1     │  ← Заголовок (строка 1)
│ pH: 6.5  EC: 1.8 │  ← Показания (строка 2)
│ ● ONLINE  AUTO   │  ← Статус (строка 3)
│ ↑12ml ↓5ml →45ml │  ← Насосы за час (строка 4)
└──────────────────┘
```

**Статусы:**
- `● ONLINE` - связь с ROOT есть (зеленый LED)
- `⚠ DEGRADED` - связь нестабильная (желтый LED)
- `⚡ AUTONOMOUS` - автономный режим (желтый LED мигает)
- `🔴 EMERGENCY` - авария (красный LED)

**Режимы:**
- `AUTO` - PID контроль включен
- `MANUAL` - ручное управление (через кнопку MODE)

### Кнопка MODE на NODE pH/EC:

- **1 нажатие (< 1 сек):** Переключение AUTO ↔ MANUAL
- **2 нажатия:** Сброс Buzzer аларма
- **Долгое (> 3 сек):** Сброс аварийного режима
- **Очень долгое (> 10 сек):** Factory reset

### Автономная работа:

**Сценарий 1: Потеря связи с ROOT**
```
t=0:    Связь с ROOT потеряна
t=10s:  LED желтый, OLED показывает "DEGRADED"
t=30s:  Переключение в AUTONOMOUS mode
        ├─ Загрузка последних настроек из NVS
        ├─ PID продолжает работу
        ├─ Данные буферизуются локально (ring buffer 1000 записей)
        └─ OLED показывает "⚡ AUTONOMOUS"

t=5min: Если связь не восстановлена:
        └─ Более консервативные пороги коррекции
        
t=восстановление:
        ├─ Переключение в ONLINE mode
        ├─ Отправка буфера на ROOT (батчами по 10 записей)
        └─ Синхронизация конфигурации
```

**Сценарий 2: Критичная ситуация (pH < 5.0)**
```
pH упал до 4.8:
├─ Немедленная агрессивная коррекция (pH UP)
├─ Buzzer: 3 сигнала каждые 10 секунд
├─ LED: красный быстро мигает
├─ OLED: "🔴 EMERGENCY pH 4.8!"
├─ Попытка отправить SOS на ROOT (высокий приоритет)
└─ Если ROOT доступен:
    ├─ ROOT → MQTT → Server
    ├─ Server → Telegram "⚠️ КРИТИЧНО! pH 4.8 в зоне 1"
    └─ Server → SMS на телефон владельца
```

---

## 🌐 NODE RELAY - Управление климатом

### Оборудование:

**Линейные актуаторы форточек:**
- Тип: 12V DC, ход 50-100 мм
- Управление: Реле (вкл = открыто, выкл = закрыто)
- Концевики: Встроенные в актуатор
- GPIO: 2 пина (форточка 1, форточка 2)

**Вентилятор:**
- Тип: 220V AC или 12V DC
- Управление: Реле
- GPIO: 1 пин

**Свет (с PWM диммированием):**
- Тип: LED лента/панель 12V
- Управление: PWM + MOSFET (плавная регулировка)
- GPIO: 1 PWM пин
- Яркость: 0-100%

### Логика управления (на ROOT!):

**Если NODE Climate ONLINE:**
```
Climate отправляет данные на ROOT
ROOT принимает решения:
  ├─ Если CO2 > 800 ppm → команда на Relay: "open_vent"
  ├─ Если Temp > 28°C → команда на Relay: "open_windows"
  ├─ Если Humidity > 75% → команда на Relay: "start_fan"
  └─ Если Lux < 500 (днем) → команда на Relay: "light_on 80%"
```

**Если NODE Climate OFFLINE (> 30 сек):**
```
ROOT переходит в fallback режим:
  ├─ Форточки: открывать на 30 мин каждые 3 часа
  ├─ Вентилятор: включать на 15 мин каждый час
  └─ Свет: по таймеру (08:00-22:00)
```

### Комбинированная вентиляция:

```c
void root_climate_logic_task(void *arg) {
    while (1) {
        // Получение данных Climate
        climate_data_t climate = get_climate_data_from_registry("climate_001");
        
        bool should_ventilate = false;
        
        // Проверка CO2
        if (climate.co2 > 800) {
            should_ventilate = true;
            ESP_LOGI(TAG, "Ventilation: CO2 high (%d ppm)", climate.co2);
        }
        
        // Проверка температуры
        if (climate.temp > 28.0) {
            should_ventilate = true;
            ESP_LOGI(TAG, "Ventilation: Temp high (%.1f °C)", climate.temp);
        }
        
        // Проверка влажности
        if (climate.humidity > 75.0) {
            should_ventilate = true;
            ESP_LOGI(TAG, "Ventilation: Humidity high (%.1f %%)", climate.humidity);
        }
        
        // Отправка команды на NODE Relay
        if (should_ventilate) {
            send_command_to_node("relay_001", "start_fan", NULL);
            send_command_to_node("relay_001", "open_windows", NULL);
        } else {
            send_command_to_node("relay_001", "stop_fan", NULL);
            send_command_to_node("relay_001", "close_windows", NULL);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // Каждые 10 секунд
    }
}
```

---

## 📱 TELEGRAM БОТ

### Функции:

**Уведомления (односторонние):**
```
⚠️ pH/EC Zone 1: pH критично низкий 4.8!
🔴 Автономный режим активирован (zone 1)
✅ pH скорректирован до целевого значения
💧 Water pump activated (zone 2)
```

**Статус (команды):**
- `/status` - общий статус всех узлов
- `/ph` - pH всех зон
- `/ec` - EC всех зон
- `/climate` - климатические данные
- `/nodes` - список всех узлов (online/offline)
- `/alerts` - текущие тревоги

**Пример ответа на `/status`:**
```
🌱 Hydro System Status

📊 pH/EC Zones:
  Zone 1: pH 6.5, EC 1.8 ✅
  Zone 2: pH 6.7, EC 1.9 ✅

🌡️ Climate:
  Temp: 24.0°C, Humidity: 65% ✅
  CO2: 450 ppm, Lux: 520 lux ✅

💧 Water: Level 80%, Flow OK ✅

🔧 System: All nodes online ✅
```

---

## 📲 SMS УВЕДОМЛЕНИЯ

### Когда отправлять SMS:

**ТОЛЬКО критичные события:**
1. pH < 5.0 или > 8.0
2. EC > 3.0
3. NODE pH/EC offline > 5 минут
4. ROOT offline > 2 минуты
5. Аварийная остановка насосов
6. Emergency режим активирован

### SMS Gateway:

Использовать один из сервисов:
- **Twilio** (международный, API)
- **SMS.ru** (российский, API)
- **SMSC.ru** (российский, HTTP API)

**Интеграция в Laravel:**

```php
// app/Services/SmsService.php
class SmsService {
    public function sendAlert($message) {
        // Twilio API
        $twilio = new Client(config('sms.twilio_sid'), config('sms.twilio_token'));
        
        $twilio->messages->create(
            config('sms.phone_number'),  // +79001234567
            [
                'from' => config('sms.twilio_from'),
                'body' => $message
            ]
        );
    }
}
```

**Пример SMS:**
```
⚠️ HYDRO ALERT
pH критично низкий: 4.8
Зона: 1
Время: 17.10.2025 14:35
Действие: авто коррекция
```

---

## 💡 PWM ДИММИРОВАНИЕ СВЕТА

### Настройка PWM (NODE Relay):

```c
// LED PWM конфигурация
#define LED_PWM_GPIO        7
#define LED_PWM_CHANNEL     LEDC_CHANNEL_0
#define LED_PWM_FREQ        5000  // 5 kHz
#define LED_PWM_RESOLUTION  LEDC_TIMER_8_BIT  // 0-255

void light_controller_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LED_PWM_RESOLUTION,
        .freq_hz = LED_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LED_PWM_CHANNEL,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_PWM_GPIO,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

// Установка яркости 0-100%
void light_set_brightness(uint8_t percent) {
    uint32_t duty = (percent * 255) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LED_PWM_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LED_PWM_CHANNEL);
}
```

### Управление через веб-интерфейс:

```vue
<!-- LightDimmer.vue -->
<template>
  <v-card>
    <v-card-title>Light Control</v-card-title>
    <v-card-text>
      <v-slider
        v-model="brightness"
        min="0"
        max="100"
        step="5"
        thumb-label="always"
        @update:model-value="updateBrightness"
      >
        <template v-slot:prepend>
          <v-icon>mdi-lightbulb-off</v-icon>
        </template>
        <template v-slot:append>
          <v-icon>mdi-lightbulb-on</v-icon>
        </template>
      </v-slider>
      
      <div class="text-center mt-4">
        <v-chip color="primary">{{ brightness }}%</v-chip>
      </div>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { ref } from 'vue'
import axios from 'axios'

const brightness = ref(80)

async function updateBrightness(value) {
  await axios.post('/api/nodes/relay_001/command', {
    command: 'set_light_brightness',
    params: { brightness: value }
  })
}
</script>
```

---

## 🗄️ БАЗА ДАННЫХ (PostgreSQL)

### Схема БД:

```sql
-- Таблица пользователей (Laravel Sanctum)
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255),
    email VARCHAR(255) UNIQUE,
    password VARCHAR(255),
    remember_token VARCHAR(100),
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

-- Таблица узлов
CREATE TABLE nodes (
    id SERIAL PRIMARY KEY,
    node_id VARCHAR(32) UNIQUE NOT NULL,
    mac VARCHAR(17),
    type VARCHAR(16) NOT NULL,  -- ph_ec, climate, water, relay, display
    name VARCHAR(64),
    zone VARCHAR(32),  -- зона/помещение
    online BOOLEAN DEFAULT false,
    last_seen TIMESTAMP,
    config JSONB,  -- Конфигурация узла
    last_telemetry JSONB,  -- Последние данные
    firmware_version VARCHAR(16),
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

CREATE INDEX idx_nodes_online ON nodes(online);
CREATE INDEX idx_nodes_type ON nodes(type);
CREATE INDEX idx_nodes_zone ON nodes(zone);

-- Таблица телеметрии (ВСЯ ИСТОРИЯ!)
CREATE TABLE telemetry (
    id BIGSERIAL PRIMARY KEY,
    node_id VARCHAR(32) NOT NULL,
    timestamp TIMESTAMP NOT NULL,
    data JSONB NOT NULL,  -- Данные датчиков/насосов
    created_at TIMESTAMP
);

CREATE INDEX idx_telemetry_node_time ON telemetry(node_id, timestamp DESC);
CREATE INDEX idx_telemetry_timestamp ON telemetry(timestamp DESC);

-- Партиционирование по месяцам (для производительности)
-- ALTER TABLE telemetry PARTITION BY RANGE (timestamp);
-- CREATE TABLE telemetry_2025_10 PARTITION OF telemetry 
--   FOR VALUES FROM ('2025-10-01') TO ('2025-11-01');

-- Таблица событий
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

CREATE INDEX idx_events_node ON events(node_id);
CREATE INDEX idx_events_level ON events(level);
CREATE INDEX idx_events_ack ON events(acknowledged);

-- Таблица команд (история)
CREATE TABLE commands (
    id SERIAL PRIMARY KEY,
    node_id VARCHAR(32),
    command VARCHAR(64),
    params JSONB,
    result VARCHAR(16),  -- success, failed, timeout
    sent_at TIMESTAMP,
    executed_at TIMESTAMP,
    created_at TIMESTAMP
);

-- Таблица OTA обновлений
CREATE TABLE ota_updates (
    id SERIAL PRIMARY KEY,
    firmware_file VARCHAR(255),
    version VARCHAR(16),
    target_node_type VARCHAR(16),  -- ph_ec, climate, water, etc.
    description TEXT,
    file_size BIGINT,
    md5_hash VARCHAR(32),
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

-- Таблица OTA статусов узлов
CREATE TABLE ota_node_status (
    id SERIAL PRIMARY KEY,
    node_id VARCHAR(32),
    update_id INTEGER REFERENCES ota_updates(id),
    status VARCHAR(16),  -- pending, downloading, updating, success, failed
    progress INTEGER,  -- 0-100%
    error_message TEXT,
    started_at TIMESTAMP,
    completed_at TIMESTAMP
);
```

---

## 🔔 СИСТЕМА УВЕДОМЛЕНИЙ

### Приоритеты:

| Уровень | Telegram | SMS | WebSocket | Email |
|---------|----------|-----|-----------|-------|
| **info** | ✅ | ❌ | ✅ | ❌ |
| **warning** | ✅ | ❌ | ✅ | ❌ |
| **critical** | ✅ | ✅ | ✅ | ❌ |
| **emergency** | ✅ | ✅ | ✅ | ❌ |

### Telegram бот - команды:

```
/start - Начало работы с ботом
/status - Общий статус системы
/ph - pH всех зон
/ec - EC всех зон
/climate - Температура, влажность, CO2
/nodes - Список всех узлов
/alerts - Текущие тревоги
/help - Помощь
```

---

## 🔄 OTA ОБНОВЛЕНИЯ

### Процесс обновления:

**Веб-интерфейс → Сервер → MQTT → ROOT → NODE**

```
1. Пользователь загружает .bin файл в веб-интерфейс
   ↓
2. Laravel сохраняет в storage/ota/
   ↓
3. Расчет MD5 хэша
   ↓
4. Создание записи в БД (ota_updates)
   ↓
5. Выбор целевых узлов (по типу или конкретные)
   ↓
6. MQTT publish: hydro/ota/start/{node_id}
   ↓
7. ROOT получает команду
   ↓
8. ROOT раздает прошивку узлам через mesh (chunks по 1KB)
   ↓
9. NODE скачивает, проверяет MD5, устанавливает
   ↓
10. NODE перезагружается, отправляет новую версию
    ↓
11. Server обновляет статус в БД
```

### Безопасность OTA:

- ✅ Проверка MD5 хэша перед установкой
- ✅ Rollback при ошибке (dual partition)
- ✅ Тестирование на одном узле перед массовым обновлением
- ✅ Таймаут 5 минут на узел
- ❌ Не обновлять все одновременно (поэтапно)

---

## 💾 ХРАНЕНИЕ КОНФИГУРАЦИИ В NVS (КАЖДАЯ НОДА!)

### Общий компонент: common/node_config/

```c
// node_config.h

// Базовая структура конфигурации (общая для всех)
typedef struct {
    char node_id[32];             // Уникальный ID
    char node_type[16];           // Тип узла
    char zone[32];                // Зона/помещение
    bool config_valid;            // Флаг валидности
    uint32_t config_version;      // Версия конфигурации
    uint64_t last_updated;        // Время последнего обновления
} base_config_t;

// Специфичная конфигурация для pH/EC узла
typedef struct {
    base_config_t base;
    
    // Целевые значения
    float ph_target;
    float ec_target;
    
    // Рабочие диапазоны
    float ph_min, ph_max;
    float ec_min, ec_max;
    
    // Аварийные пороги
    float ph_emergency_low, ph_emergency_high;
    float ec_emergency_high;
    
    // PID параметры для каждого насоса
    struct {
        float kp, ki, kd;
        float output_min, output_max;
        bool enabled;
    } pump_pid[5];
    
    // Safety
    uint32_t max_pump_time_ms;
    uint32_t cooldown_ms;
    uint32_t max_daily_volume_ml;
    
    // Автономный режим
    bool autonomous_enabled;
    uint32_t mesh_timeout_ms;
    
    // Калибровка
    float ph_cal_offset;
    float ec_cal_offset;
} ph_ec_node_config_t;

// Специфичная конфигурация для Climate узла
typedef struct {
    base_config_t base;
    
    float temp_target;
    float humidity_target;
    uint16_t co2_max;
    uint16_t lux_min;
} climate_node_config_t;

// Специфичная конфигурация для Relay узла
typedef struct {
    base_config_t base;
    
    // Свет
    struct {
        uint8_t brightness;       // 0-100%
        char schedule_on[6];      // "08:00"
        char schedule_off[6];     // "22:00"
        bool schedule_enabled;
        bool lux_control;         // Управление по освещенности
        uint16_t lux_threshold;   // Порог включения
    } light;
    
    // Вентиляция
    struct {
        uint16_t co2_threshold;   // 800 ppm
        float temp_threshold;     // 28.0°C
        float humidity_threshold; // 75.0%
    } ventilation;
    
    // Форточки (линейные актуаторы)
    struct {
        uint32_t open_time_ms;    // Время полного открытия
        uint32_t close_time_ms;   // Время полного закрытия
    } windows;
} relay_node_config_t;

// API для всех узлов:
esp_err_t node_config_init(void);
esp_err_t node_config_load(void *config, size_t size, const char *namespace);
esp_err_t node_config_save(const void *config, size_t size, const char *namespace);
esp_err_t node_config_reset_to_default(void *config, const char *node_type);

// Обработка команды обновления конфигурации
void node_config_handle_update(cJSON *new_config);
```

### Использование в NODE pH/EC:

```c
void node_ph_ec_init_config(void) {
    ph_ec_node_config_t config;
    
    // Попытка загрузить из NVS
    if (node_config_load(&config, sizeof(config), "ph_ec_ns") != ESP_OK) {
        ESP_LOGW(TAG, "No config in NVS, loading defaults...");
        node_config_reset_to_default(&config, "ph_ec");
        node_config_save(&config, sizeof(config), "ph_ec_ns");
    }
    
    // Применение конфигурации
    apply_config(&config);
}

// При получении новой конфигурации от ROOT
void on_config_received(cJSON *json_config) {
    ph_ec_node_config_t config;
    
    // Парсинг JSON
    config.ph_target = cJSON_GetObjectItem(json_config, "ph_target")->valuedouble;
    config.ec_target = cJSON_GetObjectItem(json_config, "ec_target")->valuedouble;
    // ... остальные параметры
    
    // Сохранение в NVS
    node_config_save(&config, sizeof(config), "ph_ec_ns");
    
    // Применение
    apply_config(&config);
    
    ESP_LOGI(TAG, "Configuration updated and saved to NVS");
}
```

---

## 🚀 ПОРЯДОК РЕАЛИЗАЦИИ

### Фаза 1: Базовая mesh-сеть (3-4 дня)
**Цель:** Заработала базовая связь ROOT ↔ NODE

1. ✅ Создать структуру проекта `mesh_hydro_v2/`
2. ✅ Реализовать `common/mesh_manager` (ROOT и NODE режимы)
3. ✅ Реализовать `common/mesh_protocol` (JSON parse/create)
4. ✅ Реализовать `common/node_config` (NVS save/load)
5. ✅ Прототип: ROOT (S3 #1) + тестовый NODE (любой ESP32)
6. ✅ Протестировать отправку/прием JSON через mesh

**Критерий успеха:** ROOT получает данные от NODE, NODE получает команды

---

### Фаза 2: ROOT узел (3-4 дня)
**Цель:** ROOT полностью функционален

7. ✅ Реализовать `root_node/components/node_registry`
8. ✅ Реализовать `root_node/components/mqtt_client`
9. ✅ Реализовать `root_node/components/data_router`
10. ✅ Реализовать `root_node/components/climate_logic` (резервная логика)
11. ✅ Создать `root_node/main/app_main.c`
12. ✅ Протестировать ROOT ↔ MQTT ↔ локальный Mosquitto

**Критерий успеха:** ROOT маршрутизирует данные NODE → MQTT и обратно

---

### Фаза 3: Сервер (базовый) (3-4 дня)
**Цель:** Сервер принимает данные и отображает

13. ✅ Настроить Laravel 10 проект
14. ✅ Создать миграции БД (nodes, telemetry, events)
15. ✅ Реализовать Models (Node, Telemetry, Event)
16. ✅ Реализовать `MqttService` (подписка на топики)
17. ✅ Создать CLI команду `php artisan mqtt:listen`
18. ✅ Создать базовые API endpoints
19. ✅ Протестировать ROOT → MQTT → Laravel → PostgreSQL

**Критерий успеха:** Данные сохраняются в БД

---

### Фаза 4: NODE pH/EC #1 (4-5 дней) ⚠️ КРИТИЧНО!
**Цель:** Критичный узел с автономией

20. ✅ Портировать из hydro1.0:
    - sensor_manager (pH, EC)
    - pump_manager (5 насосов)
    - adaptive_pid (AI PID)

21. ✅ Реализовать `oled_display` (SSD1306 128x64, I2C 0x3C)
22. ✅ Реализовать `connection_monitor` (online/autonomous/emergency)
23. ✅ Реализовать `local_storage` (ring buffer 1000 записей)
24. ✅ Реализовать `buzzer_led` (индикация + кнопка MODE)
25. ✅ Реализовать `node_controller` (обработка команд)
26. ✅ Создать `app_main.c`
27. ✅ Тестирование:
    - Нормальная работа (online)
    - Автономный режим (отключить ROOT)
    - Критичная ситуация (pH 4.5)
    - Восстановление связи (синхронизация буфера)

**Критерий успеха:** Узел работает автономно при потере связи!

---

### Фаза 5: Vue.js Dashboard (3-4 дня)
**Цель:** Веб-интерфейс с real-time обновлениями

28. ✅ Настроить Vite + Vue 3 + Vuetify 3
29. ✅ Создать компоненты (Dashboard, NodeCard, Charts)
30. ✅ Настроить WebSocket (Laravel Broadcasting)
31. ✅ Реализовать LightDimmer (PWM слайдер)
32. ✅ Создать страницы (Home, Nodes, History, Settings)
33. ✅ Интегрировать Pinia stores
34. ✅ Протестировать real-time обновления

**Критерий успеха:** Dashboard показывает данные в реальном времени

---

### Фаза 6: NODE Display (3-4 дня)
**Цель:** TFT дисплей показывает все узлы

35. ✅ Портировать LVGL + LCD + Encoder из hydro1.0
36. ✅ Создать экраны:
    - Dashboard (карточки всех узлов)
    - Node Detail (детали узла)
    - Node List (список с прокруткой)
    - System Menu
37. ✅ Реализовать `display_controller`
38. ✅ Протестировать запрос данных от ROOT

**Критерий успеха:** На TFT виден статус всех узлов

---

### Фаза 7: NODE Climate + Relay (3-4 дня)
**Цель:** Климат-контроль работает

39. ✅ Реализовать NODE Climate (датчики SHT3x, CCS811, Lux)
40. ✅ Реализовать NODE Relay:
    - Линейные актуаторы (форточки)
    - Вентилятор
    - PWM свет
41. ✅ Реализовать логику на ROOT (`climate_logic`)
42. ✅ Протестировать цепочку:
    - Climate → ROOT → Relay (открыть форточку при CO2 > 800)

**Критерий успеха:** Форточки открываются автоматически

---

### Фаза 8: NODE Water (2-3 дня)
**Цель:** Управление водой

43. ✅ Реализовать NODE Water (насосы, клапаны)
44. ✅ Протестировать управление водой через веб-интерфейс

---

### Фаза 9: Telegram + SMS (2-3 дня)
**Цель:** Мобильные уведомления

45. ✅ Реализовать TelegramService (бот)
46. ✅ Реализовать SmsService (Twilio/SMS.ru)
47. ✅ Интегрировать с событиями Laravel
48. ✅ Протестировать:
    - Критичное событие → SMS + Telegram
    - Команда /status в Telegram

**Критерий успеха:** SMS приходит при pH < 5.0

---

### Фаза 10: OTA система (3-4 дня)
**Цель:** Обновление прошивок через веб-интерфейс

49. ✅ Реализовать `common/ota_manager` для NODE
50. ✅ Реализовать `ota_coordinator` для ROOT
51. ✅ Создать OtaController в Laravel
52. ✅ Создать OtaManager.vue в интерфейсе
53. ✅ Протестировать обновление узла

**Критерий успеха:** Обновили NODE через веб-интерфейс

---

### Фаза 11: Аутентификация (1-2 дня)
**Цель:** Защита веб-интерфейса

54. ✅ Настроить Laravel Sanctum
55. ✅ Создать Login.vue
56. ✅ Middleware для защиты API
57. ✅ Тестирование входа/выхода

---

### Фаза 12: Backup + Export/Import (1-2 дня)
**Цель:** Резервное копирование

58. ✅ Автоматический backup БД (cron)
59. ✅ API endpoint: GET /api/export (скачать JSON)
60. ✅ API endpoint: POST /api/import (загрузить JSON)
61. ✅ UI кнопки Export/Import

---

### Фаза 13: Документация (2-3 дня)
**Цель:** Полная документация для AI и пользователей

62. ✅ AI_INSTRUCTIONS.md (команды, правила, API)
63. ✅ ARCHITECTURE_DETAILED.md (архитектура)
64. ✅ API_REFERENCE.md (все endpoints)
65. ✅ MESH_PROTOCOL.md (формат JSON)
66. ✅ NODE_CREATION_GUIDE.md (как добавить узел)
67. ✅ PINOUT_ALL_NODES.md (GPIO)
68. ✅ OTA_GUIDE.md (инструкция OTA)
69. ✅ TELEGRAM_BOT_COMMANDS.md (команды бота)
70. ✅ SMS_SETUP.md (настройка SMS)
71. ✅ TROUBLESHOOTING.md (решение проблем)

---

### Фаза 14: Финальное тестирование (3-5 дней)
**Цель:** Полная система работает стабильно

72. ✅ Тестирование всех узлов вместе
73. ✅ Стресс-тест (10+ узлов, длительная работа)
74. ✅ Тестирование отказоустойчивости:
    - Отключение ROOT → автономия pH/EC
    - Отключение Climate → fallback логика на ROOT
    - Потеря WiFi → mesh продолжает работу
75. ✅ Тестирование OTA на всех типах узлов
76. ✅ Тестирование уведомлений (Telegram + SMS)
77. ✅ Нагрузочное тестирование веб-интерфейса

**Критерий успеха:** Система работает 24 часа без ошибок

---

## ⏱️ ОБЩАЯ ОЦЕНКА ВРЕМЕНИ

| Фаза | Дни | Сложность |
|------|-----|-----------|
| 1. Базовая mesh | 3-4 | ⭐⭐⭐ |
| 2. ROOT | 3-4 | ⭐⭐⭐ |
| 3. Сервер базовый | 3-4 | ⭐⭐ |
| 4. NODE pH/EC | 4-5 | ⭐⭐⭐⭐ (критично!) |
| 5. Vue Dashboard | 3-4 | ⭐⭐ |
| 6. NODE Display | 3-4 | ⭐⭐⭐ |
| 7. Climate + Relay | 3-4 | ⭐⭐ |
| 8. Water | 2-3 | ⭐ |
| 9. Telegram + SMS | 2-3 | ⭐⭐ |
| 10. OTA | 3-4 | ⭐⭐⭐ |
| 11. Auth | 1-2 | ⭐ |
| 12. Backup | 1-2 | ⭐ |
| 13. Документация | 2-3 | ⭐⭐ |
| 14. Тестирование | 3-5 | ⭐⭐⭐ |
| **ИТОГО:** | **36-51 день** | |

**Реалистичная оценка:** ~40-45 дней (1.5-2 месяца) при работе 5-6 часов/день

---

## 🎯 КРИТИЧНЫЕ ПРАВИЛА ДЛЯ AI

### Правило #1: NODE pH/EC - святое!
- ✅ ВСЕГДА автономная работа
- ✅ ВСЕГДА сохранение в NVS
- ✅ ВСЕГДА локальный буфер
- ✅ ВСЕГДА watchdog reset
- ❌ НИКОГДА не блокировать PID при потере связи

### Правило #2: JSON размер < 1 KB
- Mesh пакет: max 1456 байт
- Использовать краткие ключи
- Батчить данные при синхронизации

### Правило #3: Используй существующий код
- Портируй из hydro1.0, не пиши с нуля
- sensor_manager, pump_manager, adaptive_pid - готовые!
- LVGL экраны - адаптируй, не создавай новые

### Правило #4: NVS для всех узлов
- Каждый узел хранит свою конфигурацию
- Обновление через команду `config` от ROOT
- Fallback на NVS при потере связи

### Правило #5: PWM свет
- ledc_timer для PWM
- Частота 5 kHz
- Плавные переходы (fade)

---

## 📦 SHOPPING LIST (Что докупить)

### Минимум для старта:
- 1× ESP32 обычный (~$3-4) - NODE Climate
- 1× ESP32 обычный (~$3-4) - NODE Relay  
- 1× ESP32-C3 (~$2-3) - NODE Water
- 2× Линейные актуаторы 12V (~$10-15 каждый) - форточки
- 1× OLED SSD1306 128x64 (~$3-5) - для pH/EC #1
- 1× LED RGB или 3×LED (~$1) - индикация pH/EC
- 1× Buzzer 3.3V (~$1) - аларм pH/EC
- 1× Кнопка (~$0.5) - MODE на pH/EC

**Итого:** ~$35-50

### Для масштабирования:
- N× ESP32-S3 (~$10 каждый) - дополнительные pH/EC узлы
- N× OLED дисплеи
- N× Комплекты насосов

---

## ✅ ФИНАЛЬНАЯ ПРОВЕРКА ПЛАНА

### Все требования учтены:

- ✅ **1a** - ROOT (S3 #1) + Display (S3 #2) + pH/EC (S3 #3)
- ✅ **2** - Форточки (линейные актуаторы) + вентиляция
- ✅ **3a** - NODE Climate только датчики
- ✅ **4c** - OTA hybrid (ROOT HTTP, остальные через mesh)
- ✅ **5c** - Приоритет: Сервер (для сразу видеть данные)
- ✅ **6a** - Laravel Sanctum (логин/пароль)
- ✅ **7c** - Свет: таймер + lux контроль
- ✅ **8c** - Графики: стандартные + произвольные
- ✅ **9d** - Telegram + SMS (без email)
- ✅ **10b** - OLED: pH, EC + статус
- ✅ **11c** - Линейные актуаторы (реле управление)
- ✅ **12c** - Логика на ROOT если Climate offline
- ✅ **13a** - OLED SSD1306 128x64
- ✅ **14a** - Независимые зоны pH/EC
- ✅ **15b** - Telegram: уведомления + статус
- ✅ **16** - СМС критичные (не email)
- ✅ **17a** - Вся история (бесконечно)
- ✅ **18a** - OTA вручную
- ✅ **19b** - WPA2 достаточно
- ✅ **20c** - Auto backup + export/import
- ✅ **21c** - Независимые зоны
- ✅ **22b** - PWM диммирование
- ✅ **23d** - Комбинированная вентиляция

---

## 🎉 ИТОГО

**Проект:** Полностью спланирован!  
**Сложность:** Высокая (но выполнимо)  
**Время:** 40-45 дней  
**Бюджет:** ~$35-50 (минимум)

**Готов к реализации!** 🚀

---

**Следующий шаг:** Начать с Фазы 1 (базовая mesh-сеть)?

