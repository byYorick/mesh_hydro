@echo off
echo ========================================
echo PUMP DEBUG TEST - NODE pH
echo ========================================
echo.

REM Инициализация ESP-IDF окружения
C:\Windows\system32\cmd.exe /k "C:\Espressif\idf_cmd_init.bat" esp-idf-1dcc643656a1439837fdf6ab63363005

echo.
echo ========================================
echo BUILDING WITH DEBUG LOGS...
echo ========================================

idf.py build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Now flashing and monitoring...
    echo.
    idf.py flash monitor
) else (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    echo Check errors above
    pause
)
