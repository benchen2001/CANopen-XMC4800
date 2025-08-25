# XMC4800 實戰完整指南 - 真正能用的版本

## 🎯 目標：從頭到尾完成一個可運行的 UART + CAN 專案

---

## 第一部分：工具和環境確認

### 1.1 檢查所有必要工具
```powershell
# 檢查 DAVE IDE
Test-Path "C:\Infineon\DAVE-IDE-4.5.0.202105191637-64bit\eclipse\DAVE.exe"

# 檢查 ARM-GCC 編譯器
Test-Path "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-gcc.exe"

# 檢查 Make 工具
Test-Path "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe"

# 檢查 J-Link
Test-Path "C:\Program Files\SEGGER\JLink\JLink.exe"
```

**如果任何一個返回 False，你需要先安裝對應的軟體！**

### 1.2 設定環境變數（可選但建議）
```powershell
# 加入編譯工具到 PATH（臨時性）
$env:PATH += ";C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin"
$env:PATH += ";C:\Program Files\SEGGER\JLink"

# 檢查是否生效
arm-none-eabi-gcc --version
make --version
```

---

## 第二部分：檢查現有專案狀態

### 2.1 專案目錄確認
```powershell
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"
dir

# 你應該看到這些資料夾和檔案：
# Dave/          - DAVE 生成的程式碼
# Debug/         - 編譯輸出目錄
# Libraries/     - XMC 函式庫
# Startup/       - 啟動程式碼
# main.c         - 主程式
# .project       - Eclipse 專案檔
```

### 2.2 檢查 DAVE 生成的檔案
```powershell
# 檢查主要標頭檔
Get-Content "Dave\Generated\DAVE.h" | Select-Object -First 20

# 檢查 LED 設定
Get-Content "Dave\Generated\DIGITAL_IO\digital_io_extern.h"

# 檢查 UART 設定
Get-Content "Dave\Generated\UART\uart_extern.h"

# 檢查 CAN 設定
Get-Content "Dave\Generated\CAN_NODE\can_node_extern.h"
```

---

## 第三部分：編譯專案

### 3.1 清理之前的編譯
```powershell
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"

# 使用 Make 清理
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" clean

# 檢查清理結果
dir *.o
dir *.elf
```

### 3.2 重新編譯
```powershell
# 在 Debug 目錄下編譯
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"

# 執行編譯
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all

# 檢查編譯是否成功
if ($LASTEXITCODE -eq 0) {
    Write-Host "編譯成功！" -ForegroundColor Green
    dir *.elf
} else {
    Write-Host "編譯失敗！" -ForegroundColor Red
    # 如果失敗，檢查錯誤訊息
}
```

### 3.3 生成燒錄檔案
```powershell
# 確保在 Debug 目錄
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"

# 生成 binary 檔案
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-objcopy.exe" -O binary XMC4800_CANopen.elf XMC4800_CANopen.bin

# 生成 Intel HEX 檔案
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-objcopy.exe" -O ihex XMC4800_CANopen.elf XMC4800_CANopen.hex

# 檢查檔案大小
dir XMC4800_CANopen.*

# 顯示程式大小資訊
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-size.exe" XMC4800_CANopen.elf
```

---

## 第四部分：燒錄到硬體

### 4.1 硬體連接檢查
```powershell
# 檢查硬體連接
Write-Host "請確認："
Write-Host "1. XMC4800 Kit 已用 USB 線連接到電腦"
Write-Host "2. 板上電源 LED 亮起"
Write-Host "3. 沒有其他程式佔用 J-Link"
```

### 4.2 測試 J-Link 連接
```powershell
# 建立簡單的連接測試腳本
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

@"
connect
q
"@ | Out-File -FilePath "test_connection.txt" -Encoding ASCII

# 測試連接
& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -speed 12000 -CommanderScript test_connection.txt

# 如果連接成功，你會看到類似的訊息：
# "Connecting to target via SWD"
# "Found Cortex-M4"
```

### 4.3 建立燒錄腳本
```powershell
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

# 建立基本燒錄腳本
@"
r
h
loadbin Debug\XMC4800_CANopen.bin, 0x0C000000
r
g
qc
"@ | Out-File -FilePath "flash_program.txt" -Encoding ASCII

# 建立進階燒錄腳本（含擦除和驗證）
@"
r
h
erase 0x0C000000 0x0C010000
loadbin Debug\XMC4800_CANopen.bin, 0x0C000000
verifybin Debug\XMC4800_CANopen.bin, 0x0C000000
r
g
qc
"@ | Out-File -FilePath "flash_program_verify.txt" -Encoding ASCII
```

