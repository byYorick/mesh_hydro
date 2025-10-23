@echo off
echo Building NODE pH with pH sensor fixes...

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
    echo pH sensor fixes applied:
    echo - Added pH data validation (0.0-14.0 range)
    echo - Enhanced I2C error logging
    echo - Auto-fallback to mock mode on errors
    echo - Added mock mode control commands
    echo.
    echo To flash: flash.bat
    echo To monitor: idf.py monitor
) else (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    echo Check errors above
)

pause
