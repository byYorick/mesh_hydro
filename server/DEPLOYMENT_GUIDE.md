# 🚀 Mesh Hydro System - Production Deployment Guide

Полное руководство по развертыванию системы на Windows Server.

---

## 📋 Содержание

1. [Требования](#требования)
2. [Установка компонентов](#установка-компонентов)
3. [Настройка Backend](#настройка-backend)
4. [Настройка Frontend](#настройка-frontend)
5. [Настройка служб Windows](#настройка-служб-windows)
6. [Мониторинг и обслуживание](#мониторинг-и-обслуживание)
7. [Troubleshooting](#troubleshooting)

---

## 🔧 Требования

### Аппаратные:
- **ОЗУ:** минимум 4 GB, рекомендуется 8 GB
- **Диск:** минимум 20 GB свободного места
- **CPU:** 2+ ядра
- **Сеть:** Ethernet/WiFi для подключения ROOT узла

### Программные:
- **Windows 10/11** или **Windows Server 2016+**
- **PHP 8.2+** с расширениями (pdo_sqlite, mbstring, openssl, curl, fileinfo, zip)
- **Node.js 18+**
- **Composer 2.x**
- **Mosquitto MQTT Broker**
- **NSSM** (для Windows Services)

---

## 📦 Установка компонентов

### 1. PHP 8.2+

1. Скачайте: https://windows.php.net/download/
2. Выберите **Thread Safe x64**
3. Распакуйте в `C:\PHP\`
4. Настройте `php.ini`:
   ```ini
   extension=pdo_sqlite
   extension=sqlite3
   extension=mbstring
   extension=openssl
   extension=curl
   extension=fileinfo
   extension=zip
   
   date.timezone = Europe/Moscow
   memory_limit = 512M
   upload_max_filesize = 100M
   post_max_size = 100M
   max_execution_time = 600
   ```
5. Добавьте `C:\PHP` в PATH

### 2. Composer

1. Скачайте: https://getcomposer.org/Composer-Setup.exe
2. Запустите установщик
3. Укажите путь к PHP

### 3. Node.js

1. Скачайте: https://nodejs.org/
2. Установите LTS версию (18+)

### 4. Mosquitto MQTT

1. Скачайте: https://mosquitto.org/download/
2. Установите в `C:\Program Files\mosquitto\`
3. ✅ Отметьте "Install as Service"
4. Создайте `C:\Program Files\mosquitto\mosquitto.conf`:
   ```
   listener 1883 0.0.0.0
   allow_anonymous false
   password_file C:\Program Files\mosquitto\passwd
   log_dest file C:\Program Files\mosquitto\mosquitto.log
   log_type all
   persistence true
   persistence_location C:\Program Files\mosquitto\data
   ```
5. Создайте пользователя:
   ```cmd
   cd "C:\Program Files\mosquitto"
   mosquitto_passwd.exe -c passwd hydro_server
   ```
6. Перезапустите сервис:
   ```cmd
   net stop mosquitto
   net start mosquitto
   ```

### 5. NSSM (для Windows Services)

1. Скачайте: https://nssm.cc/download
2. Распакуйте в `C:\nssm\`

---

## ⚙️ Настройка Backend

### 1. Установка зависимостей

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
composer install --optimize-autoloader --no-dev
```

### 2. Конфигурация

```cmd
REM Копируем .env
copy .env.example .env

REM Генерируем ключ
php artisan key:generate

REM Настраиваем .env для production
```

Отредактируйте `.env`:
```env
APP_ENV=production
APP_DEBUG=false

DB_CONNECTION=sqlite
DB_DATABASE=

MQTT_HOST=127.0.0.1
MQTT_PORT=1883
MQTT_USERNAME=hydro_server
MQTT_PASSWORD=your_secure_password

TELEGRAM_ENABLED=true
TELEGRAM_BOT_TOKEN=your_bot_token
TELEGRAM_CHAT_ID=your_chat_id
```

### 3. База данных

```cmd
REM Создаем БД и выполняем миграции
php artisan migrate --force

REM Заполняем тестовыми данными (опционально)
php artisan db:seed --force
```

### 4. Оптимизация

```cmd
php artisan config:cache
php artisan route:cache
php artisan view:cache
```

---

## 🌐 Настройка Frontend

### 1. Установка

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\frontend
npm ci --production
```

### 2. Конфигурация

Создайте `.env`:
```env
VITE_API_URL=http://localhost:8000/api
```

### 3. Build

```cmd
npm run build
```

Build будет в `dist/`

### 4. Копирование в Backend public

```cmd
xcopy /E /I dist C:\esp\hydro\mesh\mesh_hydro\server\backend\public\app
```

---

## 🔧 Настройка служб Windows (NSSM)

### Backend API Service

```cmd
cd C:\nssm\win64

nssm.exe install HydroBackend ^
  "C:\PHP\php.exe" ^
  "C:\esp\hydro\mesh\mesh_hydro\server\backend\artisan" serve --host=0.0.0.0 --port=8000

nssm.exe set HydroBackend AppDirectory "C:\esp\hydro\mesh\mesh_hydro\server\backend"
nssm.exe set HydroBackend DisplayName "Hydro System - Backend API"
nssm.exe set HydroBackend Description "REST API for Mesh Hydro System"
nssm.exe set HydroBackend Start SERVICE_AUTO_START
nssm.exe set HydroBackend AppStdout "C:\HydroLogs\backend.log"
nssm.exe set HydroBackend AppStderr "C:\HydroLogs\backend-error.log"
nssm.exe set HydroBackend AppRotateFiles 1
nssm.exe set HydroBackend AppRotateBytes 10485760

mkdir C:\HydroLogs
nssm.exe start HydroBackend
```

### MQTT Listener Service

```cmd
nssm.exe install HydroMqttListener ^
  "C:\PHP\php.exe" ^
  "C:\esp\hydro\mesh\mesh_hydro\server\backend\artisan" mqtt:listen

nssm.exe set HydroMqttListener AppDirectory "C:\esp\hydro\mesh\mesh_hydro\server\backend"
nssm.exe set HydroMqttListener DisplayName "Hydro System - MQTT Listener"
nssm.exe set HydroMqttListener Start SERVICE_AUTO_START
nssm.exe set HydroMqttListener AppStdout "C:\HydroLogs\mqtt.log"
nssm.exe set HydroMqttListener AppStderr "C:\HydroLogs\mqtt-error.log"
nssm.exe set HydroMqttListener AppRotateFiles 1

nssm.exe start HydroMqttListener
```

### Laravel Scheduler Service

```cmd
nssm.exe install HydroScheduler ^
  "C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe" ^
  "-ExecutionPolicy Bypass -File C:\esp\hydro\mesh\mesh_hydro\server\backend\scheduler.ps1"

nssm.exe set HydroScheduler AppDirectory "C:\esp\hydro\mesh\mesh_hydro\server\backend"
nssm.exe set HydroScheduler DisplayName "Hydro System - Scheduler"
nssm.exe set HydroScheduler Start SERVICE_AUTO_START

nssm.exe start HydroScheduler
```

---

## 🛡️ Firewall Configuration

```cmd
REM API port (если нужен доступ извне)
netsh advfirewall firewall add rule name="Hydro API" dir=in action=allow protocol=TCP localport=8000

REM MQTT port (только локально)
netsh advfirewall firewall add rule name="Mosquitto MQTT" dir=in action=allow protocol=TCP localport=1883 remoteip=localsubnet
```

---

## 📊 Мониторинг

### Проверка служб

```cmd
REM Проверка всех служб
sc query Mosquitto
nssm.exe status HydroBackend
nssm.exe status HydroMqttListener
nssm.exe status HydroScheduler
```

### Просмотр логов

```cmd
REM Backend logs
type C:\HydroLogs\backend.log

REM MQTT logs
type C:\HydroLogs\mqtt.log

REM Laravel logs
type C:\esp\hydro\mesh\mesh_hydro\server\backend\storage\logs\laravel.log
```

### Health Check Script

```cmd
REM Создайте health-check.bat
curl http://127.0.0.1:8000/api/status
```

---

## 🔄 Backup

### Автоматический backup (Task Scheduler)

Создайте задачу в Task Scheduler:
- **Name:** Hydro System Backup
- **Trigger:** Daily at 03:00
- **Action:** Run `C:\esp\hydro\mesh\mesh_hydro\server\backend\backup.bat`

### Ручной backup

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
backup.bat
```

---

## 🔐 Безопасность

### 1. Измените пароли
- PostgreSQL/SQLite пароль
- MQTT пароль
- Telegram токен

### 2. Firewall правила
- Закройте ненужные порты
- Разрешите только локальную сеть

### 3. SSL/TLS (опционально)
- Для MQTT используйте порт 8883
- Для API используйте nginx с SSL

---

## 📝 Проверочный список

- [ ] PHP установлен и настроен
- [ ] Composer установлен
- [ ] Node.js установлен
- [ ] Mosquitto установлен и запущен
- [ ] NSSM установлен
- [ ] Backend зависимости установлены (composer install)
- [ ] Frontend собран (npm run build)
- [ ] .env файл настроен
- [ ] База данных создана и мигрирована
- [ ] Службы Windows установлены
- [ ] Firewall настроен
- [ ] Backup настроен
- [ ] Health check работает
- [ ] ROOT узел подключен к MQTT

---

## 🎯 Быстрый старт (одной командой)

```cmd
START_SYSTEM.bat
```

---

## 📚 Ссылки

- Backend README: `backend/README.md`
- Frontend README: `frontend/README.md`
- Windows Setup: `backend/WINDOWS_SETUP.md`
- Quick Start: `backend/QUICK_START.md`

---

**Версия:** 2.0.0 Production  
**Дата:** 2025-10-18  
**Платформа:** Windows 10/11, Windows Server 2016+

