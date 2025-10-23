@echo off
REM ========================================
REM MESH HYDRO V2 - DOCKER STOP SCRIPT (Windows)
REM ========================================

setlocal enabledelayedexpansion

echo 🛑 Останавливаем Mesh Hydro System V2...

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

REM Остановка сервисов
echo [INFO] Останавливаем все сервисы...
docker-compose down

REM Очистка (опционально)
if "%1"=="--clean" (
    echo [INFO] Очищаем volumes и orphaned контейнеры...
    docker-compose down -v --remove-orphans
    
    echo [INFO] Очищаем неиспользуемые образы...
    docker system prune -f
    
    echo [SUCCESS] Система полностью очищена
) else (
    echo [SUCCESS] Сервисы остановлены (данные сохранены)
)

REM Проверка статуса
echo [INFO] Проверяем статус контейнеров...
docker-compose ps

echo [SUCCESS] Система остановлена!

echo.
echo 🛑 Mesh Hydro System V2 остановлена!
echo.
echo 📊 Статус:
echo - Все контейнеры остановлены
echo - Данные сохранены в volumes
echo - Система готова к перезапуску
echo.
echo 🔧 Полезные команды:
echo - Запустить: start.bat
echo - Полная очистка: stop.bat --clean
echo - Статус: docker-compose ps
echo - Логи: docker-compose logs -f
echo.
echo 🚀 Для запуска выполните: start.bat

pause
