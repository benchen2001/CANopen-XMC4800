# build.ps1 - 自動編譯腳本
param(
    [switch]$Clean = $false
)

$ToolPath = "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin"
$DebugPath = "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"

Write-Host "=== XMC4800 自動編譯腳本 ===" -ForegroundColor Cyan

# 切換到 Debug 目錄
cd $DebugPath

if ($Clean) {
    Write-Host "清理之前的編譯檔案..." -ForegroundColor Yellow
    & "$ToolPath\make.exe" clean
}

Write-Host "開始編譯..." -ForegroundColor Yellow
& "$ToolPath\make.exe" all

if ($LASTEXITCODE -eq 0) {
    Write-Host "編譯成功！" -ForegroundColor Green
    
    # 生成 binary 檔案
    Write-Host "生成燒錄檔案..." -ForegroundColor Yellow
    & "$ToolPath\arm-none-eabi-objcopy.exe" -O binary XMC4800_CANopen.elf XMC4800_CANopen.bin
    & "$ToolPath\arm-none-eabi-objcopy.exe" -O ihex XMC4800_CANopen.elf XMC4800_CANopen.hex
    
    # 顯示程式大小
    Write-Host "程式大小資訊：" -ForegroundColor Green
    & "$ToolPath\arm-none-eabi-size.exe" XMC4800_CANopen.elf
    
    # 顯示檔案資訊
    Write-Host "生成的檔案：" -ForegroundColor Green
    dir XMC4800_CANopen.elf, XMC4800_CANopen.bin, XMC4800_CANopen.hex | Format-Table Name, Length, LastWriteTime
    
}
else {
    Write-Host "編譯失敗！請檢查錯誤訊息。" -ForegroundColor Red
    exit 1
}