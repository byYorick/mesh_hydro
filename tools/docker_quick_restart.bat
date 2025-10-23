@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  БЫСТРЫЙ ПЕРЕЗАПУСК DOCKER С ОЧИСТКОЙ КЕША
echo ════════════════════════════════════════════════════════
echo.

cd /d %~dp0\..\server

echo [1/5] Остановка контейнеров...
docker-compose down

echo.
echo [2/5] Очистка кеша сборки...
docker builder prune -f

echo.
echo [3/5] Пересборка без кеша...
docker-compose build --no-cache

echo.
echo [4/5] Запуск системы...
docker-compose up -d

echo.
echo [5/5] Ожидание запуска (15 секунд)...
timeout /t 15 /nobreak >nul

echo.
echo ════════════════════════════════════════════════════════
echo  СТАТУС КОНТЕЙНЕРОВ
echo ════════════════════════════════════════════════════════
docker-compose ps

echo.
echo Сервер доступен на:
echo   Frontend: http://localhost:3000
echo   Backend API: http://localhost:8000
echo   WebSocket: ws://localhost:8080
echo.
pause
