@echo off
echo Testing NODE pH pumps on GPIO 12, 13...
echo.

REM Инициализация ESP-IDF окружения
C:\Windows\system32\cmd.exe /k "C:\Espressif\idf_cmd_init.bat" esp-idf-1dcc643656a1439837fdf6ab63363005

echo.
echo ========================================
echo PUMP TEST COMMANDS
echo ========================================
echo.
echo 1. Manual pump test (2 seconds):
echo    {"command": "run_pump_manual", "params": {"pump_id": 0, "duration_sec": 2}}
echo    {"command": "run_pump_manual", "params": {"pump_id": 1, "duration_sec": 2}}
echo.
echo 2. Check pump status:
echo    {"command": "get_sensor_status", "params": {}}
echo.
echo 3. Emergency stop:
echo    {"command": "emergency_stop", "params": {}}
echo.
echo ========================================
echo GPIO CONFIGURATION:
echo ========================================
echo GPIO 12 = Pump 0 (pH UP)
echo GPIO 13 = Pump 1 (pH DOWN)
echo.
echo Connect LEDs to test:
echo GPIO 12 → LED (+ 220Ω resistor) → GND
echo GPIO 13 → LED (+ 220Ω resistor) → GND
echo.
echo ========================================
echo Starting monitor...
echo ========================================

idf.py monitor
