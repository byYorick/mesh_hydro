@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ОСТАНОВКА ВСЕХ КОНТЕЙНЕРОВ
echo ════════════════════════════════════════════════════════
echo.

cd /d %~dp0\..\server

docker-compose down

echo.
echo ✅ Все контейнеры остановлены
echo.
pause

