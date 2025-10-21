@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ПРОШИВКА NODE pH (ESP32-C3)
echo ════════════════════════════════════════════════════════
echo.
echo ⚠️  УКАЖИ COM ПОРТ В СТРОКЕ 14!
echo.
pause

set "IDF_INIT=C:\Espressif\idf_cmd_init.bat esp-idf-29323a3f5a0574597d6dbaa0af20c775"
set "COM_PORT=COM3"

cd /d %~dp0\..\node_ph

echo.
echo 🔌 Прошивка на порт %COM_PORT%...
echo.
C:\Windows\system32\cmd.exe /k "%IDF_INIT% && idf.py -p %COM_PORT% flash monitor"

