@echo off
REM Скрипт сборки для node_climate
REM Инициализирует окружение ESP-IDF и запускает сборку

echo Инициализация ESP-IDF окружения...

call "C:\Espressif\idf_cmd_init.bat" esp-idf-1dcc643656a1439837fdf6ab63363005

echo.
echo Проверка target ESP32...
idf.py set-target esp32

echo.
echo Запуск сборки проекта...
idf.py build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Сборка завершена успешно!
) else (
    echo.
    echo Ошибка при сборке!
    exit /b %ERRORLEVEL%
)

