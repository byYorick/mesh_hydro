@echo off
REM ========================================
REM MESH HYDRO V2 - DOCKER STATUS SCRIPT (Windows)
REM ========================================

setlocal enabledelayedexpansion

echo 📊 Статус Mesh Hydro System V2...

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
echo [INFO] Проверяем наличие docker-compose.yml...

if not exist "docker-compose.yml" (
    echo [ERROR] Файл docker-compose.yml не найден!
    pause
    exit /b 1
)

REM Статус контейнеров
echo [INFO] Проверяем статус контейнеров...
echo.
echo 📋 Статус контейнеров:
echo =====================
docker-compose ps

echo.
echo 📊 Детальная информация:
echo ========================

REM Проверка каждого сервиса
set services=postgres mosquitto backend mqtt_listener scheduler reverb frontend
for %%s in (%services%) do (
    docker-compose ps -q %%s >nul 2>&1
    if not errorlevel 1 (
        echo [SUCCESS] ✅ %%s: Запущен
    ) else (
        echo [ERROR] ❌ %%s: Не найден
    )
)

echo.
echo 🔍 Проверка доступности сервисов:
echo =================================

REM Проверка PostgreSQL
docker-compose exec -T postgres pg_isready -U hydro -d hydro_system >nul 2>&1
if not errorlevel 1 (
    echo [SUCCESS] ✅ PostgreSQL: Доступен
) else (
    echo [ERROR] ❌ PostgreSQL: Недоступен
)

REM Проверка Backend
curl -s http://localhost:8000/health >nul 2>&1
if not errorlevel 1 (
    echo [SUCCESS] ✅ Backend API: Доступен
) else (
    echo [WARNING] ⚠️ Backend API: Недоступен
)

REM Проверка Frontend
curl -s http://localhost:3000 >nul 2>&1
if not errorlevel 1 (
    echo [SUCCESS] ✅ Frontend: Доступен
) else (
    echo [WARNING] ⚠️ Frontend: Недоступен
)

REM Проверка MQTT
docker-compose exec -T mosquitto mosquitto_pub -h localhost -t test -m "test" >nul 2>&1
if not errorlevel 1 (
    echo [SUCCESS] ✅ MQTT Broker: Доступен
) else (
    echo [WARNING] ⚠️ MQTT Broker: Недоступен
)

REM Проверка WebSocket
curl -s http://localhost:8080 >nul 2>&1
if not errorlevel 1 (
    echo [SUCCESS] ✅ WebSocket: Доступен
) else (
    echo [WARNING] ⚠️ WebSocket: Недоступен
)

echo.
echo 📈 Использование ресурсов:
echo =========================
docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.NetIO}}\t{{.BlockIO}}"

echo.
echo 🔧 Полезные команды:
echo ===================
echo - Запустить: start.bat
echo - Остановить: stop.bat
echo - Перезапустить: docker-compose restart
echo - Логи: docker-compose logs -f
echo - Логи сервиса: docker-compose logs -f [service_name]
echo - Очистить все: stop.bat --clean
echo.
echo 📱 Доступные URL:
echo =================
echo - Frontend: http://localhost:3000
echo - Backend API: http://localhost:8000
echo - MQTT Broker: localhost:1883
echo - PostgreSQL: localhost:5432
echo - WebSocket: ws://localhost:8080
echo.
echo 🚀 Система готова к работе!

pause