### 4.4 執行燒錄
```powershell
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

Write-Host "開始燒錄程式..." -ForegroundColor Yellow

# 執行燒錄
& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 -CommanderScript flash_program_verify.txt

# 檢查燒錄結果
if ($LASTEXITCODE -eq 0) {
    Write-Host "燒錄成功！程式應該開始執行了。" -ForegroundColor Green
} else {
    Write-Host "燒錄失敗！請檢查硬體連接。" -ForegroundColor Red
}
```

---

## 第五部分：建立自動化腳本

### 5.1 完整編譯腳本
```powershell
# 建立 build.ps1 腳本
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

@'
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
    
} else {
    Write-Host "編譯失敗！請檢查錯誤訊息。" -ForegroundColor Red
    exit 1
}
'@ | Out-File -FilePath "build.ps1" -Encoding UTF8
```

### 5.2 完整燒錄腳本
```powershell
# 建立 flash.ps1 腳本
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

@'
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
} else {
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
} else {
    Write-Host "燒錄失敗！請檢查硬體連接和 J-Link 驅動。" -ForegroundColor Red
}

# 清理臨時檔案
if (Test-Path $ScriptFile) {
    Remove-Item $ScriptFile
}
'@ | Out-File -FilePath "flash.ps1" -Encoding UTF8
```

### 5.3 完整測試腳本
```powershell
# 建立 test.ps1 腳本
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

@'
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
.\flash.ps1 -Verify

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

Write-Host "`n【CAN 測試】" -ForegroundColor Green
Write-Host "✓ 連接 CAN 分析儀（500 kbps）"
Write-Host "✓ 應該看到 ID 0x123 的週期性訊息"
Write-Host "✓ 資料內容遞增：02 03 04 05 06 07 08 09..."

Write-Host "`n【故障排除】" -ForegroundColor Yellow
Write-Host "如果 LED1 快速連續閃爍 → DAVE 初始化失敗"
Write-Host "如果沒有 UART 輸出 → 檢查 UART 腳位和設定"
Write-Host "如果沒有 CAN 活動 → 檢查 CAN 收發器和腳位"

Write-Host "`n=== 測試完成 ===" -ForegroundColor Cyan
'@ | Out-File -FilePath "test.ps1" -Encoding UTF8
```

---

## 第六部分：UART 通訊測試

### 6.1 使用 PuTTY 測試 UART
```powershell
# 如果安裝了 PuTTY
if (Test-Path "C:\Program Files\PuTTY\putty.exe") {
    Write-Host "啟動 PuTTY UART 終端機..."
    Write-Host "設定：COM 埠號, 115200, 8, N, 1"
    & "C:\Program Files\PuTTY\putty.exe"
} else {
    Write-Host "PuTTY 未安裝，請手動設定 UART 終端機："
    Write-Host "- 鮑率：115200"
    Write-Host "- 資料位元：8"
    Write-Host "- 同位位元：None"
    Write-Host "- 停止位元：1"
}
```

### 6.2 使用 PowerShell 直接讀取 UART
```powershell
# 建立 uart_monitor.ps1
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

@'
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
'@ | Out-File -FilePath "uart_monitor.ps1" -Encoding UTF8

Write-Host "UART 監控腳本已建立：uart_monitor.ps1"
Write-Host "使用方法：.\uart_monitor.ps1 -ComPort COM3"
```

---

## 第七部分：完整的一鍵執行

### 7.1 主要執行腳本
```powershell
# 建立 run_all.ps1 - 一鍵完成所有操作
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

@'
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
    .\flash.ps1 -Verify
    if ($LASTEXITCODE -ne 0) { exit 1 }
}

# UART 監控
if ($MonitorUART) {
    Write-Host "`n啟動 UART 監控..." -ForegroundColor Yellow
    .\uart_monitor.ps1 -ComPort $ComPort
}

Write-Host "`n=== 所有操作完成 ===" -ForegroundColor Cyan
Write-Host "你的 XMC4800 程式已經在運行了！" -ForegroundColor Green
'@ | Out-File -FilePath "run_all.ps1" -Encoding UTF8
```

---

## 第八部分：使用指南

### 8.1 基本使用
```powershell
# 進入專案目錄
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

# 一鍵完成編譯和燒錄
.\run_all.ps1

# 或者分步執行：
# 1. 只編譯
.\build.ps1

