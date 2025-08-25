# CANopen Wireshark 監控啟動腳本
# 這個腳本會啟動橋接程式並開啟 Wireshark 進行即時監控

Write-Host "=== CANopen Wireshark 監控系統 ===" -ForegroundColor Green
Write-Host "啟動 CANopen 即時封包分析..." -ForegroundColor Yellow

# 檢查 Python 是否安裝
try {
    $pythonVersion = python --version 2>&1
    Write-Host "Python 版本: $pythonVersion" -ForegroundColor Cyan
}
catch {
    Write-Host "錯誤: 找不到 Python！請先安裝 Python 3.x" -ForegroundColor Red
    exit 1
}

# 檢查 Wireshark 是否安裝
$wiresharkPath = "C:\Program Files\Wireshark\Wireshark.exe"
if (-not (Test-Path $wiresharkPath)) {
    Write-Host "錯誤: 找不到 Wireshark！請先安裝 Wireshark" -ForegroundColor Red
    Write-Host "下載地址: https://www.wireshark.org/download.html" -ForegroundColor Yellow
    exit 1
}

# 檢查串列埠
Write-Host "正在檢查可用的串列埠..." -ForegroundColor Yellow
$availablePorts = [System.IO.Ports.SerialPort]::getPortNames()
if ($availablePorts.Count -eq 0) {
    Write-Host "警告: 沒有找到可用的串列埠" -ForegroundColor Yellow
    Write-Host "請確認 XMC4800 已連接並安裝驅動程式" -ForegroundColor Yellow
}
else {
    Write-Host "可用串列埠: $($availablePorts -join ', ')" -ForegroundColor Cyan
}

# 設定串列埠 (預設 COM3，可修改)
$serialPort = "COM3"
if ($availablePorts -contains "COM3") {
    Write-Host "使用串列埠: COM3" -ForegroundColor Green
}
elseif ($availablePorts.Count -gt 0) {
    $serialPort = $availablePorts[0]
    Write-Host "使用串列埠: $serialPort" -ForegroundColor Yellow
}
else {
    Write-Host "無法找到串列埠，使用預設值 COM3" -ForegroundColor Yellow
}

# 清理舊的 pcap 檔案
$pcapFile = "canopen_live.pcap"
if (Test-Path $pcapFile) {
    Remove-Item $pcapFile
    Write-Host "已清理舊的 pcap 檔案" -ForegroundColor Yellow
}

Write-Host "`n開始監控程序..." -ForegroundColor Green
Write-Host "1. 啟動橋接程式接收 CANopen 資料" -ForegroundColor Cyan
Write-Host "2. 啟動 Wireshark 進行即時分析" -ForegroundColor Cyan
Write-Host "3. 按 Ctrl+C 停止監控" -ForegroundColor Cyan
Write-Host "-" * 50

# 啟動橋接程式 (背景執行)
$bridgeJob = Start-Job -ScriptBlock {
    param($port)
    Set-Location $using:PWD
    python canopen_wireshark_bridge.py $port
} -ArgumentList $serialPort

Write-Host "橋接程式已啟動 (Job ID: $($bridgeJob.Id))" -ForegroundColor Green

# 等待一下讓橋接程式初始化
Start-Sleep -Seconds 2

# 檢查橋接程式是否正常運行
if ($bridgeJob.State -eq "Running") {
    Write-Host "橋接程式運行正常" -ForegroundColor Green
    
    # 等待 pcap 檔案建立
    $timeout = 10
    $elapsed = 0
    while ((-not (Test-Path $pcapFile)) -and ($elapsed -lt $timeout)) {
        Start-Sleep -Seconds 1
        $elapsed++
        Write-Host "等待 pcap 檔案建立... ($elapsed/$timeout)" -ForegroundColor Yellow
    }
    
    if (Test-Path $pcapFile) {
        Write-Host "pcap 檔案已建立，啟動 Wireshark..." -ForegroundColor Green
        
        # 啟動 Wireshark
        try {
            & $wiresharkPath -r $pcapFile -Y "udp.port == 3211" &
            Write-Host "Wireshark 已啟動" -ForegroundColor Green
            Write-Host "過濾器已設定為: udp.port == 3211 (CANopen 封包)" -ForegroundColor Cyan
        }
        catch {
            Write-Host "啟動 Wireshark 時發生錯誤: $_" -ForegroundColor Red
        }
    }
    else {
        Write-Host "警告: pcap 檔案未建立，可能沒有接收到資料" -ForegroundColor Yellow
    }
}
else {
    Write-Host "錯誤: 橋接程式啟動失敗" -ForegroundColor Red
    Receive-Job $bridgeJob
}

Write-Host "`n監控系統已啟動完成！" -ForegroundColor Green
Write-Host "使用說明:" -ForegroundColor Cyan
Write-Host "- Wireshark 視窗會顯示即時的 CANopen 封包" -ForegroundColor White
Write-Host "- 每個封包都包含完整的時間戳和 CANopen 資料" -ForegroundColor White
Write-Host "- 可以使用 Wireshark 的篩選功能進行詳細分析" -ForegroundColor White
Write-Host "- 按任意鍵停止監控..." -ForegroundColor Yellow

# 等待使用者按鍵停止
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

# 清理工作
Write-Host "`n正在停止監控..." -ForegroundColor Yellow
Stop-Job $bridgeJob
Remove-Job $bridgeJob

Write-Host "監控已停止" -ForegroundColor Green
Write-Host "pcap 檔案已保存: $pcapFile" -ForegroundColor Cyan