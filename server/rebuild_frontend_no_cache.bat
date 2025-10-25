@echo off
echo ========================================
echo ПЕРЕСБОРКА ФРОНТЕНДА БЕЗ КЕШИРОВАНИЯ
echo ========================================

echo Остановка контейнеров...
docker-compose down

echo Удаление старых образов фронтенда...
docker rmi hydro_frontend 2>nul

echo Пересборка фронтенда без кеша...
docker-compose build --no-cache frontend

echo Запуск сервисов...
docker-compose up -d

echo ========================================
echo ГОТОВО! Фронтенд пересобран без кеширования
echo Доступен по адресу: http://localhost:3000
echo ========================================
pause
