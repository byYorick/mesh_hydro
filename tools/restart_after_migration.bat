@echo off
REM ========================================
REM Перезапуск после миграции на Laravel 11
REM ========================================

echo.
echo ========================================
echo ПЕРЕЗАПУСК ПОСЛЕ МИГРАЦИИ LARAVEL 11
echo ========================================
echo.

cd /d "%~dp0\..\server"

echo [1/6] Остановка контейнеров...
docker-compose down

echo.
echo [2/6] Пересборка backend, reverb, mqtt_listener...
docker-compose build backend reverb mqtt_listener

echo.
echo [3/6] Запуск системы...
docker-compose up -d

echo.
echo [4/6] Ожидание запуска backend (10 секунд)...
timeout /t 10 /nobreak

echo.
echo [5/6] Очистка кэша Laravel...
docker exec hydro_backend composer dump-autoload
docker exec hydro_backend php artisan config:clear
docker exec hydro_backend php artisan cache:clear
docker exec hydro_backend php artisan route:cache

echo.
echo [6/6] Пересборка frontend...
docker-compose build frontend
docker-compose up -d frontend

echo.
echo ========================================
echo ПРОВЕРКА СТАТУСА
echo ========================================
echo.

docker-compose ps

echo.
echo ========================================
echo ЛОГИ BACKEND
echo ========================================
docker logs hydro_backend --tail 30

echo.
echo ========================================
echo ЛОГИ REVERB
echo ========================================
docker logs hydro_reverb --tail 20

echo.
echo ========================================
echo ГОТОВО!
echo ========================================
echo.
echo Открой браузер: http://localhost:3000
echo API: http://localhost:8000/api/nodes
echo Health: http://localhost:8000/up
echo.
echo Для просмотра логов в реальном времени:
echo docker-compose logs -f
echo.

pause

