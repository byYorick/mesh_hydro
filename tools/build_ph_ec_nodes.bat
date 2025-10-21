@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  СБОРКА NODE pH и NODE EC (ESP32-C3)
echo ════════════════════════════════════════════════════════
echo.
echo ⚠️  ВНИМАНИЕ: Это займёт 5-10 минут!
echo.
pause

set "IDF_INIT=C:\Espressif\idf_cmd_init.bat esp-idf-29323a3f5a0574597d6dbaa0af20c775"
cd /d %~dp0\..

echo.
echo ════════════════════════════════════════════════════════
echo  1/2 СБОРКА NODE pH
echo ════════════════════════════════════════════════════════
cd node_ph
echo 🔨 Установка target ESP32-C3...
C:\Windows\system32\cmd.exe /c "%IDF_INIT% && idf.py set-target esp32c3"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ОШИБКА установки target!
    pause
    exit /b 1
)
echo 🔨 Сборка...
C:\Windows\system32\cmd.exe /c "%IDF_INIT% && idf.py build"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ОШИБКА сборки NODE pH!
    pause
    exit /b 1
)

echo.
echo ════════════════════════════════════════════════════════
echo  2/2 СБОРКА NODE EC
echo ════════════════════════════════════════════════════════
cd ..\node_ec
echo 🔨 Установка target ESP32-C3...
C:\Windows\system32\cmd.exe /c "%IDF_INIT% && idf.py set-target esp32c3"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ОШИБКА установки target!
    pause
    exit /b 1
)
echo 🔨 Сборка...
C:\Windows\system32\cmd.exe /c "%IDF_INIT% && idf.py build"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ОШИБКА сборки NODE EC!
    pause
    exit /b 1
)

cd ..

echo.
echo ════════════════════════════════════════════════════════
echo  ✅ ОБЕ НОДЫ СОБРАНЫ УСПЕШНО!
echo ════════════════════════════════════════════════════════
echo.
echo Для прошивки используйте:
echo   - tools\flash_ph.bat (COM порт нужно указать)
echo   - tools\flash_ec.bat (COM порт нужно указать)
echo.
pause

