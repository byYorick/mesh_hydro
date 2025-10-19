@echo off
chcp 65001 >nul
cls
echo ════════════════════════════════════════════════════════
echo  LIVE ЛОГИ MQTT LISTENER
echo ════════════════════════════════════════════════════════
echo.
echo Нажмите Ctrl+C для выхода
echo.

docker logs hydro_mqtt_listener -f --tail 20

