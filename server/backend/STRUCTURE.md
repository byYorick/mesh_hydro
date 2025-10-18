# 📁 Структура Backend проекта

Быстрая навигация по структуре Laravel приложения Mesh Hydro System.

## 🗂️ Основные директории

```
backend/
├── app/                          # Логика приложения
│   ├── Console/
│   │   ├── Commands/             # Консольные команды
│   │   │   ├── MqttListenerCommand.php        # 🎧 MQTT listener
│   │   │   ├── TelegramBotCommand.php         # 📱 Telegram бот
│   │   │   ├── CheckNodesStatusCommand.php    # ✅ Проверка узлов
│   │   │   └── CleanupTelemetryCommand.php    # 🧹 Очистка данных
│   │   └── Kernel.php            # Scheduler (cron задачи)
│   │
│   ├── Exceptions/
│   │   └── Handler.php           # Обработка ошибок
│   │
│   ├── Http/
│   │   ├── Controllers/          # HTTP контроллеры
│   │   │   ├── Controller.php                 # Базовый контроллер
│   │   │   ├── NodeController.php             # 🔌 Управление узлами
│   │   │   ├── TelemetryController.php        # 📊 Телеметрия
│   │   │   ├── EventController.php            # 🔔 События
│   │   │   └── DashboardController.php        # 📈 Dashboard
│   │   │
│   │   ├── Middleware/           # HTTP middleware
│   │   │   ├── Authenticate.php
│   │   │   ├── TrustProxies.php
│   │   │   └── ...
│   │   │
│   │   └── Kernel.php            # HTTP ядро
│   │
│   ├── Models/                   # Eloquent модели
│   │   ├── Node.php              # 🖥️ Узел mesh-сети
│   │   ├── Telemetry.php         # 📡 Данные телеметрии
│   │   ├── Event.php             # ⚠️ События и алерты
│   │   └── Command.php           # 🎮 История команд
│   │
│   ├── Services/                 # Бизнес-логика
│   │   ├── MqttService.php       # 📨 MQTT клиент (pub/sub)
│   │   ├── TelegramService.php   # 💬 Telegram уведомления
│   │   └── SmsService.php        # 📲 SMS уведомления
│   │
│   └── Providers/
│       └── AppServiceProvider.php
│
├── bootstrap/
│   └── app.php                   # Инициализация Laravel
│
├── config/                       # Конфигурация
│   ├── app.php                   # Основная конфигурация
│   ├── database.php              # PostgreSQL настройки
│   ├── mqtt.php                  # 🔌 MQTT брокер
│   ├── telegram.php              # 📱 Telegram бот
│   ├── sms.php                   # 📲 SMS шлюз
│   └── hydro.php                 # ⚙️ Специфичные настройки системы
│
├── database/
│   ├── migrations/               # Миграции БД
│   │   ├── 2024_01_01_000001_create_nodes_table.php
│   │   ├── 2024_01_01_000002_create_telemetry_table.php
│   │   ├── 2024_01_01_000003_create_events_table.php
│   │   └── 2024_01_01_000004_create_commands_table.php
│   │
│   ├── seeders/                  # Seeder'ы (тестовые данные)
│   └── factories/                # Фабрики моделей
│
├── routes/
│   ├── api.php                   # 🌐 API маршруты (/api/*)
│   ├── web.php                   # 🌍 Web маршруты
│   └── console.php               # 💻 Console маршруты
│
├── storage/
│   ├── app/                      # Файлы приложения
│   ├── framework/                # Laravel кэш/сессии
│   └── logs/                     # 📝 Логи (laravel.log)
│
├── vendor/                       # Composer зависимости
│
├── .env                          # Переменные окружения
├── .env.example                  # Пример .env файла
├── artisan                       # CLI Laravel
├── composer.json                 # PHP зависимости
└── README.md                     # Документация
```

---

## 🔑 Ключевые файлы

### Models (app/Models/)

| Файл | Описание | Связи |
|------|----------|-------|
| `Node.php` | Узел mesh-сети | `hasMany(Telemetry)`, `hasMany(Event)`, `hasMany(Command)` |
| `Telemetry.php` | Телеметрия узла | `belongsTo(Node)` |
| `Event.php` | События системы | `belongsTo(Node)` |
| `Command.php` | Команды узлам | `belongsTo(Node)` |

### Services (app/Services/)

| Файл | Назначение | Основные методы |
|------|------------|-----------------|
| `MqttService.php` | MQTT клиент | `connect()`, `subscribe()`, `publish()`, `handleTelemetry()`, `handleEvent()` |
| `TelegramService.php` | Telegram бот | `sendAlert()`, `sendMessage()`, `sendNodeStatus()` |
| `SmsService.php` | SMS отправка | `sendAlert()`, `sendSms()` |

### Commands (app/Console/Commands/)

| Команда | Запуск | Описание |
|---------|--------|----------|
| `MqttListenerCommand` | `php artisan mqtt:listen` | Прослушивание MQTT сообщений |
| `TelegramBotCommand` | `php artisan telegram:bot` | Telegram бот для уведомлений |
| `CheckNodesStatusCommand` | `php artisan nodes:check-status` | Проверка статуса узлов |
| `CleanupTelemetryCommand` | `php artisan telemetry:cleanup` | Очистка старой телеметрии |

### Controllers (app/Http/Controllers/)

| Контроллер | Префикс | Методы |
|------------|---------|--------|
| `DashboardController` | `/api/dashboard` | `summary()`, `status()` |
| `NodeController` | `/api/nodes` | `index()`, `show()`, `sendCommand()`, `updateConfig()` |
| `TelemetryController` | `/api/telemetry` | `index()`, `latest()`, `aggregate()`, `export()` |
| `EventController` | `/api/events` | `index()`, `show()`, `resolve()`, `resolveBulk()` |

