# 🌐 Mesh Hydro System - Backend API

Laravel 11 REST API для централизованного управления и мониторинга гидропонической mesh-системы.

## 📋 Содержание

- [Требования](#требования)
- [Установка](#установка)
- [Конфигурация](#конфигурация)
- [Запуск](#запуск)
- [API Endpoints](#api-endpoints)
- [Консольные команды](#консольные-команды)
- [Архитектура](#архитектура)
- [Troubleshooting](#troubleshooting)

---

## 🔧 Требования (Windows)

### Обязательно:
- **PHP 8.2+** (с расширениями: pdo, pdo_pgsql, mbstring, json, openssl)
  - Скачать: https://windows.php.net/download/
- **PostgreSQL 15+** (обязательно PostgreSQL, не MySQL!)
  - Скачать: https://www.postgresql.org/download/windows/
- **Composer 2.x**
  - Скачать: https://getcomposer.org/download/
- **Mosquitto MQTT Broker**
  - Скачать: https://mosquitto.org/download/

### Опционально:
- **NSSM** (для запуска как Windows Service)
  - Скачать: https://nssm.cc/download
- **Redis for Windows** (для кэширования)
  - Скачать: https://github.com/tporadowski/redis/releases

---

## 📦 Установка

### 1. Установка зависимостей

```bash
cd backend
composer install
```

### 2. Настройка окружения

```bash
# Копируем конфигурационный файл
cp .env.example .env

# Генерируем ключ приложения
php artisan key:generate
```

### 3. Настройка PostgreSQL (Windows)

**Через pgAdmin 4:**
1. Откройте pgAdmin 4 (установлен вместе с PostgreSQL)
2. Создайте базу данных: ПКМ на Databases → Create → Database
   - Name: `hydro_system`
3. Создайте пользователя: ПКМ на Login/Group Roles → Create → Login/Group Role
   - General → Name: `hydro`
   - Definition → Password: `your_secure_password`
   - Privileges → Can login: Yes
4. Дайте права: ПКМ на `hydro_system` → Properties → Security → Privileges
   - Добавьте `hydro` с правами ALL

**Или через psql (cmd):**
```cmd
# Откройте cmd и выполните
"C:\Program Files\PostgreSQL\15\bin\psql.exe" -U postgres

postgres=# CREATE DATABASE hydro_system;
postgres=# CREATE USER hydro WITH PASSWORD 'your_secure_password';
postgres=# GRANT ALL PRIVILEGES ON DATABASE hydro_system TO hydro;
postgres=# \q
```

### 4. Настройка .env файла

Отредактируйте `.env`:

```env
# База данных (PostgreSQL!)
DB_CONNECTION=pgsql
DB_HOST=127.0.0.1
DB_PORT=5432
DB_DATABASE=hydro_system
DB_USERNAME=hydro
DB_PASSWORD=your_secure_password

# MQTT Broker
MQTT_HOST=127.0.0.1
MQTT_PORT=1883
MQTT_USERNAME=hydro_server
MQTT_PASSWORD=mqtt_password

# Telegram Bot (опционально)
TELEGRAM_ENABLED=true
TELEGRAM_BOT_TOKEN=your_bot_token_from_botfather
TELEGRAM_CHAT_ID=your_chat_id

# SMS Gateway (опционально)
SMS_ENABLED=false
SMS_API_KEY=your_sms_api_key
SMS_PHONE=+79991234567
```

### 5. Запуск миграций

```bash
php artisan migrate
```

### 6. Установка Mosquitto MQTT Broker (Windows)

1. **Скачайте установщик:**
   - https://mosquitto.org/download/
   - Выберите "mosquitto-x.x.x-install-windows-x64.exe"

2. **Установите:**
   - Запустите установщик
   - Установите в `C:\Program Files\mosquitto\`
   - ✅ Отметьте "Install as a Windows Service"

3. **Настройте:**
   Создайте файл `C:\Program Files\mosquitto\mosquitto.conf`:
   ```
   listener 1883
   allow_anonymous false
   password_file C:\Program Files\mosquitto\passwd
   ```

4. **Создайте пользователя:**
   ```cmd
   cd "C:\Program Files\mosquitto"
   mosquitto_passwd.exe -c passwd hydro_server
   # Введите пароль
   ```

5. **Запустите сервис:**
   ```cmd
   net stop mosquitto
   net start mosquitto
   ```

6. **Проверьте:**
   ```cmd
   # Откройте cmd 1 (subscriber)
   mosquitto_sub.exe -h localhost -t "test/topic" -u hydro_server -P your_password
   
   # Откройте cmd 2 (publisher)
   mosquitto_pub.exe -h localhost -t "test/topic" -m "Hello" -u hydro_server -P your_password
   ```

---

## ⚙️ Конфигурация

### Настройка MQTT Listener как Windows Service (NSSM)

Для автоматического запуска MQTT Listener при загрузке Windows используйте NSSM.

1. **Скачайте NSSM:**
   - https://nssm.cc/download
   - Распакуйте в `C:\nssm\`

2. **Установите MQTT Listener как сервис:**
   ```cmd
   # Откройте cmd от имени администратора
   cd C:\nssm\win64
   
   nssm.exe install HydroMqttListener "C:\php\php.exe" ^
     "C:\esp\hydro\mesh\mesh_hydro\server\backend\artisan" mqtt:listen
   
   nssm.exe set HydroMqttListener AppDirectory "C:\esp\hydro\mesh\mesh_hydro\server\backend"
   nssm.exe set HydroMqttListener AppStdout "C:\esp\hydro\mesh\logs\mqtt-listener.log"
   nssm.exe set HydroMqttListener AppStderr "C:\esp\hydro\mesh\logs\mqtt-listener-error.log"
   nssm.exe set HydroMqttListener AppRotateFiles 1
   nssm.exe set HydroMqttListener AppRotateBytes 10485760
   
   # Запустите сервис
   nssm.exe start HydroMqttListener
   ```

3. **Управление сервисом:**
   ```cmd
   # Проверка статуса
   nssm.exe status HydroMqttListener
   
   # Остановить
   nssm.exe stop HydroMqttListener
   
   # Запустить
   nssm.exe start HydroMqttListener
   
   # Удалить сервис
   nssm.exe remove HydroMqttListener confirm
   ```

4. **Или используйте GUI:**
   ```cmd
   nssm.exe edit HydroMqttListener
   ```

### Настройка через Task Scheduler (альтернатива)

Если не хотите использовать NSSM:

1. Откройте **Task Scheduler** (Планировщик заданий)
2. Создайте задачу:
   - General → Name: "Hydro MQTT Listener"
   - General → Run whether user is logged on or not
   - Triggers → At startup
   - Actions → Start a program:
     - Program: `C:\php\php.exe`
     - Arguments: `artisan mqtt:listen`
     - Start in: `C:\esp\hydro\mesh\mesh_hydro\server\backend`
   - Settings → If the task fails, restart every: 1 minute

---

## 🚀 Запуск

### Development режим

**Терминал 1 - API сервер:**
```bash
php artisan serve
# Доступен на http://localhost:8000
```

**Терминал 2 - MQTT Listener:**
```bash
php artisan mqtt:listen
```

**Терминал 3 (опционально) - Telegram Bot:**
```bash
php artisan telegram:bot
```

### Production режим

Используйте Supervisor для управления процессами (см. раздел "Настройка Supervisor").

---

## 📡 API Endpoints

### Dashboard

- `GET /api/dashboard/summary` - Общая сводка системы
- `GET /api/status` - Статус всех компонентов
- `GET /api/health` - Health check

### Nodes (Узлы)

- `GET /api/nodes` - Список всех узлов
  - Query params: `?type=ph_ec`, `?status=online`
- `GET /api/nodes/{nodeId}` - Детали узла
- `POST /api/nodes` - Создать/обновить узел
- `POST /api/nodes/{nodeId}/command` - Отправить команду
  ```json
  {
    "command": "calibrate",
    "params": {"ph": 7.0}
  }
  ```
- `PUT /api/nodes/{nodeId}/config` - Обновить конфигурацию
- `GET /api/nodes/{nodeId}/statistics` - Статистика узла
  - Query params: `?hours=24`
- `DELETE /api/nodes/{nodeId}` - Удалить узел

### Telemetry (Телеметрия)

- `GET /api/telemetry` - История телеметрии
  - Query params: `?node_id=ph_ec_001`, `?hours=24`, `?limit=1000`
- `GET /api/telemetry/latest` - Последние данные от всех узлов
- `GET /api/telemetry/aggregate` - Агрегированные данные
  ```
  ?node_id=ph_ec_001&field=ph&hours=24&interval=1hour
  ```
- `GET /api/telemetry/export` - Экспорт в CSV
  - Query params: `?node_id=ph_ec_001`, `?hours=168`

### Events (События)

- `GET /api/events` - Список событий
  - Query params: `?node_id=`, `?level=critical`, `?status=active`, `?critical_only=true`
- `GET /api/events/statistics` - Статистика событий
- `GET /api/events/{id}` - Детали события
- `POST /api/events/{id}/resolve` - Пометить как решенное
- `POST /api/events/resolve-bulk` - Массовое резолвение
  ```json
  {
    "event_ids": [1, 2, 3],
    "resolved_by": "admin"
  }
  ```
- `DELETE /api/events/{id}` - Удалить событие (только resolved)

### Примеры запросов

**Получить все онлайн узлы:**
```bash
curl http://localhost:8000/api/nodes?status=online
```

**Отправить команду узлу:**
```bash
curl -X POST http://localhost:8000/api/nodes/ph_ec_001/command \
  -H "Content-Type: application/json" \
  -d '{"command": "calibrate", "params": {"ph": 7.0}}'
```

**Получить телеметрию за 24 часа:**
```bash
curl "http://localhost:8000/api/telemetry?node_id=ph_ec_001&hours=24"
```

---

## 🛠️ Консольные команды

### MQTT Listener

Прослушивание MQTT сообщений от ROOT узла:

```bash
php artisan mqtt:listen

# С опциями
php artisan mqtt:listen --reconnect-delay=5 --max-retries=10
```

### Telegram Bot

Запуск Telegram бота для уведомлений:

```bash
php artisan telegram:bot
```

### Проверка статуса узлов

Проверка всех узлов и отметка офлайн:

```bash
php artisan nodes:check-status

# С уведомлениями
php artisan nodes:check-status --notify
```

### Очистка телеметрии

Удаление старых записей телеметрии:

```bash
# Удалить данные старше 365 дней
php artisan telemetry:cleanup --days=365

# Dry run (посмотреть что будет удалено)
php artisan telemetry:cleanup --days=365 --dry-run
```

### Запланированные задачи (Windows Task Scheduler)

Для автоматического выполнения задач создайте задачу в Task Scheduler:

1. Откройте **Task Scheduler** (Планировщик заданий)
2. Create Basic Task:
   - Name: "Hydro Scheduler"
   - Trigger: "When the computer starts"
   - Action: "Start a program"
     - Program: `C:\php\php.exe`
     - Arguments: `artisan schedule:run`
     - Start in: `C:\esp\hydro\mesh\mesh_hydro\server\backend`
   - Settings: 
     - ✅ Run task every: 1 minute
     - ✅ Repeat task every: 1 minute
     - ✅ For a duration of: Indefinitely

Или используйте PowerShell скрипт `scheduler.ps1`:
```powershell
# scheduler.ps1
while ($true) {
    cd C:\esp\hydro\mesh\mesh_hydro\server\backend
    php artisan schedule:run
    Start-Sleep -Seconds 60
}
```

Запланированные задачи:
- **Каждую минуту:** Проверка статуса узлов
- **Каждый час:** Автоматическое резолвение старых событий
- **Каждое воскресенье 03:00:** Очистка старой телеметрии

---

## 🏗️ Архитектура

### Models

- **Node** - Узлы mesh-сети (ph_ec, climate, relay, water, display, root)
- **Telemetry** - Телеметрия от узлов (JSONB данные)
- **Event** - События и алерты (info, warning, critical, emergency)
- **Command** - История команд отправленных узлам

### Services

- **MqttService** - Работа с MQTT брокером (pub/sub)
- **TelegramService** - Отправка Telegram уведомлений
- **SmsService** - Отправка SMS для критичных событий

### Console Commands

- **MqttListenerCommand** - Прослушивание MQTT сообщений
- **TelegramBotCommand** - Telegram бот
- **CheckNodesStatusCommand** - Проверка статуса узлов
- **CleanupTelemetryCommand** - Очистка старых данных

### HTTP Controllers

- **DashboardController** - Общая информация и статус
- **NodeController** - Управление узлами
- **TelemetryController** - Работа с телеметрией
- **EventController** - Управление событиями

### База данных (PostgreSQL)

**Таблицы:**
- `nodes` - Узлы с конфигурацией (JSONB)
- `telemetry` - Телеметрия с GIN индексом на JSONB
- `events` - События с резолвением
- `commands` - История команд

**Индексы:**
- Составные индексы для быстрых запросов
- GIN индексы для поиска по JSONB
- Партиционирование telemetry по дате (для больших объемов)

---

## 🐛 Troubleshooting

### MQTT не подключается (Windows)

**Проблема:** `MQTT connection failed`

**Решение:**
1. Проверьте что Mosquitto запущен:
   ```cmd
   sc query mosquitto
   # Или в Services.msc найдите "Mosquitto Broker"
   ```
2. Проверьте логи:
   ```cmd
   notepad "C:\Program Files\mosquitto\mosquitto.log"
   ```
3. Проверьте настройки в `.env`:
   ```
   MQTT_HOST=127.0.0.1
   MQTT_PORT=1883
   ```
4. Проверьте firewall:
   ```cmd
   # Откройте порт 1883
   netsh advfirewall firewall add rule name="Mosquitto MQTT" dir=in action=allow protocol=TCP localport=1883
   ```

### PostgreSQL ошибки (Windows)

**Проблема:** `SQLSTATE[42P01]: Undefined table`

**Решение:**
```cmd
php artisan migrate:fresh
```

**Проблема:** `connection refused`

**Решение:**
1. Проверьте что PostgreSQL запущен:
   ```cmd
   sc query postgresql-x64-15
   # Или в Services.msc найдите "postgresql-x64-15"
   ```
2. Запустите если остановлен:
   ```cmd
   net start postgresql-x64-15
   ```
3. Проверьте настройки подключения в `.env`
4. Проверьте `pg_hba.conf`:
   - Файл: `C:\Program Files\PostgreSQL\15\data\pg_hba.conf`
   - Добавьте строку: `host all all 127.0.0.1/32 md5`
   - Перезапустите PostgreSQL

### Telegram бот не работает

**Проблема:** `Telegram API error`

**Решение:**
1. Проверьте токен бота в `.env`
2. Получите chat_id:
   ```bash
   curl https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```
3. Проверьте подключение:
   ```bash
   php artisan tinker
   >>> app(\App\Services\TelegramService::class)->checkConnection()
   ```

### Высокая нагрузка на БД (Windows)

**Проблема:** Медленные запросы телеметрии

**Решение:**
1. Проверьте индексы в pgAdmin 4 или через psql:
   ```sql
   \di+ telemetry*
   ```
2. Включите партиционирование:
   ```sql
   CREATE TABLE telemetry_2025_01 PARTITION OF telemetry
   FOR VALUES FROM ('2025-01-01') TO ('2025-02-01');
   ```
3. Очистите старые данные:
   ```cmd
   php artisan telemetry:cleanup --days=90
   ```
4. Увеличьте производительность PostgreSQL:
   - Откройте: `C:\Program Files\PostgreSQL\15\data\postgresql.conf`
   - Увеличьте: `shared_buffers = 256MB` (для 8GB RAM)
   - Увеличьте: `effective_cache_size = 2GB`
   - Перезапустите PostgreSQL

---

## 📝 Логирование (Windows)

Логи находятся в `storage/logs/laravel.log`

**Просмотр в реальном времени:**
```cmd
# PowerShell
Get-Content storage\logs\laravel.log -Wait -Tail 50

# Или используйте Notepad++, VS Code, Baretail
```

**Уровни логирования:**
- `debug` - Детальная информация для отладки
- `info` - Важные события
- `warning` - Предупреждения
- `error` - Ошибки выполнения

**Дополнительные логи:**
- Mosquitto: `C:\Program Files\mosquitto\mosquitto.log`
- PostgreSQL: `C:\Program Files\PostgreSQL\15\data\log\`
- PHP errors: `C:\php\logs\php_errors.log`

---

## 🔒 Безопасность (Windows)

1. **Измените пароли** в `.env` (БД, MQTT)

2. **Настройте Windows Firewall:**
   ```cmd
   # Разрешите только локальные подключения к PostgreSQL
   netsh advfirewall firewall add rule name="PostgreSQL Local" dir=in action=allow protocol=TCP localport=5432 remoteip=127.0.0.1
   
   # MQTT только локально
   netsh advfirewall firewall add rule name="MQTT Local" dir=in action=allow protocol=TCP localport=1883 remoteip=127.0.0.1
   ```

3. **Регулярный backup базы данных:**
   ```cmd
   # Создайте backup.bat
   @echo off
   set BACKUP_DIR=C:\backups\hydro
   set TIMESTAMP=%date:~-4,4%%date:~-7,2%%date:~-10,2%
   "C:\Program Files\PostgreSQL\15\bin\pg_dump.exe" -U hydro -F c hydro_system > %BACKUP_DIR%\backup_%TIMESTAMP%.dump
   ```
   
   Запланируйте через Task Scheduler ежедневно в 3:00 AM

4. **Используйте SSL для MQTT** (опционально):
   - Сгенерируйте сертификаты с помощью OpenSSL
   - Настройте Mosquitto на порт 8883

---

## 📊 Мониторинг

### Проверка статуса системы

```bash
curl http://localhost:8000/api/status
```

Ответ:
```json
{
  "status": "running",
  "database": "ok",
  "mqtt": "connected",
  "telegram": "ok",
  "system": {
    "php_version": "8.2.x",
    "laravel_version": "10.x",
    "server_time": "2025-01-15 12:00:00"
  }
}
```

---

## 🤝 Поддержка

При возникновении проблем:
1. Проверьте логи: `storage/logs/laravel.log`
2. Проверьте статус: `curl http://localhost:8000/api/status`
3. Проверьте Mosquitto: `sudo systemctl status mosquitto`

---

## 📚 Дополнительные ресурсы

- [Laravel Documentation](https://laravel.com/docs/10.x)
- [PostgreSQL Documentation](https://www.postgresql.org/docs/)
- [Mosquitto MQTT](https://mosquitto.org/documentation/)
- [php-mqtt/client](https://github.com/php-mqtt/client)

---

**Версия:** 1.0.0  
**Laravel:** 10.x  
**PHP:** 8.2+  
**PostgreSQL:** 15+

