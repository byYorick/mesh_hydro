@echo off
REM Прошивка всех узлов Mesh Hydro System
REM Windows версия

echo ===================================
echo Mesh Hydro System - Flash All Nodes
echo ===================================
echo.

echo [1/6] Flashing ROOT NODE (ESP32-S3 #1)...
cd ..\root_node
idf.py -p COM3 flash
if errorlevel 1 goto :error
echo ROOT NODE flashed successfully!
echo.

echo [2/6] Flashing NODE Display (ESP32-S3 #2)...
cd ..\node_display
idf.py -p COM4 flash
if errorlevel 1 goto :error
echo NODE Display flashed successfully!
echo.

echo [3/6] Flashing NODE pH/EC (ESP32-S3 #3)...
cd ..\node_ph_ec
idf.py -p COM5 flash
if errorlevel 1 goto :error
echo NODE pH/EC flashed successfully!
echo.

echo [4/6] Flashing NODE Climate (ESP32)...
cd ..\node_climate
idf.py -p COM6 flash
if errorlevel 1 goto :error
echo NODE Climate flashed successfully!
echo.

echo [5/6] Flashing NODE Relay (ESP32)...
cd ..\node_relay
idf.py -p COM7 flash
if errorlevel 1 goto :error
echo NODE Relay flashed successfully!
echo.

echo [6/6] Flashing NODE Water (ESP32-C3)...
cd ..\node_water
idf.py -p COM8 flash
if errorlevel 1 goto :error
echo NODE Water flashed successfully!
echo.

echo ===================================
echo All nodes flashed successfully! ✓
echo ===================================
cd ..\tools
pause
exit /b 0

:error
echo.
echo ===================================
echo Error flashing node! ✗
echo ===================================
cd ..\tools
pause
exit /b 1

