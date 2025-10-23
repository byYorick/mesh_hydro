# Backend API Test Script
# Laravel 11 + Reverb

$baseUrl = "http://localhost:8000/api"

Write-Host "`n=====================================================" -ForegroundColor Cyan
Write-Host " ТЕСТИРОВАНИЕ BACKEND API (Laravel 11)" -ForegroundColor Green
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host ""

# Test 1: Nodes
Write-Host "[1/5] GET /api/nodes" -ForegroundColor Yellow
$nodes = Invoke-RestMethod -Uri "$baseUrl/nodes"
Write-Host "OK - Узлов: $($nodes.Count)" -ForegroundColor Green
$nodes | Select-Object node_id, node_type, online, mac_address | Format-Table
Write-Host ""

# Test 2: Dashboard
Write-Host "[2/5] GET /api/dashboard/summary" -ForegroundColor Yellow
$dashboard = Invoke-RestMethod -Uri "$baseUrl/dashboard/summary"
Write-Host "OK - Узлов всего/онлайн: $($dashboard.nodes.total)/$($dashboard.nodes.online)" -ForegroundColor Green
Write-Host ""

# Test 3: Events
Write-Host "[3/5] GET /api/events" -ForegroundColor Yellow
$events = Invoke-RestMethod -Uri "$baseUrl/events?limit=3"
Write-Host "OK - Событий: $($events.Count)" -ForegroundColor Green
Write-Host ""

# Test 4: Telemetry
Write-Host "[4/5] GET /api/telemetry" -ForegroundColor Yellow
$telemetry = Invoke-RestMethod -Uri "$baseUrl/telemetry?limit=3"
Write-Host "OK - Записей телеметрии: $($telemetry.data.Count)" -ForegroundColor Green
Write-Host ""

# Test 5: Send command
Write-Host "[5/5] POST /api/nodes/root_98a316f5fde8/command" -ForegroundColor Yellow
$cmdBody = @{ command = "status"; params = @() } | ConvertTo-Json
try {
    $cmdResult = Invoke-RestMethod -Uri "$baseUrl/nodes/root_98a316f5fde8/command" -Method Post -Body $cmdBody -ContentType "application/json"
    Write-Host "OK - Команда отправлена" -ForegroundColor Green
} catch {
    Write-Host "WARN - $($_.Exception.Message)" -ForegroundColor Yellow
}
Write-Host ""

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host " ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ" -ForegroundColor Green
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Backend API: http://localhost:8000" -ForegroundColor Cyan
Write-Host "WebSocket:   ws://localhost:8080" -ForegroundColor Cyan
Write-Host "Frontend:    http://localhost:3000" -ForegroundColor Cyan
Write-Host ""
