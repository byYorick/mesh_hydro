@echo off
REM ========================================
REM Complete Backend Setup Script
REM ========================================

echo.
echo ========================================
echo   Complete Laravel Backend Setup
echo ========================================
echo.

cd /d "%~dp0"

echo [1/5] Regenerating Composer autoload...
composer dump-autoload
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Composer dump-autoload failed
    pause
    exit /b 1
)
echo [OK] Autoload regenerated
echo.

echo [2/5] Generating application key...
php artisan key:generate --force
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Key generation failed
    pause
    exit /b 1
)
echo [OK] Key generated
echo.

echo [3/5] Clearing all caches...
php artisan config:clear
php artisan route:clear  
php artisan cache:clear
php artisan view:clear
echo [OK] Caches cleared
echo.

echo [4/5] Testing API routes...
php artisan route:list --path=api
echo.

echo [5/5] Checking configuration...
php artisan config:cache
echo [OK] Config cached
echo.

echo ========================================
echo   Setup Complete!
echo ========================================
echo.
echo Backend is ready to start!
echo.
echo Next steps:
echo   1. Review .env file (database settings)
echo   2. Run: php artisan serve
echo   3. Open: http://localhost:8000/api/health
echo.
pause

