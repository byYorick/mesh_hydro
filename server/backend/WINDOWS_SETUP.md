# 🪟 Windows Setup Guide - Mesh Hydro System Backend

Пошаговая инструкция по установке на Windows 10/11.

---

## 📋 Чеклист установки

- [ ] PHP 8.2+ установлен
- [ ] Composer установлен
- [ ] PostgreSQL 15+ установлен
- [ ] Mosquitto MQTT установлен
- [ ] NSSM установлен (опционально)
- [ ] Laravel приложение настроено
- [ ] База данных создана
- [ ] Миграции выполнены
- [ ] MQTT listener запущен

---

## 1️⃣ Установка PHP 8.2+

### Скачивание

1. Откройте https://windows.php.net/download/
2. Скачайте **PHP 8.2+ Thread Safe x64** (ZIP)
3. Распакуйте в `C:\php\`

### Настройка

1. Скопируйте `php.ini-development` → `php.ini`
2. Отредактируйте `php.ini`:

```ini
; Раскомментируйте (уберите ;)
extension=pdo_pgsql
extension=pgsql
extension=mbstring
extension=openssl
extension=curl
extension=fileinfo

; Установите часовой пояс
date.timezone = Europe/Moscow

; Увеличьте лимиты
memory_limit = 256M
upload_max_filesize = 64M
post_max_size = 64M
max_execution_time = 300
```

### Добавление в PATH

1. Откройте **Переменные среды** (Win+Pause → Дополнительные параметры системы)
2. System variables → Path → Изменить
3. Добавьте: `C:\php`
4. ОК → ОК

### Проверка

```cmd
php -v
# Должно показать: PHP 8.2.x (cli)
```

---

## 2️⃣ Установка Composer

1. Скачайте: https://getcomposer.org/Composer-Setup.exe
2. Запустите установщик
3. Укажите путь к PHP: `C:\php\php.exe`
4. Завершите установку

### Проверка

```cmd
composer --version
# Должно показать: Composer version 2.x.x
```

---

## 3️⃣ Установка PostgreSQL 15

1. Скачайте: https://www.postgresql.org/download/windows/
2. Выберите **PostgreSQL 15.x Windows x86-64**
3. Запустите установщик
4. Установите в `C:\Program Files\PostgreSQL\15\`
5. Задайте пароль для пользователя `postgres` (запомните его!)
6. Порт: `5432` (по умолчанию)
7. ✅ Установите pgAdmin 4

### Создание базы данных и пользователя

**Вариант 1: pgAdmin 4 (графический интерфейс)**

1. Откройте pgAdmin 4
2. Подключитесь к серверу (введите пароль postgres)
3. Создайте базу данных:
   - ПКМ на Databases → Create → Database
   - Name: `hydro_system`
   - Owner: `postgres`
   - Save
4. Создайте пользователя:
   - ПКМ на Login/Group Roles → Create → Login/Group Role
   - General → Name: `hydro`
   - Definition → Password: `HydroSecure123!`
   - Privileges → ✅ Can login
   - Save
5. Дайте права пользователю:
   - ПКМ на базе `hydro_system` → Properties
   - Security → Privileges → Add
   - Grantee: `hydro`
   - Privileges: ✅ ALL
   - Save

**Вариант 2: psql (командная строка)**

```cmd
cd "C:\Program Files\PostgreSQL\15\bin"
psql.exe -U postgres

