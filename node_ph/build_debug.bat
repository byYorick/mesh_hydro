@echo off
echo ========================================
echo BUILDING NODE pH WITH DEBUG LOGS
echo ========================================
echo.

REM Инициализация ESP-IDF окружения
C:\Windows\system32\cmd.exe /k "C:\Espressif\idf_cmd_init.bat" esp-idf-1dcc643656a1439837fdf6ab63363005

REM Очистка и сборка
idf.py clean
idf.py build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Debug logs added:
    echo - Mesh message parsing
    echo - Command processing
    echo - Pump controller execution
    echo.
    echo To flash: idf.py flash monitor
) else (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    echo Check errors above
)

pause
