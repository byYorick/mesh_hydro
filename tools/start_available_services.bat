@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ЗАПУСК ДОСТУПНЫХ СЕРВИСОВ
echo ════════════════════════════════════════════════════════
echo.

cd /d %~dp0\..\server

echo 🚀 Запуск PostgreSQL и Mosquitto...
docker-compose up -d postgres mosquitto

echo.
echo 🌐 Запуск WebSocket сервера...
docker run -d --name hydro_websocket --network server_hydro_network -p 6001:6001 server-websocket:latest

echo.
echo ⏳ Ждём 10 секунд для полного запуска...
timeout /t 10 /nobreak >nul

echo.
echo ════════════════════════════════════════════════════════
echo  СТАТУС КОНТЕЙНЕРОВ
echo ════════════════════════════════════════════════════════
docker ps

echo.
echo ════════════════════════════════════════════════════════
echo  ДОСТУПНЫЕ СЕРВИСЫ
echo ════════════════════════════════════════════════════════
echo.
echo ✅ PostgreSQL: localhost:5432
echo ✅ MQTT Broker: localhost:1883
echo ✅ WebSocket: localhost:6001
echo.
echo ⚠️  Backend и Frontend недоступны из-за проблем с Docker Hub
echo    (ошибка 503 Service Unavailable)
echo.
echo 💡 Для полного запуска нужно:
echo    1. Дождаться восстановления Docker Hub
echo    2. Или установить PHP и Node.js локально
echo.
pause