# 2. 只燒錄
.\flash.ps1

# 3. 完整測試
.\test.ps1

# 4. 監控 UART
.\uart_monitor.ps1 -ComPort COM3
```

### 8.2 常見操作
```powershell
# 清理重新編譯
.\build.ps1 -Clean

# 燒錄但不驗證（更快）
.\flash.ps1 -Verify:$false

# 編譯+燒錄+UART監控 一次完成
.\run_all.ps1 -MonitorUART -ComPort COM3

# 只燒錄，跳過編譯
.\run_all.ps1 -SkipBuild
```

---

## 第九部分：故障排除

### 9.1 編譯問題
```powershell
# 如果編譯失敗，檢查錯誤：

# 1. 確認 main.c 存在且完整
Get-Content main.c | Select-Object -First 10

# 2. 檢查 DAVE 生成的檔案
dir Dave\Generated\

# 3. 清理後重新編譯
cd Debug
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" clean
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all
```

### 9.2 燒錄問題
```powershell
# 如果燒錄失敗：

# 1. 檢查硬體連接
Write-Host "請確認："
Write-Host "- USB 線連接正常"
Write-Host "- 板上電源 LED 亮起"
Write-Host "- 沒有其他程式使用 J-Link"

# 2. 測試 J-Link 連接
@"
connect
q
"@ | Out-File -FilePath "test.txt" -Encoding ASCII

& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -CommanderScript test.txt
```

### 9.3 功能問題
```powershell
# 如果程式不工作：

# 1. 檢查 LED 狀態
Write-Host "LED 狀態說明："
Write-Host "- LED1 快速閃爍 3 次：初始化成功"
Write-Host "- LED1 規律閃爍：程式正常運行"
Write-Host "- LED1 快速連續閃爍：初始化失敗"
Write-Host "- LED2 間歇閃爍：通訊活動"

# 2. 檢查 UART 輸出
Write-Host "UART 設定："
Write-Host "- 鮑率：115200"
Write-Host "- 資料位元：8"
Write-Host "- 同位位元：None"
Write-Host "- 停止位元：1"
```

---

## 第十部分：檔案清單

### 10.1 你應該有的所有檔案
```powershell
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

# 檢查所有必要檔案
$RequiredFiles = @(
    "main.c",
    "build.ps1",
    "flash.ps1", 
    "test.ps1",
    "run_all.ps1",
    "uart_monitor.ps1",
    "Dave\Generated\DAVE.h",
    "Debug\Makefile"
)

Write-Host "檢查必要檔案：" -ForegroundColor Yellow
foreach ($file in $RequiredFiles) {
    if (Test-Path $file) {
        Write-Host "✓ $file" -ForegroundColor Green
    } else {
        Write-Host "✗ $file (缺少)" -ForegroundColor Red
    }
}
```

### 10.2 完整的專案結構
```
C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\
├── main.c                    # 主程式
├── build.ps1                 # 編譯腳本
├── flash.ps1                 # 燒錄腳本
├── test.ps1                  # 測試腳本
├── run_all.ps1               # 一鍵執行腳本
├── uart_monitor.ps1          # UART 監控腳本
├── Dave/
│   └── Generated/
│       ├── DAVE.h            # 主標頭檔
│       ├── DIGITAL_IO/       # LED 控制
│       ├── UART/            # UART 通訊
│       ├── CAN_NODE/        # CAN 通訊
│       └── ...
├── Debug/
│   ├── Makefile             # Make 建構檔
│   ├── XMC4800_CANopen.elf  # 執行檔
│   ├── XMC4800_CANopen.bin  # 燒錄檔
│   └── ...
├── Libraries/               # XMC 函式庫
└── Startup/                # 啟動程式碼
```

---

## 🎯 快速開始指令

**如果你想立即開始，執行這些命令：**

```powershell
# 1. 進入專案目錄
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

# 2. 執行一鍵腳本（編譯+燒錄）
.\run_all.ps1

# 3. 觀察 LED 是否按預期閃爍

# 4. 如果要監控 UART 輸出
.\uart_monitor.ps1 -ComPort COM3
```

**就這麼簡單！你的 XMC4800 程式就能運行了！**

---

## 📞 支援資訊

如果遇到問題：
1. 檢查工具路徑是否正確
2. 確認硬體連接
3. 查看錯誤訊息並對照故障排除章節
4. 確保所有必要檔案都存在

**這份指南包含了所有實際操作所需的工具路徑、命令和腳本，是真正可以執行的完整指南！**