@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ПЕРЕЗАПУСК DOCKER БЕЗ КЕША
echo ════════════════════════════════════════════════════════
echo.

cd /d %~dp0\..\server

echo [1/8] Остановка всех контейнеров...
docker-compose down

echo.
echo [2/8] Удаление всех образов проекта...
docker-compose down --rmi all

echo.
echo [3/8] Очистка неиспользуемых образов...
docker image prune -f

echo.
echo [4/8] Очистка кеша сборки Docker...
docker builder prune -f

echo.
echo [5/8] Очистка неиспользуемых контейнеров...
docker container prune -f

echo.
echo [6/8] Очистка неиспользуемых сетей...
docker network prune -f

echo.
echo [7/8] Пересборка всех сервисов без кеша...
docker-compose build --no-cache

echo.
echo [8/8] Запуск системы...
docker-compose up -d

echo.
echo ⏳ Ждём 20 секунд для полного запуска...
timeout /t 20 /nobreak >nul

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
docker logs hydro_backend --tail 10

echo.
echo 📋 MQTT Listener:
docker logs hydro_mqtt_listener --tail 10

echo.
echo 📋 Reverb:
docker logs hydro_reverb --tail 10

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
pause
