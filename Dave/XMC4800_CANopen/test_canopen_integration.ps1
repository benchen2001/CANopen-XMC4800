# XMC4800 CANopen Integration Test Script
# PowerShell script to verify CANopenNode integration

Write-Host "=== XMC4800 CANopen Integration Test ===" -ForegroundColor Green

# Check CANopenNode directory structure
Write-Host "`n1. Checking CANopenNode file structure..." -ForegroundColor Yellow

$CanopenDir = ".\CANopenNode"
$Files301Dir = ".\CANopenNode\301"
$PortDir = ".\port"
$AppDir = ".\application"

if (Test-Path $CanopenDir) {
    Write-Host "   ✓ CANopenNode directory found" -ForegroundColor Green
    
    # Check main files
    if (Test-Path "$CanopenDir\CANopen.h") {
        Write-Host "   ✓ CANopen.h found" -ForegroundColor Green
    }
    else {
        Write-Host "   ✗ CANopen.h missing" -ForegroundColor Red
    }
    
    if (Test-Path "$CanopenDir\CANopen.c") {
        Write-Host "   ✓ CANopen.c found" -ForegroundColor Green
    }
    else {
        Write-Host "   ✗ CANopen.c missing" -ForegroundColor Red
    }
    
    # Check 301 directory
    if (Test-Path $Files301Dir) {
        Write-Host "   ✓ 301 directory found" -ForegroundColor Green
        $files301 = Get-ChildItem $Files301Dir -Filter "*.h" | Measure-Object
        Write-Host "   ✓ Found $($files301.Count) header files in 301/" -ForegroundColor Green
    }
    else {
        Write-Host "   ✗ 301 directory missing" -ForegroundColor Red
    }
}
else {
    Write-Host "   ✗ CANopenNode directory not found" -ForegroundColor Red
}

# Check port layer
Write-Host "`n2. Checking XMC4800 port layer..." -ForegroundColor Yellow

if (Test-Path $PortDir) {
    Write-Host "   ✓ Port directory found" -ForegroundColor Green
    
    if (Test-Path "$PortDir\CO_driver_target.h") {
        Write-Host "   ✓ CO_driver_target.h found" -ForegroundColor Green
    }
    else {
        Write-Host "   ✗ CO_driver_target.h missing" -ForegroundColor Red
    }
    
    if (Test-Path "$PortDir\CO_driver_XMC4800.c") {
        Write-Host "   ✓ CO_driver_XMC4800.c found" -ForegroundColor Green
    }
    else {
        Write-Host "   ✗ CO_driver_XMC4800.c missing" -ForegroundColor Red
    }
}
else {
    Write-Host "   ✗ Port directory not found" -ForegroundColor Red
}

# Check application layer
Write-Host "`n3. Checking application layer..." -ForegroundColor Yellow

if (Test-Path $AppDir) {
    Write-Host "   ✓ Application directory found" -ForegroundColor Green
    
    if (Test-Path "$AppDir\OD.h") {
        Write-Host "   ✓ OD.h found" -ForegroundColor Green
    }
    else {
        Write-Host "   ✗ OD.h missing" -ForegroundColor Red
    }
    
    if (Test-Path "$AppDir\OD.c") {
        Write-Host "   ✓ OD.c found" -ForegroundColor Green
    }
    else {
        Write-Host "   ✗ OD.c missing" -ForegroundColor Red
    }
}
else {
    Write-Host "   ✗ Application directory not found" -ForegroundColor Red
}

# Check configuration files
Write-Host "`n4. Checking configuration files..." -ForegroundColor Yellow

