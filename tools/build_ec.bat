@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  СБОРКА NODE EC (ESP32-C3)
echo ════════════════════════════════════════════════════════
echo.

set "IDF_INIT=C:\Espressif\idf_cmd_init.bat esp-idf-29323a3f5a0574597d6dbaa0af20c775"

cd /d %~dp0\..\node_ec

echo 🔨 Установка target ESP32-C3...
C:\Windows\system32\cmd.exe /c "%IDF_INIT% && idf.py set-target esp32c3"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ОШИБКА установки target!
    pause
    exit /b 1
)

echo.
echo 🔨 Сборка проекта...
C:\Windows\system32\cmd.exe /c "%IDF_INIT% && idf.py build"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ОШИБКА сборки!
    pause
    exit /b 1
)

echo.
echo ════════════════════════════════════════════════════════
echo  ✅ NODE EC СОБРАН УСПЕШНО!
echo ════════════════════════════════════════════════════════
echo.
echo Для прошивки используйте:
echo   tools\flash_ec.bat
echo.
pause

