# flash.ps1 - 自動燒錄腳本
param(
    [switch]$Verify = $true
)

$JLinkPath = "C:\Program Files\SEGGER\JLink\JLink.exe"
$BinaryFile = "Debug\XMC4800_CANopen.bin"
$ScriptFile = "flash_temp.txt"

Write-Host "=== XMC4800 自動燒錄腳本 ===" -ForegroundColor Cyan

# 檢查檔案是否存在
if (-not (Test-Path $BinaryFile)) {
    Write-Host "錯誤：找不到 $BinaryFile" -ForegroundColor Red
    Write-Host "請先執行 .\build.ps1 進行編譯" -ForegroundColor Yellow
    exit 1
}

# 檢查檔案大小
$FileSize = (Get-Item $BinaryFile).Length
Write-Host "燒錄檔案：$BinaryFile (大小：$FileSize bytes)" -ForegroundColor Yellow

# 建立燒錄腳本
if ($Verify) {
    $FlashCommands = @"
r
h
erase 0x0C000000 0x0C010000
loadbin $BinaryFile, 0x0C000000
verifybin $BinaryFile, 0x0C000000
r
g
qc
"@
}
else {
    $FlashCommands = @"
r
h
loadbin $BinaryFile, 0x0C000000
r
g
qc
"@
}

$FlashCommands | Out-File -FilePath $ScriptFile -Encoding ASCII

Write-Host "開始燒錄..." -ForegroundColor Yellow

# 執行燒錄
& $JLinkPath -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 -CommanderScript $ScriptFile

if ($LASTEXITCODE -eq 0) {
    Write-Host "燒錄成功！" -ForegroundColor Green
    Write-Host "程式已開始執行，請觀察 LED 狀態：" -ForegroundColor Green
    Write-Host "- LED1 應該快速閃爍 3 次然後規律閃爍" -ForegroundColor White
    Write-Host "- LED2 應該間歇閃爍表示活動" -ForegroundColor White
}
else {
    Write-Host "燒錄失敗！請檢查硬體連接和 J-Link 驅動。" -ForegroundColor Red
}

# 清理臨時檔案
if (Test-Path $ScriptFile) {
    Remove-Item $ScriptFile
}