@echo off
REM ========================================
REM Mesh Hydro System - Development Server
REM ========================================

echo.
echo ========================================
echo   Starting Hydro System Backend (DEV)
echo ========================================
echo.

REM Check .env
if not exist .env (
    echo [ERROR] .env file not found!
    echo Run install.bat first
    pause
    exit /b 1
)

echo Starting Laravel development server...
echo API will be available at: http://localhost:8000
echo.
echo Press Ctrl+C to stop
echo.

php artisan serve

