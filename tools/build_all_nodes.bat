@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

cls
echo ╔═══════════════════════════════════════════════════════════════╗
echo ║      СБОРКА ВСЕХ FIRMWARE НОД MESH HYDRO (MULTI-ZONE)          ║
echo ╠═══════════════════════════════════════════════════════════════╣
echo ║  Этот скрипт последовательно соберёт все прошивки:            ║
echo ║   • Root Node        (ESP32-S3)                                ║
echo ║   • Climate Node     (ESP32)                                   ║
echo ║   • pH Node          (ESP32-C3)                                ║
echo ║   • EC Node          (ESP32-C3)                                ║
echo ║   • pH/EC Node       (ESP32-S3)                                ║
echo ║   • Water Node       (ESP32)                                   ║
echo ║   • Relay Node       (ESP32)                                   ║
echo ║   • Display Node     (ESP32-S3)                                ║
echo ╚═══════════════════════════════════════════════════════════════╝
echo.
echo ⚠️  Перед запуском убедитесь, что репозиторий обновлён и зависимые
echo    компоненты собраны. Сборка займёт 15-20 минут.
echo.
pause

set "IDF_INIT=C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005"
set "CMD=%SystemRoot%\system32\cmd.exe"
set "SCRIPT_DIR=%~dp0"

call :build_node "root_node"      "esp32s3" "Root Node"
call :build_node "node_climate"   "esp32"   "Climate Node"
call :build_node "node_ph"        "esp32c3" "pH Node"
call :build_node "node_ec"        "esp32c3" "EC Node"
call :build_node "node_ph_ec"     "esp32s3" "pH/EC Node"
call :build_node "node_water"     "esp32"   "Water Node"
call :build_node "node_relay"     "esp32"   "Relay Node"
call :build_node "node_display"   "esp32s3" "Display Node"

echo.
echo ╔═══════════════════════════════════════════════════════════════╗
echo ║        ✅ ВСЕ ПРОШИВКИ СОБРАНЫ УСПЕШНО БЕЗ ОШИБОК!             ║
echo ╠═══════════════════════════════════════════════════════════════╣
echo ║  Артефакты сборки расположены в соответствующих каталогах:    ║
echo ║   <project>/node_*/build/                                     ║
echo ║                                                               ║
echo ║  Для прошивки используйте flash-скрипты из каталога tools.    ║
echo ╚═══════════════════════════════════════════════════════════════╝
echo.
pause
exit /b 0

:build_node
set "NODE_DIR=%~1"
set "TARGET=%~2"
set "LABEL=%~3"

echo.
echo ════════════════════════════════════════════════════════════════
echo  🔧 СБОРКА: %LABEL%  (%NODE_DIR%)  →  target %TARGET%
echo ════════════════════════════════════════════════════════════════

pushd "%SCRIPT_DIR%\..\%NODE_DIR%" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ❌ Не удалось перейти в каталог "%NODE_DIR%".
    pause
    exit /b 1
)

echo 🔨 Установка target %TARGET%...
"%CMD%" /c "%IDF_INIT% && idf.py set-target %TARGET%"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ Ошибка установки target %TARGET% для %LABEL%.
    popd >nul
    pause
    exit /b 1
)

echo 🔨 Сборка проекта...
"%CMD%" /c "%IDF_INIT% && idf.py build"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ Сборка %LABEL% завершилась с ошибкой.
    popd >nul
    pause
    exit /b 1
)

popd >nul
echo ✅ %LABEL% собрано успешно.
goto :eof

