@echo off
REM ========================================
REM Quick Setup Script for Laravel Backend
REM ========================================

echo.
echo ========================================
echo   Quick Setup - Mesh Hydro Backend
echo ========================================
echo.

cd /d "%~dp0"

REM Generate APP_KEY
echo Generating application key...
php artisan key:generate --force
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to generate key
    echo Make sure PHP is in PATH
    pause
    exit /b 1
)
echo [OK] Key generated
echo.

REM Clear caches
echo Clearing caches...
php artisan config:clear
php artisan route:clear
php artisan cache:clear
echo [OK] Caches cleared
echo.

echo ========================================
echo   Setup Complete!
echo ========================================
echo.
echo Backend is ready to start.
echo Run: php artisan serve
echo.
pause

