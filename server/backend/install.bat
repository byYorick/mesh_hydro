@echo off
REM ========================================
REM Mesh Hydro System - Quick Install Script
REM ========================================

echo.
echo ========================================
echo   Mesh Hydro System - Backend Setup
echo ========================================
echo.

REM Check if PHP is installed
where php >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] PHP not found!
    echo Please install PHP 8.2+ from https://windows.php.net/download/
    pause
    exit /b 1
)

echo [OK] PHP found
php -v
echo.

REM Check if Composer is installed
where composer >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Composer not found!
    echo Please install Composer from https://getcomposer.org/download/
    pause
    exit /b 1
)

echo [OK] Composer found
composer --version
echo.

REM Install dependencies
echo ========================================
echo Installing Composer dependencies...
echo ========================================
echo.
call composer install
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Composer install failed!
    pause
    exit /b 1
)
echo [OK] Dependencies installed
echo.

REM Copy .env file
if not exist .env (
    echo ========================================
    echo Creating .env file...
    echo ========================================
    echo.
    copy .env.example .env
    echo [OK] .env file created
    echo.
) else (
    echo [INFO] .env file already exists
    echo.
)

REM Generate app key
echo ========================================
echo Generating application key...
echo ========================================
echo.
php artisan key:generate
echo.

REM Check database connection
echo ========================================
echo Checking database connection...
echo ========================================
echo.
echo [INFO] Make sure PostgreSQL is running!
echo [INFO] Make sure you have created the database!
echo.
pause

REM Run migrations
echo ========================================
echo Running database migrations...
echo ========================================
echo.
php artisan migrate
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Migrations failed!
    echo Please check:
    echo  - PostgreSQL is running
    echo  - Database exists (hydro_system)
    echo  - .env settings are correct
    pause
    exit /b 1
)
echo [OK] Migrations completed
echo.

REM Success
echo ========================================
echo   Installation Complete!
echo ========================================
echo.
echo Next steps:
echo  1. Edit .env file with your settings
echo  2. Start API server: php artisan serve
echo  3. Start MQTT listener: php artisan mqtt:listen
echo.
echo For detailed instructions, see:
echo  - README.md
echo  - WINDOWS_SETUP.md
echo.
pause

