@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  ПРОШИВКА NODE PH_EC (COM9)
echo ════════════════════════════════════════════════════════
echo.

set "IDF_INIT=C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005"

cd /d %~dp0\..\node_ph_ec

C:\Windows\system32\cmd.exe /k "%IDF_INIT% && idf.py -p COM9 flash monitor"

