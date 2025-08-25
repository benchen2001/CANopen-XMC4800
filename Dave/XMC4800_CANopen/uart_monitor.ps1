# uart_monitor.ps1 - UART 監控腳本
param(
    [string]$ComPort = "COM3",
    [int]$BaudRate = 115200
)

Write-Host "=== UART 監控器 ===" -ForegroundColor Cyan
Write-Host "COM 埠：$ComPort, 鮑率：$BaudRate" -ForegroundColor Yellow

try {
    # 建立串口連接
    $port = New-Object System.IO.Ports.SerialPort $ComPort, $BaudRate, None, 8, One
    $port.Open()
    
    Write-Host "已連接到 $ComPort，按 Ctrl+C 停止監控" -ForegroundColor Green
    Write-Host "等待資料..." -ForegroundColor Yellow
    
    while ($true) {
        if ($port.BytesToRead -gt 0) {
            $data = $port.ReadExisting()
            Write-Host $data -NoNewline -ForegroundColor White
        }
        Start-Sleep -Milliseconds 100
    }
} catch {
    Write-Host "錯誤：$($_.Exception.Message)" -ForegroundColor Red
    Write-Host "請檢查 COM 埠號是否正確" -ForegroundColor Yellow
} finally {
    if ($port -and $port.IsOpen) {
        $port.Close()
        Write-Host "`n已關閉 COM 埠連接" -ForegroundColor Yellow
    }
}