@echo off
REM ========================================
REM MESH HYDRO V2 - DOCKER START SCRIPT (Windows)
REM ========================================

setlocal enabledelayedexpansion

echo 🚀 Запускаем Mesh Hydro System V2...

REM Проверка Docker
docker --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Docker не установлен. Установите Docker Desktop и попробуйте снова.
    pause
    exit /b 1
)

docker-compose --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Docker Compose не установлен. Установите Docker Desktop и попробуйте снова.
    pause
    exit /b 1
)

echo [INFO] Docker и Docker Compose найдены

REM Проверка файлов
echo [INFO] Проверяем наличие необходимых файлов...

if not exist "docker-compose.yml" (
    echo [ERROR] Файл docker-compose.yml не найден!
    pause
    exit /b 1
)

REM Создание .env файла если не существует
if not exist ".env" (
    echo [WARNING] .env файл не найден. Создаем базовый .env файл...
    (
        echo # Mesh Hydro System V2 Environment
        echo APP_NAME="Mesh Hydro System"
        echo APP_ENV=production
        echo APP_KEY=base64:hydro_app_key_2025
        echo APP_DEBUG=false
        echo APP_URL=http://localhost
        echo.
        echo LOG_CHANNEL=stack
        echo LOG_DEPRECATIONS_CHANNEL=null
        echo LOG_LEVEL=info
        echo.
        echo DB_CONNECTION=pgsql
        echo DB_HOST=postgres
        echo DB_PORT=5432
        echo DB_DATABASE=hydro_system
        echo DB_USERNAME=hydro
        echo DB_PASSWORD=hydro_secure_pass_2025
        echo.
        echo BROADCAST_DRIVER=reverb
        echo CACHE_DRIVER=file
        echo FILESYSTEM_DISK=local
        echo QUEUE_CONNECTION=sync
        echo SESSION_DRIVER=file
        echo SESSION_LIFETIME=120
        echo.
        echo MEMCACHED_HOST=127.0.0.1
        echo.
        echo REDIS_HOST=127.0.0.1
        echo REDIS_PASSWORD=null
        echo REDIS_PORT=6379
        echo.
        echo MAIL_MAILER=smtp
        echo MAIL_HOST=mailpit
        echo MAIL_PORT=1025
        echo MAIL_USERNAME=null
        echo MAIL_PASSWORD=null
        echo MAIL_ENCRYPTION=null
        echo MAIL_FROM_ADDRESS="hello@example.com"
        echo MAIL_FROM_NAME="${APP_NAME}"
        echo.
        echo AWS_ACCESS_KEY_ID=
        echo AWS_SECRET_ACCESS_KEY=
        echo AWS_DEFAULT_REGION=us-east-1
        echo AWS_BUCKET=
        echo AWS_USE_PATH_STYLE_ENDPOINT=false
        echo.
        echo PUSHER_APP_ID=hydro-app
        echo PUSHER_APP_KEY=hydro-app-key
        echo PUSHER_APP_SECRET=hydro-app-secret
        echo PUSHER_HOST=reverb
        echo PUSHER_PORT=8080
        echo PUSHER_SCHEME=http
        echo PUSHER_APP_CLUSTER=mt1
        echo.
        echo VITE_PUSHER_APP_KEY="${PUSHER_APP_KEY}"
        echo VITE_PUSHER_HOST="${PUSHER_HOST}"
        echo VITE_PUSHER_PORT="${PUSHER_PORT}"
        echo VITE_PUSHER_SCHEME="${PUSHER_SCHEME}"
        echo VITE_PUSHER_APP_CLUSTER="${PUSHER_APP_CLUSTER}"
        echo.
        echo # MQTT Settings
        echo MQTT_HOST=mosquitto
        echo MQTT_PORT=1883
        echo MQTT_USERNAME=
        echo MQTT_PASSWORD=
        echo MQTT_CLIENT_ID=hydro_backend
        echo MQTT_TOPIC_PREFIX=hydro
        echo.
        echo # Telegram Bot
        echo TELEGRAM_BOT_TOKEN=
        echo TELEGRAM_CHAT_ID=
        echo.
        echo # Node Settings
        echo NODE_OFFLINE_TIMEOUT=300
        echo NODE_HEARTBEAT_INTERVAL=60
    ) > .env
    echo [SUCCESS] .env файл создан
) else (
    echo [INFO] .env файл уже существует
)

