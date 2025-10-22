@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ПОЛНАЯ ОЧИСТКА И ПЕРЕЗАПУСК DOCKER
echo ════════════════════════════════════════════════════════
echo.
echo ⚠️  ВНИМАНИЕ: Этот скрипт удалит ВСЕ Docker данные!
echo ⚠️  Убедитесь, что у вас есть резервные копии важных данных.
echo.
set /p confirm="Продолжить? (y/N): "
if /i not "%confirm%"=="y" (
    echo Отменено.
    pause
    exit /b
)

cd /d %~dp0\..\server

echo.
echo [1/10] Остановка всех контейнеров...
docker-compose down

echo.
echo [2/10] Удаление всех контейнеров проекта...
docker-compose down --volumes --remove-orphans

echo.
echo [3/10] Удаление всех образов проекта...
docker-compose down --rmi all --volumes --remove-orphans

echo.
echo [4/10] Остановка всех контейнеров Docker...
docker stop $(docker ps -aq) 2>nul

echo.
echo [5/10] Удаление всех контейнеров...
docker rm $(docker ps -aq) 2>nul

echo.
echo [6/10] Удаление всех образов...
docker rmi $(docker images -q) 2>nul

echo.
echo [7/10] Очистка всех volumes...
docker volume prune -f

echo.
echo [8/10] Очистка всех сетей...
docker network prune -f

echo.
echo [9/10] Полная очистка системы Docker...
docker system prune -a -f --volumes

echo.
echo [10/10] Пересборка и запуск системы...
docker-compose build --no-cache --pull
docker-compose up -d

echo.
echo ⏳ Ждём 30 секунд для полного запуска...
timeout /t 30 /nobreak >nul

echo.
echo ════════════════════════════════════════════════════════
echo  СТАТУС КОНТЕЙНЕРОВ
echo ════════════════════════════════════════════════════════
docker-compose ps

echo.
echo ════════════════════════════════════════════════════════
echo  ПРОВЕРКА ЛОГОВ
echo ════════════════════════════════════════════════════════
echo.
echo 📋 Backend:
docker logs hydro_backend --tail 15

echo.
echo 📋 MQTT Listener:
docker logs hydro_mqtt_listener --tail 15

echo.
echo 📋 Reverb:
docker logs hydro_reverb --tail 15

echo.
echo 📋 Frontend:
docker logs hydro_frontend --tail 10

echo.
echo Сервер доступен на:
echo   Frontend: http://localhost:3000
echo   Backend API: http://localhost:8000
echo   WebSocket: ws://localhost:8080
echo   MQTT Broker: mqtt://192.168.1.100:1883
echo.
echo Для просмотра логов в реальном времени:
echo docker-compose logs -f
echo.
echo ════════════════════════════════════════════════════════
echo  ГОТОВО! Система полностью пересобрана.
echo ════════════════════════════════════════════════════════
echo.
pause
