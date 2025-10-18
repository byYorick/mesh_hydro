# ========================================
# Mesh Hydro System - Scheduler
# ========================================
# Run Laravel scheduled tasks every minute

$BackendPath = $PSScriptRoot
$LogFile = Join-Path $BackendPath "storage\logs\scheduler.log"

# Create logs directory if not exists
$LogDir = Split-Path $LogFile
if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}

Write-Host "========================================" -ForegroundColor Green
Write-Host "  Hydro System Scheduler Started" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Backend Path: $BackendPath" -ForegroundColor Cyan
Write-Host "Log File: $LogFile" -ForegroundColor Cyan
Write-Host ""
Write-Host "Press Ctrl+C to stop" -ForegroundColor Yellow
Write-Host ""

# Change to backend directory
Set-Location $BackendPath

# Main loop
while ($true) {
    try {
        $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Write-Host "[$timestamp] Running scheduled tasks..." -ForegroundColor Cyan
        
        # Run Laravel scheduler
        $output = php artisan schedule:run 2>&1
        
        # Log output
        "[$timestamp] $output" | Out-File -Append $LogFile
        
        # Show output if not "No scheduled commands"
        if ($output -notmatch "No scheduled commands") {
            Write-Host "  $output" -ForegroundColor White
        }
        
    } catch {
        $errorMsg = $_.Exception.Message
        Write-Host "  [ERROR] $errorMsg" -ForegroundColor Red
        "[$timestamp] ERROR: $errorMsg" | Out-File -Append $LogFile
    }
    
    # Wait 60 seconds
    Start-Sleep -Seconds 60
}

