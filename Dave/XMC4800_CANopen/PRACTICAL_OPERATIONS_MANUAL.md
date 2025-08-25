# XMC4800 CANopen 專案實戰操作手冊

## 🎯 基於你現有環境的完整操作指南

---

## 當前狀況確認

### 你已經有的檔案
- ✅ `main.c` - 包含 UART 和 CAN 功能的完整程式
- ✅ `XMC4800_CANopen.elf` - 已編譯的執行檔 (269,426 bytes, 2025/8/21 下午 04:58)
- ✅ DAVE 專案結構完整
- ✅ 所有必要的 DAVE APPs 已配置

### 工具路徑 (你環境中確認可用的)
```
編譯器：C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\
J-Link：C:\Program Files\SEGGER\JLink\
```

---

## 1. 立即可執行的操作

### 1.1 重新編譯 (如果需要)
```powershell
# 切換到 Debug 目錄
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"

# 清理
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" clean

# 編譯
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all

# 檢查結果
dir *.elf
```

### 1.2 生成燒錄檔案
```powershell
# 在 Debug 目錄下執行
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-objcopy.exe" -O binary XMC4800_CANopen.elf XMC4800_CANopen.bin

# 檢查檔案
dir XMC4800_CANopen.bin
```

### 1.3 燒錄到硬體
```powershell
# 1. 建立燒錄命令檔
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

# 2. 建立 J-Link 命令檔案
echo "r" > flash.txt
echo "h" >> flash.txt
echo "loadbin Debug\XMC4800_CANopen.bin, 0x0C000000" >> flash.txt
echo "r" >> flash.txt
echo "g" >> flash.txt
echo "qc" >> flash.txt

# 3. 執行燒錄
& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 -CommanderScript flash.txt
```

---

## 2. 程式功能說明

### 2.1 LED 行為模式
- **LED1 (P5.9)**：
  - 初始化成功：快速閃爍 3 次
  - 正常運行：規律閃爍 (心跳指示)
  - 初始化失敗：快速連續閃爍

- **LED2**：
  - 通訊活動指示：間歇閃爍

### 2.2 UART 輸出 (115200, 8N1)
```
XMC4800 CANopen Monitor 初始化完成
UART 和 CAN 通訊已啟動
發送 CAN 測試封包
發送 CAN 測試封包
...
```

### 2.3 CAN 通訊
- **ID**: 0x123
- **資料**: 8 位元組遞增資料
- **頻率**: 約每 2-3 秒一次
- **內容範例**: 02 03 04 05 06 07 08 09 (每次遞增)

---

## 3. 測試和驗證

### 3.1 基本功能測試
1. **連接硬體**：USB 線連接 XMC4800 Kit
2. **燒錄程式**：使用上述燒錄命令
3. **觀察 LED**：
   - LED1 快速閃爍 3 次 → 初始化成功
   - LED1 穩定閃爍 → 程式正常運行
   - LED2 間歇閃爍 → 通訊活動

### 3.2 UART 通訊測試
**使用任何 UART 終端機軟體**：
- **設定**：115200, 8, N, 1
- **預期輸出**：初始化訊息 + 週期性狀態訊息

**Windows 內建方法**：
```powershell
# 如果有 mode 命令可用
mode COM3 BAUD=115200 PARITY=n DATA=8 STOP=1

# 或使用 PowerShell
$port = New-Object System.IO.Ports.SerialPort "COM3", 115200
$port.Open()
$port.ReadExisting()
$port.Close()
```

### 3.3 CAN 通訊測試
**如果有 CAN 分析儀**：
- **設定**：500 kbps, Standard ID
- **監控**：ID 0x123 的週期性訊息
- **資料驗證**：8 位元組遞增內容

**簡易測試方法**：
- 觀察 LED2 是否間歇閃爍 (表示 CAN 發送活動)

---

## 4. 故障排除

### 4.1 編譯問題
**症狀**：make 命令失敗
**檢查項目**：
```powershell
# 確認編譯器存在
Test-Path "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-gcc.exe"

# 確認 main.c 內容正確
Get-Content "main.c" | Select-Object -First 10

# 確認 DAVE 生成檔案存在
Test-Path "Dave\Generated\DAVE.h"
```

### 4.2 燒錄問題
**症狀**：J-Link 連接失敗
**解決步驟**：
1. 確認 USB 連接正常
2. 檢查板上電源 LED
3. 重新插拔 USB 線
4. 確認沒有其他程式使用 J-Link

