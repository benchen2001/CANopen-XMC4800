# run_all.ps1 - 一鍵執行所有操作
param(
    [switch]$SkipBuild = $false,
    [switch]$SkipFlash = $false,
    [switch]$MonitorUART = $false,
    [string]$ComPort = "COM3"
)

Write-Host "=== XMC4800 一鍵執行腳本 ===" -ForegroundColor Cyan

# 檢查工具
Write-Host "檢查工具可用性..." -ForegroundColor Yellow
$tools = @{
    "ARM-GCC" = "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-gcc.exe"
    "Make" = "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe"
    "J-Link" = "C:\Program Files\SEGGER\JLink\JLink.exe"
}

foreach ($tool in $tools.GetEnumerator()) {
    if (Test-Path $tool.Value) {
        Write-Host "✓ $($tool.Key) 可用" -ForegroundColor Green
    } else {
        Write-Host "✗ $($tool.Key) 不可用：$($tool.Value)" -ForegroundColor Red
        exit 1
    }
}

# 編譯
if (-not $SkipBuild) {
    Write-Host "`n執行編譯..." -ForegroundColor Yellow
    .\build.ps1 -Clean
    if ($LASTEXITCODE -ne 0) { exit 1 }
}

# 燒錄
if (-not $SkipFlash) {
    Write-Host "`n執行燒錄..." -ForegroundColor Yellow
    .\flash.ps1
    if ($LASTEXITCODE -ne 0) { exit 1 }
}

# UART 監控
if ($MonitorUART) {
    Write-Host "`n啟動 UART 監控..." -ForegroundColor Yellow
    .\uart_monitor.ps1 -ComPort $ComPort
}

Write-Host "`n=== 所有操作完成 ===" -ForegroundColor Cyan
Write-Host "你的 XMC4800 程式已經在運行了！" -ForegroundColor Green