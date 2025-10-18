@echo off
REM ========================================
REM Mesh Hydro System - Database Backup
REM ========================================

echo.
echo ========================================
echo   Database Backup Script
echo ========================================
echo.

REM Configuration
set BACKUP_DIR=C:\backups\hydro
set PG_BIN=C:\Program Files\PostgreSQL\15\bin
set DB_NAME=hydro_system
set DB_USER=hydro
set TIMESTAMP=%date:~-4,4%%date:~-7,2%%date:~-10,2%_%time:~0,2%%time:~3,2%%time:~6,2%
set TIMESTAMP=%TIMESTAMP: =0%

REM Create backup directory
if not exist "%BACKUP_DIR%" (
    mkdir "%BACKUP_DIR%"
    echo [OK] Created backup directory: %BACKUP_DIR%
)

REM Set password (you should set PGPASSWORD environment variable instead)
REM Or use .pgpass file

echo Creating backup...
echo Database: %DB_NAME%
echo Timestamp: %TIMESTAMP%
echo.

REM Run pg_dump
"%PG_BIN%\pg_dump.exe" -U %DB_USER% -F c -b -v -f "%BACKUP_DIR%\backup_%TIMESTAMP%.dump" %DB_NAME%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [OK] Backup created successfully!
    echo File: %BACKUP_DIR%\backup_%TIMESTAMP%.dump
    echo.
    
    REM Delete backups older than 30 days
    echo Cleaning old backups (older than 30 days)...
    forfiles /p "%BACKUP_DIR%" /m backup_*.dump /d -30 /c "cmd /c del @path" 2>nul
    echo [OK] Cleanup complete
) else (
    echo.
    echo [ERROR] Backup failed!
    echo Check:
    echo  - PostgreSQL is running
    echo  - Database exists
    echo  - User has permissions
)

echo.
pause

