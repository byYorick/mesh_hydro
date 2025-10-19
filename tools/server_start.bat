@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ЗАПУСК BACKEND + MQTT + MOSQUITTO
echo ════════════════════════════════════════════════════════
echo.

cd /d %~dp0\..\server

docker-compose up -d

echo.
echo ⏳ Ждём 15 секунд для полного запуска...
timeout /t 15 /nobreak >nul

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
echo 📋 MQTT Listener:
docker logs hydro_mqtt_listener --tail 10

echo.
echo Сервер доступен на:
echo   Frontend: http://localhost:3000
echo   Backend API: http://localhost:8000
echo   MQTT Broker: mqtt://192.168.1.100:1883
echo.
pause