---

## 🗄️ База данных

### Таблицы

```sql
nodes
├── id (primary key)
├── node_id (unique) - "ph_ec_001"
├── node_type - "ph_ec", "climate", etc
├── zone - "Zone 1"
├── mac_address
├── online (boolean)
├── last_seen_at (timestamp)
├── config (jsonb)
├── metadata (jsonb)
└── timestamps

telemetry
├── id (primary key)
├── node_id - внешний ключ
├── node_type
├── data (jsonb) - данные телеметрии
├── received_at (timestamp)
└── timestamps

events
├── id (primary key)
├── node_id - внешний ключ
├── level (enum) - info/warning/critical/emergency
├── message (text)
├── data (jsonb)
├── resolved_at (timestamp)
├── resolved_by
└── timestamps

commands
├── id (primary key)
├── node_id - внешний ключ
├── command - "calibrate", "pump_on", etc
├── params (jsonb)
├── status (enum) - pending/sent/acknowledged/completed/failed
├── sent_at
├── acknowledged_at
├── completed_at
├── response (jsonb)
├── error (text)
└── timestamps
```

### Индексы

- `nodes`: `node_id`, `node_type`, `online`, `last_seen_at`
- `telemetry`: `node_id`, `received_at`, `(node_id, received_at)`, GIN на `data`
- `events`: `node_id`, `level`, `resolved_at`, `(node_id, created_at)`
- `commands`: `node_id`, `status`, `(node_id, status)`

---

## 🔄 Поток данных

### 1. Телеметрия (ROOT → Server)

```
ESP32 ROOT Node
    ↓ MQTT publish
Mosquitto Broker (localhost:1883)
    ↓ topic: hydro/telemetry/#
MqttService::handleTelemetry()
    ↓ сохранение
PostgreSQL (таблица: telemetry)
    ↓ обновление
Node model (last_seen_at)
```

### 2. События (ROOT → Server → Telegram/SMS)

```
ESP32 ROOT Node
    ↓ MQTT publish
Mosquitto Broker
    ↓ topic: hydro/event/#
MqttService::handleEvent()
    ↓ сохранение
PostgreSQL (таблица: events)
    ↓ если critical/emergency
TelegramService::sendAlert()
SmsService::sendAlert()
```

### 3. Команды (Server → ROOT)

```
Frontend/API request
    ↓ POST /api/nodes/{id}/command
NodeController::sendCommand()
    ↓ сохранение в БД
PostgreSQL (таблица: commands)
    ↓ MQTT publish
MqttService::sendCommand()
    ↓ topic: hydro/command/{node_id}
Mosquitto Broker
    ↓ delivery
ESP32 ROOT Node
```

---

## 🔌 MQTT Topics

| Topic | Направление | QoS | Описание |
|-------|-------------|-----|----------|
| `hydro/telemetry/#` | ROOT → Server | 0 | Телеметрия от узлов |
| `hydro/event/#` | ROOT → Server | 1 | События и алерты |
| `hydro/heartbeat/#` | ROOT → Server | 0 | Heartbeat от узлов |
| `hydro/command/{node_id}` | Server → ROOT | 1 | Команды узлам |
| `hydro/config/{node_id}` | Server → ROOT | 1 | Конфигурация узлам |
| `hydro/response/#` | ROOT → Server | 1 | Ответы на команды |
| `hydro/server/status` | Server | 1 | Статус сервера |

---

## 📊 API Endpoints (краткая справка)

```
GET    /api/health                         - Health check
GET    /api/status                         - System status
GET    /api/dashboard/summary              - Dashboard data

GET    /api/nodes                          - List nodes
GET    /api/nodes/{id}                     - Node details
POST   /api/nodes/{id}/command             - Send command
PUT    /api/nodes/{id}/config              - Update config

GET    /api/telemetry                      - Query telemetry
GET    /api/telemetry/latest               - Latest from all
GET    /api/telemetry/aggregate            - Aggregated data
GET    /api/telemetry/export               - Export to CSV

GET    /api/events                         - List events
POST   /api/events/{id}/resolve            - Resolve event
POST   /api/events/resolve-bulk            - Bulk resolve
GET    /api/events/statistics              - Event stats
```

---

## ⚙️ Конфигурация (.env)

### Обязательные переменные

```env
APP_KEY=                    # php artisan key:generate
DB_CONNECTION=pgsql         # ТОЛЬКО PostgreSQL!
DB_HOST=127.0.0.1
DB_DATABASE=hydro_system
DB_USERNAME=hydro
DB_PASSWORD=***

MQTT_HOST=127.0.0.1
MQTT_PORT=1883
MQTT_USERNAME=hydro_server
MQTT_PASSWORD=***
```

### Опциональные переменные

```env
TELEGRAM_ENABLED=true
TELEGRAM_BOT_TOKEN=***
TELEGRAM_CHAT_ID=***

SMS_ENABLED=false
SMS_API_KEY=***

NODE_OFFLINE_TIMEOUT=30           # секунды
TELEMETRY_RETENTION_DAYS=365      # дни
EVENT_AUTO_RESOLVE_HOURS=24       # часы
```

---

## 🚀 Быстрый старт

```bash
# 1. Установка
composer install
cp .env.example .env
php artisan key:generate

# 2. Настройка БД
# Отредактируйте .env
php artisan migrate

# 3. Запуск (dev режим)
# Терминал 1:
php artisan serve

# Терминал 2:
php artisan mqtt:listen

# 4. Проверка
curl http://localhost:8000/api/health
```

---

**Структура актуальна на:** 2025-01-18  
**Laravel версия:** 10.x  
**PHP версия:** 8.2+

