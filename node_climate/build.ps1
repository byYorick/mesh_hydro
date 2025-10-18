# Скрипт сборки для node_climate
# Инициализирует окружение ESP-IDF и запускает сборку

Write-Host "Инициализация ESP-IDF окружения..." -ForegroundColor Cyan

# Путь к ESP-IDF
$IDF_PATH = "C:\Espressif\frameworks\esp-idf-v5.1.2"
$IDF_TOOLS_PATH = "C:\Espressif"

# Установка переменных окружения
$env:IDF_PATH = $IDF_PATH
$env:IDF_TOOLS_PATH = $IDF_TOOLS_PATH

# Запуск export.ps1 для настройки окружения
if (Test-Path "$IDF_PATH\export.ps1") {
    Write-Host "Загрузка переменных окружения ESP-IDF..." -ForegroundColor Yellow
    & "$IDF_PATH\export.ps1"
    
    Write-Host "`nЗапуск сборки проекта..." -ForegroundColor Green
    idf.py build
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`nСборка завершена успешно!" -ForegroundColor Green
    } else {
        Write-Host "`nОшибка при сборке!" -ForegroundColor Red
        exit $LASTEXITCODE
    }
} else {
    Write-Host "Ошибка: не найден $IDF_PATH\export.ps1" -ForegroundColor Red
    Write-Host "Проверьте установку ESP-IDF" -ForegroundColor Red
    exit 1
}

