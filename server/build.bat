@echo off
REM ========================================
REM MESH HYDRO V2 - DOCKER BUILD SCRIPT (Windows)
REM ========================================

setlocal enabledelayedexpansion

echo 🚀 Начинаем сборку Mesh Hydro System V2...

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

if not exist "backend\Dockerfile" (
    echo [ERROR] Файл backend\Dockerfile не найден!
    pause
    exit /b 1
)

if not exist "frontend\Dockerfile" (
    echo [ERROR] Файл frontend\Dockerfile не найден!
    pause
    exit /b 1
)

if not exist "nginx\conf.d\production.conf" (
    echo [ERROR] Файл nginx\conf.d\production.conf не найден!
    pause
    exit /b 1
)

echo [SUCCESS] Все необходимые файлы найдены

REM Очистка старых образов
echo [INFO] Очищаем старые образы...
docker system prune -f

REM Сборка образов
echo [INFO] Собираем Docker образы...

echo [INFO] Собираем backend образ...
docker-compose build backend --no-cache
if errorlevel 1 (
    echo [ERROR] Ошибка сборки backend образа!
    pause
    exit /b 1
)

echo [INFO] Собираем frontend образ...
docker-compose build frontend --no-cache
if errorlevel 1 (
    echo [ERROR] Ошибка сборки frontend образа!
    pause
    exit /b 1
)

echo [INFO] Собираем mqtt_listener образ...
docker-compose build mqtt_listener --no-cache
if errorlevel 1 (
    echo [ERROR] Ошибка сборки mqtt_listener образа!
    pause
    exit /b 1
)

echo [INFO] Собираем scheduler образ...
docker-compose build scheduler --no-cache
if errorlevel 1 (
    echo [ERROR] Ошибка сборки scheduler образа!
    pause
    exit /b 1
)

echo [INFO] Собираем reverb образ...
docker-compose build reverb --no-cache
if errorlevel 1 (
    echo [ERROR] Ошибка сборки reverb образа!
    pause
    exit /b 1
)

echo [SUCCESS] Все образы успешно собраны!

REM Проверка образов
echo [INFO] Проверяем созданные образы...
docker images | findstr hydro

REM Создание .env файла если не существует
if not exist ".env" (
    echo [INFO] Создаем .env файл...
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

REM Финальная проверка
echo [INFO] Проверяем готовность к запуску...

REM Проверка портов (упрощенная версия для Windows)
netstat -an | findstr ":80 " >nul 2>&1
if not errorlevel 1 (
    echo [WARNING] Порт 80 уже используется. Убедитесь, что это не конфликтует с другими сервисами.
)

netstat -an | findstr ":8000 " >nul 2>&1
if not errorlevel 1 (
    echo [WARNING] Порт 8000 уже используется. Убедитесь, что это не конфликтует с другими сервисами.
)

netstat -an | findstr ":1883 " >nul 2>&1
if not errorlevel 1 (
    echo [WARNING] Порт 1883 уже используется. Убедитесь, что это не конфликтует с другими сервисами.
)

echo [SUCCESS] Система готова к запуску!

echo.
echo 🎉 Сборка завершена успешно!
echo.
echo 📋 Следующие шаги:
echo 1. Запустите систему: docker-compose up -d
echo 2. Проверьте статус: docker-compose ps
echo 3. Просмотрите логи: docker-compose logs -f
echo 4. Откройте браузер: http://localhost
echo.
echo 🔧 Полезные команды:
echo - Остановить: docker-compose down
echo - Перезапустить: docker-compose restart
echo - Очистить все: docker-compose down -v --remove-orphans
echo.
echo 📊 Мониторинг:
echo - Логи всех сервисов: docker-compose logs -f
echo - Логи конкретного сервиса: docker-compose logs -f backend
echo - Статус контейнеров: docker-compose ps
echo.
echo 🚀 Готово к запуску!

pause