REM Создание директорий для volumes
echo [INFO] Создаем необходимые директории...
if not exist "mosquitto\config" mkdir mosquitto\config
if not exist "mosquitto\data" mkdir mosquitto\data
if not exist "mosquitto\log" mkdir mosquitto\log
if not exist "nginx\ssl" mkdir nginx\ssl

REM Создание конфигурации Mosquitto
if not exist "mosquitto\config\mosquitto.conf" (
    echo [INFO] Создаем конфигурацию Mosquitto...
    (
        echo # Mosquitto Configuration for Mesh Hydro System
        echo listener 1883
        echo allow_anonymous true
        echo persistence true
        echo persistence_location /mosquitto/data/
        echo log_dest file /mosquitto/log/mosquitto.log
        echo log_type error
        echo log_type warning
        echo log_type notice
        echo log_type information
        echo connection_messages true
        echo log_timestamp true
    ) > mosquitto\config\mosquitto.conf
    echo [SUCCESS] Конфигурация Mosquitto создана
)

REM Остановка существующих контейнеров
echo [INFO] Останавливаем существующие контейнеры...
docker-compose down --remove-orphans

REM Запуск сервисов
echo [INFO] Запускаем сервисы...

REM Запуск базовых сервисов
echo [INFO] Запускаем PostgreSQL и Mosquitto...
docker-compose up -d postgres mosquitto

REM Ожидание готовности PostgreSQL
echo [INFO] Ожидаем готовности PostgreSQL...
timeout 60 >nul 2>&1
ping -n 30 127.0.0.1 >nul 2>&1

REM Запуск backend
echo [INFO] Запускаем backend...
docker-compose up -d backend

REM Ожидание готовности backend
echo [INFO] Ожидаем готовности backend...
timeout 60 >nul 2>&1
ping -n 30 127.0.0.1 >nul 2>&1

REM Запуск остальных сервисов
echo [INFO] Запускаем остальные сервисы...
docker-compose up -d mqtt_listener scheduler reverb frontend

REM Ожидание готовности frontend
echo [INFO] Ожидаем готовности frontend...
timeout 60 >nul 2>&1
ping -n 30 127.0.0.1 >nul 2>&1

REM Проверка статуса
echo [INFO] Проверяем статус сервисов...
docker-compose ps

REM Финальная проверка
echo [INFO] Проверяем доступность сервисов...

REM Проверка PostgreSQL
docker-compose exec -T postgres pg_isready -U hydro -d hydro_system >nul 2>&1
if not errorlevel 1 (
    echo [SUCCESS] ✅ PostgreSQL доступен
) else (
    echo [ERROR] ❌ PostgreSQL недоступен
)

REM Проверка Backend
curl -s http://localhost:8000/health >nul 2>&1
if not errorlevel 1 (
    echo [SUCCESS] ✅ Backend API доступен
) else (
    echo [WARNING] ⚠️ Backend API недоступен
)

REM Проверка Frontend
curl -s http://localhost:3000 >nul 2>&1
if not errorlevel 1 (
    echo [SUCCESS] ✅ Frontend доступен
) else (
    echo [WARNING] ⚠️ Frontend недоступен
)

REM Проверка MQTT
docker-compose exec -T mosquitto mosquitto_pub -h localhost -t test -m "test" >nul 2>&1
if not errorlevel 1 (
    echo [SUCCESS] ✅ MQTT Broker доступен
) else (
    echo [WARNING] ⚠️ MQTT Broker недоступен
)

echo [SUCCESS] Система запущена!

echo.
echo 🎉 Mesh Hydro System V2 запущена!
echo.
echo 📊 Доступные сервисы:
echo - Frontend: http://localhost:3000
echo - Backend API: http://localhost:8000
echo - MQTT Broker: localhost:1883
echo - PostgreSQL: localhost:5432
echo - WebSocket: ws://localhost:8080
echo.
echo 🔧 Полезные команды:
echo - Статус: docker-compose ps
echo - Логи: docker-compose logs -f
echo - Остановить: docker-compose down
echo - Перезапустить: docker-compose restart
echo.
echo 📱 Откройте браузер и перейдите на http://localhost:3000
echo.
echo 🚀 Система готова к работе!

pause