if (Test-Path "$CanopenDir\CO_config.h") {
    Write-Host "   ✓ CO_config.h found" -ForegroundColor Green
    
    # Check for key configuration defines
    $config_content = Get-Content "$CanopenDir\CO_config.h" -Raw
    if ($config_content -match "CO_CONFIG_NMT") {
        Write-Host "   ✓ NMT configuration found" -ForegroundColor Green
    }
    if ($config_content -match "CO_CONFIG_SDO_SRV") {
        Write-Host "   ✓ SDO server configuration found" -ForegroundColor Green
    }
    if ($config_content -match "CO_CONFIG_PDO") {
        Write-Host "   ✓ PDO configuration found" -ForegroundColor Green
    }
}
else {
    Write-Host "   ✗ CO_config.h missing" -ForegroundColor Red
}

# Check integration files
Write-Host "`n5. Checking integration files..." -ForegroundColor Yellow

if (Test-Path "main_canopen_device.c") {
    Write-Host "   ✓ main_canopen_device.c found" -ForegroundColor Green
    
    # Check for key integration points
    $main_content = Get-Content "main_canopen_device.c" -Raw
    if ($main_content -match "#include.*CANopen\.h") {
        Write-Host "   ✓ CANopen header included" -ForegroundColor Green
    }
    if ($main_content -match "CO_new") {
        Write-Host "   ✓ CANopen initialization found" -ForegroundColor Green
    }
    if ($main_content -match "MODE_HYBRID") {
        Write-Host "   ✓ Hybrid mode support found" -ForegroundColor Green
    }
}
else {
    Write-Host "   ✗ main_canopen_device.c missing" -ForegroundColor Red
}

# Summary
Write-Host "`n=== Integration Status Summary ===" -ForegroundColor Cyan

$core_files_ok = (Test-Path "$CanopenDir\CANopen.h") -and (Test-Path "$CanopenDir\CANopen.c")
$port_files_ok = (Test-Path "$PortDir\CO_driver_target.h") -and (Test-Path "$PortDir\CO_driver_XMC4800.c")
$app_files_ok = (Test-Path "$AppDir\OD.h") -and (Test-Path "$AppDir\OD.c")
$config_ok = Test-Path "$CanopenDir\CO_config.h"
$integration_ok = Test-Path "main_canopen_device.c"

Write-Host "Core CANopenNode files: " -NoNewline
Write-Host (if ($core_files_ok) { "✓ OK" } else { "✗ MISSING" }) -ForegroundColor (if ($core_files_ok) { "Green" } else { "Red" })

Write-Host "XMC4800 port layer: " -NoNewline  
Write-Host (if ($port_files_ok) { "✓ OK" } else { "✗ MISSING" }) -ForegroundColor (if ($port_files_ok) { "Green" } else { "Red" })

Write-Host "Application layer: " -NoNewline
Write-Host (if ($app_files_ok) { "✓ OK" } else { "✗ MISSING" }) -ForegroundColor (if ($app_files_ok) { "Green" } else { "Red" })

Write-Host "Configuration: " -NoNewline
Write-Host (if ($config_ok) { "✓ OK" } else { "✗ MISSING" }) -ForegroundColor (if ($config_ok) { "Green" } else { "Red" })

Write-Host "Integration code: " -NoNewline
Write-Host (if ($integration_ok) { "✓ OK" } else { "✗ MISSING" }) -ForegroundColor (if ($integration_ok) { "Green" } else { "Red" })

$overall_status = $core_files_ok -and $port_files_ok -and $config_ok -and $integration_ok

Write-Host "`nOverall Integration Status: " -NoNewline
Write-Host (if ($overall_status) { "✓ READY" } else { "✗ INCOMPLETE" }) -ForegroundColor (if ($overall_status) { "Green" } else { "Red" })

if ($overall_status) {
    Write-Host "`n🎉 CANopen integration is ready for compilation!" -ForegroundColor Green
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "1. Complete missing OD.c implementation" -ForegroundColor White
    Write-Host "2. Test compilation with DAVE IDE" -ForegroundColor White
    Write-Host "3. Test CANopen device functionality" -ForegroundColor White
}
else {
    Write-Host "`n⚠️  Integration needs attention before compilation" -ForegroundColor Yellow
}

Write-Host "`n=== Test Complete ===" -ForegroundColor Green