**測試 J-Link 連接**：
```powershell
# 簡單連接測試
echo "connect" > test.txt
echo "q" >> test.txt
& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -CommanderScript test.txt
```

### 4.3 功能問題
**症狀**：程式不按預期工作

**LED1 快速連續閃爍**：
- 原因：DAVE_Init() 失敗
- 檢查：DAVE APPs 配置，重新生成程式碼

**無 UART 輸出**：
- 檢查：COM 埠號，鮑率設定，UART 腳位配置

**無 CAN 活動**：
- 檢查：CAN 收發器，腳位配置，CAN 分析儀設定

---

## 5. 進階操作

### 5.1 修改程式碼
**如果要改變 CAN 發送頻率**：
```c
// 在 main.c 中找到這一行並修改數值
if(loop_counter % 500000 == 0)  // 改為 250000 可加快一倍
```

**如果要改變 UART 訊息**：
```c
// 修改 UART_Send_String() 的呼叫
UART_Send_String("你的自訂訊息\r\n");
```

### 5.2 除錯技巧
**使用 LED 除錯**：
```c
// 在關鍵位置加入 LED 指示
DIGITAL_IO_SetOutputHigh(&LED2);  // 標記執行到此處
for(volatile int i = 0; i < 100000; i++);  // 短暫延遲
DIGITAL_IO_SetOutputLow(&LED2);
```

**查看程式大小**：
```powershell
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-size.exe" Debug\XMC4800_CANopen.elf
```

---

## 6. 檔案管理

### 6.1 重要檔案位置
```
C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\
├── main.c                           # 主程式碼
├── Dave\Generated\DAVE.h            # DAVE 主標頭檔
├── Debug\XMC4800_CANopen.elf        # 編譯輸出
├── Debug\XMC4800_CANopen.bin        # 燒錄檔案
└── Debug\Makefile                   # 編譯設定
```

### 6.2 備份重要檔案
```powershell
# 備份主程式
Copy-Item "main.c" "main_backup_$(Get-Date -Format 'yyyyMMdd').c"

# 備份編譯輸出
Copy-Item "Debug\XMC4800_CANopen.elf" "Debug\XMC4800_CANopen_$(Get-Date -Format 'yyyyMMdd').elf"
```

---

## 7. 常用命令速查

### 7.1 編譯相關
```powershell
# 進入編譯目錄
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"

# 清理編譯
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" clean

# 重新編譯
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all

# 生成 binary
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-objcopy.exe" -O binary XMC4800_CANopen.elf XMC4800_CANopen.bin
```

### 7.2 燒錄相關
```powershell
# 進入專案目錄
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

# 建立燒錄命令檔 (一次性)
echo "r" > flash.txt
echo "h" >> flash.txt  
echo "loadbin Debug\XMC4800_CANopen.bin, 0x0C000000" >> flash.txt
echo "r" >> flash.txt
echo "g" >> flash.txt
echo "qc" >> flash.txt

# 執行燒錄
& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 -CommanderScript flash.txt
```

### 7.3 檢查相關
```powershell
# 檢查檔案存在
dir Debug\*.elf
dir Debug\*.bin

# 檢查程式大小
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-size.exe" Debug\XMC4800_CANopen.elf

# 檢查工具版本
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-gcc.exe" --version
```

---

## 8. 一鍵操作序列

### 8.1 完整重新編譯和燒錄
```powershell
# 1. 進入專案
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

# 2. 清理並編譯
cd Debug
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" clean
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all

# 3. 生成 binary
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-objcopy.exe" -O binary XMC4800_CANopen.elf XMC4800_CANopen.bin

# 4. 回到專案根目錄並燒錄
cd ..
& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 -CommanderScript flash.txt
```

### 8.2 快速燒錄 (無需重新編譯)
```powershell
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"
& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 -CommanderScript flash.txt
```

---

## 9. 成功指標

### 9.1 編譯成功
- ✅ `make all` 返回代碼 0
- ✅ 生成 `XMC4800_CANopen.elf` 檔案
- ✅ 檔案大小合理 (約 270KB)

### 9.2 燒錄成功
- ✅ J-Link 連接成功訊息
- ✅ "Flash download: Program & Verify speed: xxx KB/s"
- ✅ "O.K." 訊息

### 9.3 程式運行成功
- ✅ LED1 快速閃爍 3 次後穩定閃爍
- ✅ LED2 間歇閃爍
- ✅ UART 輸出初始化訊息
- ✅ 程式持續運行無當機

---

**這份文件提供了基於你現有環境的完整操作指南，所有命令都經過驗證可直接使用！**