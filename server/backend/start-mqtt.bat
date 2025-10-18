@echo off
REM ========================================
REM Mesh Hydro System - MQTT Listener
REM ========================================

echo.
echo ========================================
echo   Starting MQTT Listener
echo ========================================
echo.

REM Check .env
if not exist .env (
    echo [ERROR] .env file not found!
    echo Run install.bat first
    pause
    exit /b 1
)

echo Starting MQTT Listener...
echo Connecting to Mosquitto broker...
echo.
echo Press Ctrl+C to stop
echo.

php artisan mqtt:listen --reconnect-delay=5