postgres=# CREATE DATABASE hydro_system;
postgres=# CREATE USER hydro WITH PASSWORD 'HydroSecure123!';
postgres=# GRANT ALL PRIVILEGES ON DATABASE hydro_system TO hydro;
postgres=# \q
```

---

## 4️⃣ Установка Mosquitto MQTT Broker

1. Скачайте: https://mosquitto.org/download/
2. Выберите **mosquitto-x.x.x-install-windows-x64.exe**
3. Запустите установщик
4. Установите в `C:\Program Files\mosquitto\`
5. ✅ Отметьте **"Install as a Windows Service"**

### Настройка Mosquitto

1. Откройте Notepad от имени администратора
2. Создайте `C:\Program Files\mosquitto\mosquitto.conf`:

```
listener 1883
allow_anonymous false
password_file C:\Program Files\mosquitto\passwd
log_dest file C:\Program Files\mosquitto\mosquitto.log
log_type all
```

3. Создайте пользователя:

```cmd
cd "C:\Program Files\mosquitto"
mosquitto_passwd.exe -c passwd hydro_server
# Введите пароль: MqttSecure123!
```

4. Перезапустите сервис:

```cmd
net stop mosquitto
net start mosquitto
```

### Проверка работы

Откройте **2 окна CMD**:

**CMD 1 (subscriber):**
```cmd
cd "C:\Program Files\mosquitto"
mosquitto_sub.exe -h localhost -t "test/topic" -u hydro_server -P MqttSecure123!
```

**CMD 2 (publisher):**
```cmd
cd "C:\Program Files\mosquitto"
mosquitto_pub.exe -h localhost -t "test/topic" -m "Hello from Windows!" -u hydro_server -P MqttSecure123!
```

Если в CMD 1 появилось сообщение - ✅ работает!

---

## 5️⃣ Установка Laravel Backend

### Клонирование/копирование проекта

```cmd
# Перейдите в директорию проекта
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
```

### Установка зависимостей

```cmd
composer install
```

Если ошибка `proc_open()` - добавьте в `php.ini`:
```ini
extension=pdo_pgsql
```

### Настройка .env

```cmd
copy .env.example .env
php artisan key:generate
```

Откройте `.env` в Notepad++ или VS Code:

```env
APP_NAME="Mesh Hydro System"
APP_ENV=local
APP_DEBUG=true
APP_URL=http://localhost:8000

DB_CONNECTION=pgsql
DB_HOST=127.0.0.1
DB_PORT=5432
DB_DATABASE=hydro_system
DB_USERNAME=hydro
DB_PASSWORD=HydroSecure123!

MQTT_HOST=127.0.0.1
MQTT_PORT=1883
MQTT_USERNAME=hydro_server
MQTT_PASSWORD=MqttSecure123!

TELEGRAM_ENABLED=false
TELEGRAM_BOT_TOKEN=
TELEGRAM_CHAT_ID=

SMS_ENABLED=false
```

### Выполнение миграций

```cmd
php artisan migrate
```

Должно показать:
```
Migration table created successfully.
Migrating: 2024_01_01_000001_create_nodes_table
Migrated:  2024_01_01_000001_create_nodes_table
...
```

### Проверка API

Запустите сервер:
```cmd
php artisan serve
```

Откройте браузер: http://localhost:8000/api/health

Должен ответить:
```json
{"status":"ok","timestamp":"2025-..."}
```

---

## 6️⃣ Запуск MQTT Listener (вручную)

Откройте **новое окно CMD**:

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
php artisan mqtt:listen
```

Должно показать:
```
╔════════════════════════════════════════╗
║   MQTT Listener for Hydro System      ║
╚════════════════════════════════════════╝

🔌 Connecting to MQTT broker...
✅ Connected successfully!

📡 Subscribing to: hydro/telemetry/#
📡 Subscribing to: hydro/event/#
📡 Subscribing to: hydro/heartbeat/#
📡 Subscribing to: hydro/response/#

🎧 MQTT Listener is running...
Press Ctrl+C to stop
```

---

## 7️⃣ Установка как Windows Service (NSSM)

### Скачивание NSSM

1. Скачайте: https://nssm.cc/release/nssm-2.24.zip
2. Распакуйте в `C:\nssm\`

### Установка MQTT Listener Service

Откройте **CMD от имени администратора**:

```cmd
cd C:\nssm\win64

nssm.exe install HydroMqttListener ^
  "C:\php\php.exe" ^
  "C:\esp\hydro\mesh\mesh_hydro\server\backend\artisan" mqtt:listen

nssm.exe set HydroMqttListener AppDirectory "C:\esp\hydro\mesh\mesh_hydro\server\backend"
nssm.exe set HydroMqttListener DisplayName "Hydro MQTT Listener"
nssm.exe set HydroMqttListener Description "MQTT Listener for Mesh Hydro System"
nssm.exe set HydroMqttListener Start SERVICE_AUTO_START
nssm.exe set HydroMqttListener AppStdout "C:\esp\hydro\mesh\logs\mqtt-listener.log"
nssm.exe set HydroMqttListener AppStderr "C:\esp\hydro\mesh\logs\mqtt-error.log"
nssm.exe set HydroMqttListener AppRotateFiles 1
nssm.exe set HydroMqttListener AppRotateBytes 10485760

# Создайте папку для логов
mkdir C:\esp\hydro\mesh\logs

