@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ПРОШИВКА ROOT NODE (COM7)
echo ════════════════════════════════════════════════════════
echo.

set "IDF_INIT=C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005"

cd /d %~dp0\..\root_node

C:\Windows\system32\cmd.exe /k "%IDF_INIT% && idf.py -p COM7 flash monitor"

