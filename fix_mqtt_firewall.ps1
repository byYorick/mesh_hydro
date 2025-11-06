# ========================================
# Script to open MQTT port in Windows Firewall
# ========================================
# Run this script as ADMINISTRATOR:
#   - Right-click file -> "Run with PowerShell"
# ========================================

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Opening port 1883 for MQTT broker" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check admin rights
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "ERROR: This script requires administrator privileges!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Run PowerShell as Administrator and execute:" -ForegroundColor Yellow
    Write-Host "  .\fix_mqtt_firewall.ps1" -ForegroundColor Yellow
    Write-Host ""
    pause
    exit 1
}

Write-Host "Administrator privileges confirmed" -ForegroundColor Green
Write-Host ""

# Check existing rule
$existingRule = Get-NetFirewallRule -DisplayName "MQTT Broker (Port 1883)" -ErrorAction SilentlyContinue

if ($existingRule) {
    Write-Host "Rule already exists" -ForegroundColor Yellow
    Write-Host "  Removing old rule..." -ForegroundColor Yellow
    Remove-NetFirewallRule -DisplayName "MQTT Broker (Port 1883)" -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
}

# Create new firewall rule
Write-Host "  Creating firewall rule for port 1883 (TCP, Inbound)..." -ForegroundColor Cyan

try {
    $rule = New-NetFirewallRule `
        -DisplayName "MQTT Broker (Port 1883)" `
        -Description "Allows inbound connections to MQTT broker on port 1883 for ESP32 nodes" `
        -Direction Inbound `
        -Protocol TCP `
        -LocalPort 1883 `
        -Action Allow `
        -Profile Domain,Private,Public `
        -Enabled True `
        -ErrorAction Stop

    Write-Host "Rule created successfully!" -ForegroundColor Green
    Write-Host ""
    
    # Display rule info
    Write-Host "Rule information:" -ForegroundColor Cyan
    $rule | Format-List DisplayName, Enabled, Direction, Action, Protocol, LocalPort, Profile
    Write-Host ""
    
} catch {
    Write-Host "ERROR creating rule:" -ForegroundColor Red
    Write-Host "   $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    pause
    exit 1
}

# Test connection
Write-Host "Testing connection to port 1883..." -ForegroundColor Cyan

$testResult = Test-NetConnection -ComputerName 192.168.1.100 -Port 1883 -InformationLevel Quiet -WarningAction SilentlyContinue

if ($testResult) {
    Write-Host "Connection to 192.168.1.100:1883 works!" -ForegroundColor Green
} else {
    Write-Host "Connection to 192.168.1.100:1883 still not working" -ForegroundColor Yellow
    Write-Host "   Possible reasons:" -ForegroundColor Yellow
    Write-Host "   - IP address 192.168.1.100 not reachable from ESP32 network" -ForegroundColor Yellow
    Write-Host "   - Router blocking connections between devices" -ForegroundColor Yellow
    Write-Host "   - Interface with IP 192.168.1.100 inactive" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Done! Now restart ESP32 nodes" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Read-Host "Press Enter to exit"