# Запустите сервис
nssm.exe start HydroMqttListener
```

### Проверка статуса

```cmd
nssm.exe status HydroMqttListener
# Должно показать: SERVICE_RUNNING
```

### Управление через Services.msc

1. Нажмите Win+R
2. Введите: `services.msc`
3. Найдите: **Hydro MQTT Listener**
4. ПКМ → Start/Stop/Restart

---

## 8️⃣ Настройка Scheduler (задачи по расписанию)

### Вариант 1: PowerShell скрипт

Создайте `C:\esp\hydro\mesh\mesh_hydro\server\backend\scheduler.ps1`:

```powershell
# Hydro System Scheduler
$BackendPath = "C:\esp\hydro\mesh\mesh_hydro\server\backend"

Write-Host "Starting Hydro System Scheduler..." -ForegroundColor Green

while ($true) {
    Set-Location $BackendPath
    
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$timestamp] Running schedule..." -ForegroundColor Cyan
    
    php artisan schedule:run
    
    Start-Sleep -Seconds 60
}
```

Запустите:
```powershell
powershell.exe -ExecutionPolicy Bypass -File "C:\esp\hydro\mesh\mesh_hydro\server\backend\scheduler.ps1"
```

### Вариант 2: Task Scheduler

1. Откройте **Task Scheduler** (Win+R → `taskschd.msc`)
2. Action → Create Task
3. General:
   - Name: `Hydro System Scheduler`
   - ✅ Run whether user is logged on or not
   - ✅ Run with highest privileges
4. Triggers → New:
   - Begin: At startup
   - ✅ Enabled
5. Actions → New:
   - Program: `C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe`
   - Arguments: `-ExecutionPolicy Bypass -File "C:\esp\hydro\mesh\mesh_hydro\server\backend\scheduler.ps1"`
   - Start in: `C:\esp\hydro\mesh\mesh_hydro\server\backend`
6. OK

---

## 9️⃣ Telegram Bot (опционально)

### Получение токена

1. Откройте Telegram
2. Найдите @BotFather
3. Отправьте: `/newbot`
4. Следуйте инструкциям
5. Скопируйте токен: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Получение Chat ID

1. Найдите своего бота в Telegram
2. Отправьте ему любое сообщение
3. Откройте в браузере:
   ```
   https://api.telegram.org/bot<YOUR_TOKEN>/getUpdates
   ```
4. Найдите `"chat":{"id":123456789}`
5. Скопируйте ID

### Настройка .env

```env
TELEGRAM_ENABLED=true
TELEGRAM_BOT_TOKEN=1234567890:ABCdefGHIjklMNOpqrsTUVwxyz
TELEGRAM_CHAT_ID=123456789
```

### Проверка

```cmd
php artisan tinker

>>> app(\App\Services\TelegramService::class)->sendMessage("Hello from Windows!");
```

---

## 🔟 Финальная проверка

### 1. Проверьте сервисы

```cmd
# PostgreSQL
sc query postgresql-x64-15

# Mosquitto
sc query mosquitto

# MQTT Listener (если установлен через NSSM)
nssm.exe status HydroMqttListener
```

### 2. Проверьте API

```cmd
# Health check
curl http://localhost:8000/api/health

# System status
curl http://localhost:8000/api/status
```

### 3. Проверьте логи

```cmd
# Laravel
type storage\logs\laravel.log

# Mosquitto
type "C:\Program Files\mosquitto\mosquitto.log"

# MQTT Listener (если через NSSM)
type C:\esp\hydro\mesh\logs\mqtt-listener.log
```

---

## 🎉 Готово!

Ваш Hydro System Backend работает на Windows!

### Следующие шаги:

1. ✅ Настройте ROOT узел для подключения к этому серверу
2. ✅ Установите Frontend (Vue.js)
3. ✅ Настройте автоматический backup БД
4. ✅ Настройте firewall правила

---

## 🔧 Полезные команды

```cmd
# Перезапуск всех сервисов
net stop mosquitto && net start mosquitto
net stop postgresql-x64-15 && net start postgresql-x64-15
nssm.exe restart HydroMqttListener

# Просмотр логов в реальном времени (PowerShell)
Get-Content storage\logs\laravel.log -Wait -Tail 50

# Очистка кэша Laravel
php artisan cache:clear
php artisan config:clear
php artisan route:clear

# Backup БД
"C:\Program Files\PostgreSQL\15\bin\pg_dump.exe" -U hydro -F c hydro_system > backup.dump

# Restore БД
"C:\Program Files\PostgreSQL\15\bin\pg_restore.exe" -U hydro -d hydro_system backup.dump
```

---

**Документация актуальна на:** 2025-01-18  
**OS:** Windows 10/11  
**PHP:** 8.2+  
**PostgreSQL:** 15+

