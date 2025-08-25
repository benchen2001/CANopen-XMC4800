# XMC4800 Professional CANopen Monitor - Quick Start Script
# 專業級 CANopen 監控系統快速啟動腳本

param(
    [string]$ComPort = "COM3",
    [int]$BaudRate = 115200,
    [switch]$Flash,
    [switch]$Monitor,
    [switch]$Build,
    [switch]$Test,
    [switch]$All
)

Write-Host "🚀 XMC4800 Professional CANopen Monitor" -ForegroundColor Green
Write-Host "=======================================" -ForegroundColor Green

$PROJECT_ROOT = "C:\prj\AI\CANOpen"
$DAVE_PROJECT = "$PROJECT_ROOT\Dave\XMC4800_CANopen"
$BUILD_DIR = "$DAVE_PROJECT\Debug"

# 檢查 Python 是否可用
function Test-Python {
    try {
        $pythonVersion = python --version 2>&1
        Write-Host "✅ Python: $pythonVersion" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "❌ Python 未安裝或不在 PATH 中" -ForegroundColor Red
        return $false
    }
}

# 建置程式
function Build-Project {
    Write-Host "`n🔨 建置 CANopen 監控程式..." -ForegroundColor Yellow
    
    Set-Location $BUILD_DIR
    $result = & "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ 建置成功!" -ForegroundColor Green
        
        $elfFile = "$BUILD_DIR\XMC4800_CANopen.elf"
        if (Test-Path $elfFile) {
            $fileSize = (Get-Item $elfFile).Length
            Write-Host "📦 ELF 檔案大小: $fileSize bytes" -ForegroundColor Cyan
        }
        return $true
    }
    else {
        Write-Host "❌ 建置失敗!" -ForegroundColor Red
        Write-Host $result -ForegroundColor Red
        return $false
    }
}

# 燒錄程式
function Flash-Program {
    Write-Host "`n💾 燒錄程式到 XMC4800..." -ForegroundColor Yellow
    
    $jlinkScript = "$BUILD_DIR\flash_canopen_monitor.jlink"
    if (-not (Test-Path $jlinkScript)) {
        Write-Host "❌ 找不到 J-Link 腳本: $jlinkScript" -ForegroundColor Red
        return $false
    }
    
    $result = & "C:\Program Files (x86)\SEGGER\JLink\JLink.exe" -CommanderScript $jlinkScript 2>&1
    
    if ($result -like "*Script processing completed*") {
        Write-Host "✅ 燒錄成功!" -ForegroundColor Green
        return $true
    }
    else {
        Write-Host "❌ 燒錄失敗!" -ForegroundColor Red
        Write-Host $result -ForegroundColor Red
        return $false
    }
}

# 啟動監控
function Start-Monitor {
    Write-Host "`n🔍 啟動 CANopen 監控程式..." -ForegroundColor Yellow
    Write-Host "連接端口: $ComPort (波特率: $BaudRate)" -ForegroundColor Cyan
    Write-Host "按 Ctrl+C 停止監控`n" -ForegroundColor Cyan
    
    Set-Location $PROJECT_ROOT
    python canopen_monitor.py $ComPort -b $BaudRate
}

# 系統測試
function Test-System {
    Write-Host "`n🧪 系統測試..." -ForegroundColor Yellow
    
    # 檢查檔案
    $files = @(
        "$DAVE_PROJECT\main.c",
        "$BUILD_DIR\XMC4800_CANopen.elf",
        "$PROJECT_ROOT\canopen_monitor.py",
        "$PROJECT_ROOT\xmc_canopen_dissector.lua"
    )
    
    foreach ($file in $files) {
        if (Test-Path $file) {
            Write-Host "✅ $([System.IO.Path]::GetFileName($file))" -ForegroundColor Green
        }
        else {
            Write-Host "❌ $([System.IO.Path]::GetFileName($file))" -ForegroundColor Red
        }
    }
    
    # 檢查工具
    Test-Python | Out-Null
    
    # 檢查 J-Link
    if (Test-Path "C:\Program Files (x86)\SEGGER\JLink\JLink.exe") {
        Write-Host "✅ J-Link 工具" -ForegroundColor Green
    }
    else {
        Write-Host "❌ J-Link 工具" -ForegroundColor Red
    }
    
    # 檢查串口
    try {
        $ports = [System.IO.Ports.SerialPort]::getportnames()
        if ($ports -contains $ComPort) {
            Write-Host "✅ 串口 $ComPort 可用" -ForegroundColor Green
        }
        else {
            Write-Host "⚠️  串口 $ComPort 不存在" -ForegroundColor Yellow
            Write-Host "可用串口: $($ports -join ', ')" -ForegroundColor Cyan
        }
    }
    catch {
        Write-Host "❌ 無法檢查串口" -ForegroundColor Red
    }
}

# 顯示說明
function Show-Help {
    Write-Host "`n📖 使用方法:" -ForegroundColor Yellow
    Write-Host "  .\quick_start.ps1 -All              # 執行完整流程 (建置+燒錄+監控)" -ForegroundColor Cyan
    Write-Host "  .\quick_start.ps1 -Build            # 只建置程式" -ForegroundColor Cyan
    Write-Host "  .\quick_start.ps1 -Flash            # 只燒錄程式" -ForegroundColor Cyan
    Write-Host "  .\quick_start.ps1 -Monitor -ComPort COM5  # 啟動監控 (指定端口)" -ForegroundColor Cyan
    Write-Host "  .\quick_start.ps1 -Test             # 系統測試" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "📋 參數說明:" -ForegroundColor Yellow
    Write-Host "  -ComPort    串口名稱 (預設: COM3)" -ForegroundColor Cyan
    Write-Host "  -BaudRate   波特率 (預設: 115200)" -ForegroundColor Cyan
    Write-Host ""
}

# 主要邏輯
try {
    Set-Location $PROJECT_ROOT
    
    if ($All) {
        Test-System
        if (Build-Project -and Flash-Program) {
            Write-Host "`n⏱️  等待系統穩定..." -ForegroundColor Yellow
            Start-Sleep 3
            Start-Monitor
        }
    }
    elseif ($Build) {
        Build-Project
    }
    elseif ($Flash) {
        Flash-Program
    }
    elseif ($Monitor) {
        if (-not (Test-Python)) {
            Write-Host "請先安裝 Python 和 pyserial 套件:" -ForegroundColor Yellow
            Write-Host "pip install pyserial" -ForegroundColor White
            exit 1
        }
        Start-Monitor
    }
    elseif ($Test) {
        Test-System
    }
    else {
        Show-Help
        Test-System
    }
    
}
catch {
    Write-Host "❌ 執行錯誤: $_" -ForegroundColor Red
    exit 1
}
finally {
    Set-Location $PROJECT_ROOT
}

Write-Host "`n🎯 專業級 CANopen 監控系統就緒!" -ForegroundColor Green