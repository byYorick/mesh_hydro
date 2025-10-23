@echo off
echo 🚀 Запуск тестирования PID системы с моковыми данными pH
echo ================================================================

REM Проверяем, что мы в правильной директории
if not exist "..\node_ph" (
    echo ❌ Ошибка: Запустите скрипт из папки tools
    pause
    exit /b 1
)

echo 📁 Переходим в директорию node_ph...
cd ..\node_ph

echo.
echo 🔨 Сборка pH узла...
call build.bat
if errorlevel 1 (
    echo ❌ Ошибка сборки
    pause
    exit /b 1
)

echo.
echo ✅ Сборка завершена успешно!
echo.
echo 📋 Следующие шаги:
echo 1. Подключите ESP32-C3 к компьютеру
echo 2. Убедитесь, что порт COM доступен
echo 3. Запустите flash.bat для прошивки
echo 4. Запустите test_pid_system.py для симуляции данных
echo.

set /p choice="❓ Запустить прошивку сейчас? (y/n): "
if /i "%choice%"=="y" (
    echo.
    echo 🔌 Прошивка ESP32...
    call flash.bat
)

echo.
echo 🎯 Для запуска симуляции данных выполните:
echo    python test_pid_system.py
echo.
pause
