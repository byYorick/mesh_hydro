@echo off
REM Создание символических ссылок на common компоненты
REM Запускать от администратора!

echo Creating symlinks for common components...

REM Для каждого node создаем symlink на common
cd ..\node_ph_ec
mklink /D common ..\common
if errorlevel 1 (
    echo Error: Run as Administrator!
    pause
    exit /b 1
)

cd ..\node_display
mklink /D common ..\common

cd ..\node_climate
mklink /D common ..\common

cd ..\node_relay
mklink /D common ..\common

cd ..\node_water
mklink /D common ..\common

cd ..\node_template
mklink /D common ..\common

cd ..\root_node
mklink /D common ..\common

cd ..\tools

echo Symlinks created successfully!
pause

