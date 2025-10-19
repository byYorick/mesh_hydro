@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ПОЛНАЯ ПЕРЕСБОРКА ВСЕХ УЗЛОВ
echo ════════════════════════════════════════════════════════
echo.
echo ⚠️  ВНИМАНИЕ: Это займёт 15-20 минут!
echo.
pause

set "IDF_INIT=C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005"
cd /d %~dp0\..

echo.
echo ════════════════════════════════════════════════════════
echo  1/3 ПЕРЕСБОРКА ROOT NODE
echo ════════════════════════════════════════════════════════
cd root_node
C:\Windows\system32\cmd.exe /c "%IDF_INIT% && idf.py fullclean && idf.py build"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ОШИБКА сборки ROOT!
    pause
    exit /b 1
)

echo.
echo ════════════════════════════════════════════════════════
echo  2/3 ПЕРЕСБОРКА NODE CLIMATE
echo ════════════════════════════════════════════════════════
cd ..\node_climate
C:\Windows\system32\cmd.exe /c "%IDF_INIT% && idf.py fullclean && idf.py build"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ОШИБКА сборки NODE CLIMATE!
    pause
    exit /b 1
)

echo.
echo ════════════════════════════════════════════════════════
echo  3/3 ПЕРЕСБОРКА NODE PH_EC
echo ════════════════════════════════════════════════════════
cd ..\node_ph_ec
C:\Windows\system32\cmd.exe /c "%IDF_INIT% && idf.py fullclean && idf.py build"
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ОШИБКА сборки NODE PH_EC!
    pause
    exit /b 1
)

cd ..

echo.
echo ════════════════════════════════════════════════════════
echo  ✅ ВСЕ УЗЛЫ ПЕРЕСОБРАНЫ УСПЕШНО!
echo ════════════════════════════════════════════════════════
echo.
echo Теперь можно прошивать:
echo   - tools\flash_root.bat
echo   - tools\flash_climate.bat
echo   - tools\flash_ph_ec.bat
echo.
pause

