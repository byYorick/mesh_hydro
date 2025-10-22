@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ПРОШИВКА NODE EC (ESP32-C3)
echo ════════════════════════════════════════════════════════
echo.
echo 📌 Порт: COM11 (измени если нужно)
echo 📦 Компонент: EC Manager + Adaptive PID
echo.

cd /d %~dp0\..\node_ec

echo ⏳ Сборка прошивки...
C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005
call idf.py build

if %errorlevel% neq 0 (
    echo.
    echo ❌ Ошибка сборки!
    pause
    exit /b 1
)

echo.
echo ✅ Сборка завершена успешно!
echo.
echo ⏳ Прошивка на COM11...
call idf.py -p COM11 flash monitor

pause
