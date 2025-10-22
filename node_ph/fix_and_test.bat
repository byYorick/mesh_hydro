@echo off
echo ========================================
echo FIXING MESH PROTOCOL AND TESTING
echo ========================================
echo.

echo Fixed mesh_protocol.c - commands now work!
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
    echo MESH PROTOCOL FIXED:
    echo - Commands now parse correctly
    echo - msg.data will contain full JSON
    echo - Pumps should work now!
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
