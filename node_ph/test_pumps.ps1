Write-Host "========================================" -ForegroundColor Green
Write-Host "PUMP DEBUG TEST - NODE pH" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

Write-Host "Initializing ESP-IDF environment..." -ForegroundColor Yellow

# Запуск ESP-IDF инициализации
$env:IDF_PATH = "C:\Espressif\esp-idf"
$env:IDF_TOOLS_PATH = "C:\Espressif"

# Инициализация ESP-IDF
& "C:\Espressif\idf_cmd_init.bat" esp-idf-1dcc643656a1439837fdf6ab63363005

Write-Host ""
Write-Host "Building project..." -ForegroundColor Yellow
& "idf.py" "build"

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Now flashing and monitoring..." -ForegroundColor Yellow
    & "idf.py" "flash" "monitor"
} else {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "BUILD FAILED!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "Check errors above" -ForegroundColor Red
}
