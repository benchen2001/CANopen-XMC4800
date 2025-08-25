# test.ps1 - 完整測試腳本
Write-Host "=== XMC4800 完整測試流程 ===" -ForegroundColor Cyan

# 步驟 1：編譯
Write-Host "步驟 1：編譯程式" -ForegroundColor Yellow
.\build.ps1 -Clean

if ($LASTEXITCODE -ne 0) {
    Write-Host "編譯失敗，測試終止。" -ForegroundColor Red
    exit 1
}

# 步驟 2：燒錄
Write-Host "`n步驟 2：燒錄程式" -ForegroundColor Yellow
.\flash.ps1

if ($LASTEXITCODE -ne 0) {
    Write-Host "燒錄失敗，測試終止。" -ForegroundColor Red
    exit 1
}

# 步驟 3：功能測試指導
Write-Host "`n步驟 3：功能測試" -ForegroundColor Yellow
Write-Host "請手動驗證以下功能：" -ForegroundColor White

Write-Host "`n【LED 測試】" -ForegroundColor Green
Write-Host "✓ LED1 應該快速閃爍 3 次（初始化成功指示）"
Write-Host "✓ 然後 LED1 規律閃爍（心跳指示）"
Write-Host "✓ LED2 間歇閃爍（通訊活動指示）"

Write-Host "`n【UART 測試】" -ForegroundColor Green
Write-Host "✓ 連接 UART 終端機（115200, 8N1）"
Write-Host "✓ 應該看到初始化訊息和週期性 CAN 發送訊息"
Write-Host "✓ 執行 .\uart_monitor.ps1 -ComPort COM3 來監控"

Write-Host "`n【CAN 測試】" -ForegroundColor Green
Write-Host "✓ 連接 CAN 分析儀（500 kbps）"
Write-Host "✓ 應該看到 ID 0x123 的週期性訊息"
Write-Host "✓ 資料內容遞增：02 03 04 05 06 07 08 09..."

Write-Host "`n【故障排除】" -ForegroundColor Yellow
Write-Host "如果 LED1 快速連續閃爍 → DAVE 初始化失敗"
Write-Host "如果沒有 UART 輸出 → 檢查 UART 腳位和設定"
Write-Host "如果沒有 CAN 活動 → 檢查 CAN 收發器和腳位"

Write-Host "`n=== 測試完成 ===" -ForegroundColor Cyan