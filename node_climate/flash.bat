@echo off
REM Скрипт прошивки для node_climate
REM Автоматически находит COM порт или использует указанный

echo ========================================
echo NODE Climate - Прошивка
echo ========================================

call "C:\Espressif\idf_cmd_init.bat" esp-idf-1dcc643656a1439837fdf6ab63363005

echo.
if "%1"=="" (
    echo Прошивка с автоопределением порта...
    idf.py flash monitor
) else (
    echo Прошивка на порт %1...
    idf.py -p %1 flash monitor
)

