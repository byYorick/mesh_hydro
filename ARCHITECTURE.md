# 🏗️ Архитектура Mesh Hydro System V2

## Топология сети

```
[СЕРВЕР] <--MQTT/WiFi--> [ROOT NODE] <--ESP-WIFI-MESH--> [Все NODE узлы]
```

## Компоненты системы

### 1. ROOT NODE (ESP32-S3 #1)
- Координатор mesh-сети
- MQTT клиент (связь с сервером)
- Node Registry (учет всех узлов)
- Data Router (маршрутизация данных)
- Резервная логика климата

### 2. NODE Display (ESP32-S3 #2)
- TFT дисплей 240x320 (ILI9341)
- LVGL UI
- Ротационный энкодер
- Показ данных всех узлов

### 3. NODE pH/EC (ESP32-S3 #3+)
- Датчики pH/EC (I2C)
- 5 насосов (pH UP/DOWN, EC A/B/C)
- OLED дисплей 128x64 (SSD1306)
- Автономная работа при потере связи
- PID контроллер

### 4. NODE Climate (ESP32)
- SHT3x (температура, влажность)
- CCS811 (CO2)
- Trema Lux (освещенность)
- Только датчики, без исполнителей

### 5. NODE Relay (ESP32)
- Линейные актуаторы форточек (2 шт)
- Вентилятор
- LED свет с PWM диммированием

### 6. NODE Water (ESP32-C3)
- Насосы подачи/слива
- Соленоидные клапаны (3 зоны)
- Датчик уровня воды

### 7. Сервер
- **Backend:** Laravel 11 + PostgreSQL 15
- **Frontend:** Vue.js 3 + Vuetify 3
- **MQTT:** Mosquitto broker
- **Уведомления:** Telegram Bot + SMS Gateway

## Протокол обмена

Формат: JSON через ESP-WIFI-MESH

Типы сообщений:
- `telemetry` - телеметрия (NODE → ROOT → MQTT → Server)
- `command` - команды (Server → MQTT → ROOT → NODE)
- `config` - конфигурация (Server → NODE)
- `event` - события (NODE → Server)
- `heartbeat` - проверка связи (каждые 10 сек)

См. `doc/MESH_PROTOCOL.md` для деталей.

## Безопасность

- WPA2 для mesh-сети
- Laravel Sanctum для веб-интерфейса
- Хранение конфигураций в NVS
- Watchdog таймеры на всех узлах

## Отказоустойчивость

- **NODE pH/EC:** Автономная работа при потере связи с ROOT
- **ROOT:** Резервная логика климата если NODE Climate offline
- **Mesh:** Автоматическое переподключение узлов
- **OTA:** Dual partition с rollback при ошибке

---

Подробнее см. `doc/MESH_HYDRO_V2_FINAL_PLAN.md`

