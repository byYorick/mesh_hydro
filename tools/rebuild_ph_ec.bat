@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ПЕРЕСБОРКА NODE pH и NODE EC
echo ════════════════════════════════════════════════════════
echo.
echo 🔧 Полная пересборка с адаптивным PID
echo.

echo ════════════════════════════════════════════════════════
echo  1/2: Сборка NODE pH
echo ════════════════════════════════════════════════════════

cd /d %~dp0\..\node_ph

C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005
call idf.py fullclean
call idf.py build

if %errorlevel% neq 0 (
    echo ❌ Ошибка сборки NODE pH!
    pause
    exit /b 1
)

echo ✅ NODE pH собран успешно!
echo.

echo ════════════════════════════════════════════════════════
echo  2/2: Сборка NODE EC
echo ════════════════════════════════════════════════════════

cd /d %~dp0\..\node_ec

call idf.py fullclean
call idf.py build

if %errorlevel% neq 0 (
    echo ❌ Ошибка сборки NODE EC!
    pause
    exit /b 1
)

echo.
echo ════════════════════════════════════════════════════════
echo  ✅ ВСЕ СОБРАНО УСПЕШНО!
echo ════════════════════════════════════════════════════════
echo.
echo Готово к прошивке:
echo   tools\flash_ph.bat  - NODE pH (COM8)
echo   tools\flash_ec.bat  - NODE EC (COM11)
echo.
pause

