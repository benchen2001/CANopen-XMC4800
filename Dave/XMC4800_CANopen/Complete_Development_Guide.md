# XMC4800 CANopen Monitor 完整開發指南

## 目錄
1. [環境設置](#環境設置)
2. [專案建立](#專案建立)
3. [DAVE APPs 配置](#dave-apps-配置)
4. [程式碼實現](#程式碼實現)
5. [編譯建構](#編譯建構)
6. [燒錄部署](#燒錄部署)
7. [測試驗證](#測試驗證)
8. [故障排除](#故障排除)
9. [進階開發](#進階開發)

---

## 環境設置

### 必要軟體
- **DAVE IDE**: 4.5.0.202105191637 (64-bit)
- **ARM-GCC Toolchain**: 4.9 (隨 DAVE IDE 安裝)
- **J-Link Software**: v7.62b 或更新版本
- **Git**: 版本控制 (可選)

### 安裝路徑檢查
```powershell
# 確認 DAVE IDE 安裝路徑
Test-Path "C:\Infineon\DAVE-IDE-4.5.0.202105191637-64bit"

# 確認 ARM-GCC 工具鏈
Test-Path "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin"

# 確認 J-Link 安裝
Test-Path "C:\Program Files\SEGGER\JLink"
```

### 硬體需求
- **開發板**: XMC4800 Relax Kit
- **除錯器**: J-Link Lite-XMC4200 (板載)
- **連接線**: USB Mini-B 線
- **CAN 收發器**: 如需實際 CAN 通訊測試

---

## 專案建立

### 1. 啟動 DAVE IDE
```powershell
& "C:\Infineon\DAVE-IDE-4.5.0.202105191637-64bit\eclipse\DAVE.exe"
```

### 2. 建立新專案
1. **File** → **New** → **DAVE Project**
2. **Project name**: `XMC4800_CANopen`
3. **Target device**: XMC4800-2048
4. **Template**: `Empty Project`
5. **Location**: `C:\prj\AI\CANOpen\Dave`

### 3. 專案結構確認
```
XMC4800_CANopen/
├── Dave/
│   ├── Generated/
│   │   ├── DAVE.c
│   │   ├── DAVE.h
│   │   ├── CLOCK_XMC4/
│   │   ├── CPU_CTRL_XMC4/
│   │   └── ...
│   └── Model/
├── Libraries/
├── Startup/
├── main.c
└── Linker/
```

---

## DAVE APPs 配置

### 1. 添加必要的 APPs

#### DIGITAL_IO (LED 控制)
1. **Add New APP** → **DIGITAL_IO**
2. **Instance Label**: `LED1`
3. **Port**: P5, **Pin**: 9
4. **Direction**: Output
5. **Initial Output Level**: Low

重複添加 `LED2` (根據板卡配置選擇適當腳位)

#### UART (序列通訊)
1. **Add New APP** → **UART**
2. **Instance Label**: `UART_0`
3. **Baud Rate**: 115200
4. **Data Bits**: 8
5. **Parity**: None
6. **Stop Bits**: 1
7. **TX Pin**: P1.5 (或根據板卡配置)
8. **RX Pin**: P1.4 (或根據板卡配置)
9. **TX Interrupt**: Enable (可選)
10. **RX Interrupt**: Enable (可選)

#### GLOBAL_CAN (CAN 全域設定)
1. **Add New APP** → **GLOBAL_CAN**
2. **Instance Label**: `GLOBAL_CAN_0`
3. **Clock Source**: fCAN = fPLL
4. **CAN Frequency**: 120 MHz

#### CAN_NODE (CAN 節點)
1. **Add New APP** → **CAN_NODE**
2. **Instance Label**: `CAN_NODE_0`
3. **Global CAN**: 選擇 `GLOBAL_CAN_0`
4. **Baud Rate**: 500000 (500 kbps)
5. **Sample Point**: 75%
6. **SJW**: 1
7. **TX Pin**: P1.13 (或根據板卡配置)
8. **RX Pin**: P1.12 (或根據板卡配置)

#### Message Object 配置
1. 在 `CAN_NODE_0` 下添加 **Message Object**
2. **Label**: `CAN_NODE_0_LMO_01_Config`
3. **Type**: Transmit
4. **ID Type**: Standard (11-bit)
5. **Message ID**: 0x123
6. **Data Length**: 8
7. **TX Event**: Enable (可選)

### 2. 生成程式碼
1. **Project** → **Generate Code**
2. 確認無錯誤訊息
3. 檢查 `Dave/Generated/` 目錄結構

---

## 程式碼實現

### 1. main.c 基本架構
```c
/*
 * main.c
 * XMC4800 CANopen Monitor - UART & CAN 通訊實現
 */

#include "DAVE.h"

/* 全域變數 */
uint8_t uart_tx_buffer[256];
uint8_t can_tx_data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/* 函數宣告 */
void UART_Debug_Init(void);
void UART_Send_String(const char* str);
void CAN_Communication_Init(void);
void CAN_Send_Test_Data(void);

int main(void)
{
    DAVE_STATUS_t status;
    uint32_t delay_count;
    uint32_t loop_counter = 0;

    // 初始化 DAVE 系統
    status = DAVE_Init();
    
    if (status == DAVE_STATUS_SUCCESS) {
        // 初始化成功指示
        for(int i = 0; i < 3; i++) {
            DIGITAL_IO_SetOutputLow(&LED1);
            for(delay_count = 0; delay_count < 0x3FFFF; delay_count++);
            DIGITAL_IO_SetOutputHigh(&LED1);
            for(delay_count = 0; delay_count < 0x3FFFF; delay_count++);
        }
        
        /* 初始化通訊模組 */
        UART_Debug_Init();
        CAN_Communication_Init();
        
        /* 發送啟動訊息 */
        UART_Send_String("XMC4800 CANopen Monitor 啟動完成\r\n");
        
    } else {
        // 初始化失敗指示
        while(1) {
            DIGITAL_IO_ToggleOutput(&LED1);
            for(delay_count = 0; delay_count < 0xFFFF; delay_count++);
        }
    }
    
    // 主迴圈
    while(1) {
        if(loop_counter % 500000 == 0) {
            DIGITAL_IO_ToggleOutput(&LED2);
            CAN_Send_Test_Data();
            UART_Send_String("CAN 測試封包已發送\r\n");
        }
        
        if(loop_counter % 100000 == 0) {
            DIGITAL_IO_ToggleOutput(&LED1);
        }
        
        loop_counter++;
    }
}

/* 函數實現 */
void UART_Debug_Init(void)
{
    // UART_0 已由 DAVE_Init() 初始化
}

void UART_Send_String(const char* str)
{
    uint16_t len = 0;
    while(str[len] != '\0' && len < 255) len++;
    
    for(uint16_t i = 0; i < len; i++) {
        uart_tx_buffer[i] = (uint8_t)str[i];
    }
    
    UART_Transmit(&UART_0, uart_tx_buffer, len);
}

void CAN_Communication_Init(void)
{
    CAN_NODE_Enable(&CAN_NODE_0);
}

void CAN_Send_Test_Data(void)
{
    CAN_NODE_STATUS_t status;
    
    for(int i = 0; i < 8; i++) {
        can_tx_data[i]++;
    }
    
    status = CAN_NODE_MO_UpdateData(&CAN_NODE_0_LMO_01_Config, can_tx_data);
    if(status == CAN_NODE_STATUS_SUCCESS) {
        CAN_NODE_MO_Transmit(&CAN_NODE_0_LMO_01_Config);
    }
}
```

### 2. 程式碼驗證檢查清單
- [ ] 包含所有必要的標頭檔案
- [ ] 函數宣告與實現一致
- [ ] 全域變數初始化正確
- [ ] 錯誤處理機制完整
- [ ] LED 狀態指示清晰

---

## 編譯建構

### 1. 使用 DAVE IDE 編譯

#### 方法一：圖形介面
1. 在 DAVE IDE 中右鍵點擊專案
2. **Build Project**
3. 檢查 **Console** 視窗的編譯訊息

#### 方法二：命令列編譯
```powershell
# 切換到專案目錄
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

# 使用 Eclipse 無頭模式編譯
& "C:\Infineon\DAVE-IDE-4.5.0.202105191637-64bit\eclipse\eclipsec.exe" `
  -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild `
  -data "C:\prj\AI\CANOpen\Dave" -build XMC4800_CANopen
```

### 2. 使用 Make 編譯

#### 準備 Make 環境
```powershell
# 切換到 Debug 目錄
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"

# 設定 PATH 環境變數
$env:PATH += ";C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin"
```

#### 執行編譯
```powershell
# 清理之前的編譯
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" clean

# 編譯專案
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all
```

### 3. 編譯輸出檢查
```powershell
# 檢查編譯輸出檔案
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"
dir *.elf
dir *.map
dir *.bin
```

預期輸出：
```
XMC4800_CANopen.elf    # 主執行檔
XMC4800_CANopen.map    # 記憶體映射檔
XMC4800_CANopen.bin    # 二進位檔 (燒錄用)
```

### 4. 編譯錯誤處理

#### 常見編譯錯誤
1. **未定義的識別符號**
   ```
   error: 'LED1' undeclared
   ```
   **解決方案**: 檢查 DAVE APP 配置和程式碼生成

2. **未知的類型名稱**
   ```
   error: unknown type name 'uint8_t'
   ```
   **解決方案**: 檢查標頭檔案包含

3. **連結錯誤**
   ```
   undefined reference to 'UART_Transmit'
   ```
   **解決方案**: 確認 DAVE Libraries 正確連結

### 5. 生成燒錄檔案
```powershell
# 生成 Intel HEX 格式
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-objcopy.exe" `
  -O ihex XMC4800_CANopen.elf XMC4800_CANopen.hex

# 生成二進位格式
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-objcopy.exe" `
  -O binary XMC4800_CANopen.elf XMC4800_CANopen.bin
```

---

## 燒錄部署

### 1. 硬體連接
1. 使用 USB Mini-B 線連接 XMC4800 Kit 到電腦
2. 確認板上 LED 亮起 (電源指示)
3. 安裝 J-Link 驅動程式 (如果需要)

### 2. J-Link 連接測試
```powershell
# 測試 J-Link 連接
& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -speed 12000

# 在 J-Link Commander 中執行
# J-Link> connect
# J-Link> halt
# J-Link> q
```

### 3. 建立燒錄腳本

#### 創建 J-Link 命令腳本
```powershell
# 創建燒錄腳本檔案
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

@"
r
h
loadbin Debug\XMC4800_CANopen.bin, 0x0C000000
r
g
qc
"@ | Out-File -FilePath "jlink_flash.txt" -Encoding ASCII
```

#### 高級燒錄腳本 (含驗證)
```powershell
@"
r
h
erase 0x0C000000 0x0C010000
loadbin Debug\XMC4800_CANopen.bin, 0x0C000000
verifybin Debug\XMC4800_CANopen.bin, 0x0C000000
r
g
qc
"@ | Out-File -FilePath "jlink_flash_verify.txt" -Encoding ASCII
```

### 4. 執行燒錄

#### 基本燒錄
```powershell
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"

& "C:\Program Files\SEGGER\JLink\JLink.exe" `
  -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 `
  -CommanderScript jlink_flash.txt
```

#### 批次燒錄腳本
```powershell
# 創建批次燒錄腳本 flash.ps1
@"
# XMC4800 自動燒錄腳本
param(
    [string]`$BinaryFile = "Debug\XMC4800_CANopen.bin"
)

`$JLinkPath = "C:\Program Files\SEGGER\JLink\JLink.exe"
`$Device = "XMC4800-2048"
`$Interface = "SWD"
`$Speed = "12000"
`$FlashAddress = "0x0C000000"

Write-Host "開始燒錄 XMC4800..." -ForegroundColor Green
Write-Host "檔案: `$BinaryFile" -ForegroundColor Yellow

# 檢查檔案是否存在
if (-not (Test-Path `$BinaryFile)) {
    Write-Error "找不到燒錄檔案: `$BinaryFile"
    exit 1
}

# 創建臨時命令腳本
`$TempScript = "temp_flash.txt"
@"
r
h
erase `$FlashAddress `$([String]::Format("0x{0:X8}", [System.IO.FileInfo]`$BinaryFile.Length + 0x0C000000))
loadbin `$BinaryFile, `$FlashAddress
r
g
qc
"@ | Out-File -FilePath `$TempScript -Encoding ASCII

# 執行燒錄
try {
    & `$JLinkPath -device `$Device -if `$Interface -speed `$Speed -autoconnect 1 -CommanderScript `$TempScript
    Write-Host "燒錄完成!" -ForegroundColor Green
} catch {
    Write-Error "燒錄失敗: `$_"
} finally {
    # 清理臨時檔案
    if (Test-Path `$TempScript) {
        Remove-Item `$TempScript
    }
}
"@ | Out-File -FilePath "flash.ps1" -Encoding UTF8

# 執行燒錄
.\flash.ps1
```

### 5. 燒錄驗證

#### 檢查燒錄結果
```powershell
# 讀取記憶體內容驗證
@"
r
h
mem32 0x0C000000 0x10
qc
"@ | Out-File -FilePath "verify.txt" -Encoding ASCII

& "C:\Program Files\SEGGER\JLink\JLink.exe" `
  -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 `
  -CommanderScript verify.txt
```

#### 檢查程式執行狀態
```powershell
# 讀取 PC 暫存器
@"
connect
halt
r
g
qc
"@ | Out-File -FilePath "check_pc.txt" -Encoding ASCII

& "C:\Program Files\SEGGER\JLink\JLink.exe" `
  -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 `
  -CommanderScript check_pc.txt
```

---

## 測試驗證

### 1. LED 狀態測試

#### 預期行為
1. **上電後**: LED1 快速閃爍 3 次 (初始化成功)
2. **正常運行**: LED1 規律閃爍 (心跳指示)
3. **通訊活動**: LED2 間歇閃爍 (活動指示)
4. **錯誤狀態**: LED1 快速連續閃爍

#### 測試步驟
```powershell
# 1. 燒錄程式
.\flash.ps1

# 2. 觀察 LED 行為
Write-Host "請觀察 LED 狀態:"
Write-Host "- LED1 應快速閃爍 3 次然後規律閃爍"
Write-Host "- LED2 應間歇閃爍表示活動"
```

### 2. UART 通訊測試

#### 設定 UART 終端機
```powershell
# 使用 PuTTY 連接 (需要安裝 PuTTY)
& "C:\Program Files\PuTTY\putty.exe" -serial COM3 -sercfg 115200,8,n,1

# 或使用 PowerShell Serial 連接
$port = new-Object System.IO.Ports.SerialPort COM3,115200,None,8,one
$port.Open()
$port.ReadExisting()
```

#### 預期 UART 輸出
```
XMC4800 CANopen Monitor 啟動完成
CAN 測試封包已發送
CAN 測試封包已發送
...
```

### 3. CAN 通訊測試

#### 使用 CAN 分析儀
1. 連接 CAN 分析儀到 CAN_H, CAN_L 腳位
2. 設定 CAN 分析儀: 500 kbps, Standard ID
3. 監控 ID 0x123 的訊息

#### 預期 CAN 訊息
```
ID: 0x123
Data: 02 03 04 05 06 07 08 09 (遞增資料)
Period: ~每 2-3 秒
```

#### 簡易 CAN 測試 (迴路測試)
```c
// 在程式中添加迴路測試代碼
void CAN_Loopback_Test(void)
{
    // 啟用迴路模式
    XMC_CAN_NODE_EnableLoopBack(CAN_NODE_0.node_ptr);
    
    // 配置接收 MO
    // ... 接收配置代碼
}
```

### 4. 系統整合測試

#### 建立測試腳本
```powershell
# 系統整合測試腳本 test.ps1
@"
# XMC4800 系統測試腳本

Write-Host "=== XMC4800 CANopen Monitor 系統測試 ===" -ForegroundColor Cyan

# 1. 編譯測試
Write-Host "1. 編譯測試..." -ForegroundColor Yellow
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all

if (`$LASTEXITCODE -eq 0) {
    Write-Host "   編譯成功 ✓" -ForegroundColor Green
} else {
    Write-Host "   編譯失敗 ✗" -ForegroundColor Red
    exit 1
}

# 2. 燒錄測試
Write-Host "2. 燒錄測試..." -ForegroundColor Yellow
cd ".."
.\flash.ps1

if (`$LASTEXITCODE -eq 0) {
    Write-Host "   燒錄成功 ✓" -ForegroundColor Green
} else {
    Write-Host "   燒錄失敗 ✗" -ForegroundColor Red
    exit 1
}

# 3. 功能測試提示
Write-Host "3. 功能測試..." -ForegroundColor Yellow
Write-Host "   請手動驗證以下項目:"
Write-Host "   - LED1 快速閃爍 3 次後規律閃爍"
Write-Host "   - LED2 間歇閃爍"
Write-Host "   - UART 輸出訊息 (115200, 8N1)"
Write-Host "   - CAN 發送 ID 0x123 (500 kbps)"

Write-Host "=== 測試完成 ===" -ForegroundColor Cyan
"@ | Out-File -FilePath "test.ps1" -Encoding UTF8

# 執行測試
.\test.ps1
```

---

## 故障排除

### 1. 編譯問題

#### 問題：找不到標頭檔案
```
fatal error: 'DAVE.h' file not found
```
**解決方案**:
1. 檢查專案包含路徑設定
2. 重新生成 DAVE 程式碼
3. 清理並重新編譯

#### 問題：未定義的識別符號
```
error: 'LED1' undeclared
```
**解決方案**:
1. 檢查 DIGITAL_IO APP 配置
2. 確認 Instance Label 正確
3. 檢查 extern 宣告檔案

### 2. 燒錄問題

#### 問題：J-Link 連接失敗
```
Could not connect to J-Link
```
**解決方案**:
1. 檢查 USB 連接
2. 重新安裝 J-Link 驅動
3. 檢查板上電源

#### 問題：Flash 燒錄失敗
```
Flash programming failed
```
**解決方案**:
1. 檢查記憶體地址 (0x0C000000)
2. 確認檔案大小不超過 Flash 容量
3. 嘗試 Erase 整個 Flash

### 3. 執行問題

#### 問題：程式不執行
**症狀**: LED 無反應或不按預期閃爍
**檢查步驟**:
1. 確認程式已正確燒錄
2. 檢查 Reset 後程式是否跳轉正確
3. 使用除錯器單步執行

#### 問題：UART 無輸出
**檢查清單**:
- [ ] UART 腳位配置正確
- [ ] 鮑率設定正確 (115200)
- [ ] TX/RX 腳位無短路
- [ ] 終端機設定正確

#### 問題：CAN 無發送
**檢查清單**:
- [ ] CAN 收發器正確連接
- [ ] CAN_H, CAN_L 腳位配置
- [ ] CAN 匯流排終端電阻 (120Ω)
- [ ] CAN 分析儀設定匹配

### 4. 除錯工具使用

#### GDB 除錯
```powershell
# 啟動 GDB 除錯會話
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-gdb.exe" `
  "Debug\XMC4800_CANopen.elf"

# GDB 命令範例
# (gdb) target remote localhost:2331
# (gdb) load
# (gdb) break main
# (gdb) continue
```

#### J-Link GDB Server
```powershell
# 啟動 GDB Server
& "C:\Program Files\SEGGER\JLink\JLinkGDBServerCL.exe" `
  -device XMC4800-2048 -if SWD -speed 12000 -port 2331
```

---

## 進階開發

### 1. 中斷處理實現

#### UART 中斷處理
```c
// 在 main.c 中添加中斷處理函數
void UART_0_TX_Handler(void)
{
    // TX 完成處理
    DIGITAL_IO_ToggleOutput(&LED2);
}

void UART_0_RX_Handler(void)
{
    // RX 資料處理
    uint8_t received_data;
    UART_GetReceivedData(&UART_0, &received_data);
    // 處理接收到的資料
}
```

#### CAN 中斷處理
```c
void CAN_NODE_0_LMO_01_TX_Handler(void)
{
    // CAN TX 完成處理
    CAN_NODE_MO_ClearStatus(&CAN_NODE_0_LMO_01_Config, 
                           XMC_CAN_MO_RESET_STATUS_TX_PENDING);
}

void CAN_NODE_0_LMO_01_RX_Handler(void)
{
    // CAN RX 資料處理
    CAN_NODE_MO_ReceiveData(&CAN_NODE_0_LMO_01_Config);
    CAN_NODE_MO_ClearStatus(&CAN_NODE_0_LMO_01_Config, 
                           XMC_CAN_MO_RESET_STATUS_RX_PENDING);
}
```

### 2. CANopen 協議實現

#### PDO (Process Data Object) 實現
```c
typedef struct {
    uint16_t cob_id;
    uint8_t data[8];
    uint8_t length;
    uint32_t timestamp;
} PDO_Message_t;

void CANopen_Send_PDO1(uint32_t process_data)
{
    uint8_t pdo_data[4];
    pdo_data[0] = (process_data >> 0) & 0xFF;
    pdo_data[1] = (process_data >> 8) & 0xFF;
    pdo_data[2] = (process_data >> 16) & 0xFF;
    pdo_data[3] = (process_data >> 24) & 0xFF;
    
    CAN_NODE_MO_UpdateData(&CAN_NODE_0_LMO_01_Config, pdo_data);
    CAN_NODE_MO_Transmit(&CAN_NODE_0_LMO_01_Config);
}
```

#### SDO (Service Data Object) 實現
```c
typedef struct {
    uint8_t command;
    uint16_t index;
    uint8_t subindex;
    uint32_t data;
} SDO_Message_t;

void CANopen_Process_SDO(SDO_Message_t* sdo)
{
    switch(sdo->command) {
        case 0x40: // SDO Read Request
            // 處理讀取請求
            break;
        case 0x20: // SDO Write Request
            // 處理寫入請求
            break;
    }
}
```

### 3. 效能優化

#### DMA 傳輸實現
```c
// 添加 DMA APP 到 DAVE 專案
// 配置 DMA 用於 UART 大量資料傳輸

void UART_DMA_Send(uint8_t* data, uint16_t length)
{
    // 配置 DMA 通道
    // 啟動 DMA 傳輸
}
```

#### FIFO 緩衝區管理
```c
#define FIFO_SIZE 256

typedef struct {
    uint8_t buffer[FIFO_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} FIFO_t;

void FIFO_Put(FIFO_t* fifo, uint8_t data)
{
    if(fifo->count < FIFO_SIZE) {
        fifo->buffer[fifo->head] = data;
        fifo->head = (fifo->head + 1) % FIFO_SIZE;
        fifo->count++;
    }
}

uint8_t FIFO_Get(FIFO_t* fifo)
{
    uint8_t data = 0;
    if(fifo->count > 0) {
        data = fifo->buffer[fifo->tail];
        fifo->tail = (fifo->tail + 1) % FIFO_SIZE;
        fifo->count--;
    }
    return data;
}
```

### 4. 建構自動化

#### CMake 建構系統
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(XMC4800_CANopen)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# 編譯設定
set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

# 包含路徑
include_directories(
    Dave/Generated
    Dave/Generated/DAVE
    Libraries/XMCLib/inc
    Libraries/CMSIS/Include
    Libraries/CMSIS/Infineon/XMC4800_series/Include
)

# 原始檔案
file(GLOB_RECURSE SOURCES 
    "main.c"
    "Dave/Generated/*.c"
    "Libraries/XMCLib/src/*.c"
    "Startup/*.c"
)

# 連結器腳本
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/Linker/XMC4800x2048.ld")
set(CMAKE_EXE_LINKER_FLAGS "-T${LINKER_SCRIPT}")

# 建立執行檔
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# 生成額外格式
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND arm-none-eabi-objcopy -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND arm-none-eabi-size ${PROJECT_NAME}.elf
)
```

#### PowerShell 建構腳本
```powershell
# build.ps1 - 完整建構腳本
param(
    [string]$BuildType = "Release",
    [switch]$Clean = $false,
    [switch]$Flash = $false,
    [switch]$Test = $false
)

$ProjectPath = "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"
$DebugPath = "$ProjectPath\Debug"
$MakePath = "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe"

Write-Host "=== XMC4800 建構腳本 ===" -ForegroundColor Cyan

# 清理
if ($Clean) {
    Write-Host "清理專案..." -ForegroundColor Yellow
    cd $DebugPath
    & $MakePath clean
}

# 編譯
Write-Host "編譯專案 ($BuildType)..." -ForegroundColor Yellow
cd $DebugPath
& $MakePath all

if ($LASTEXITCODE -ne 0) {
    Write-Error "編譯失敗"
    exit 1
}

# 生成檔案資訊
$ElfFile = "$DebugPath\XMC4800_CANopen.elf"
$BinFile = "$DebugPath\XMC4800_CANopen.bin"

if (Test-Path $ElfFile) {
    $ElfSize = (Get-Item $ElfFile).Length
    Write-Host "ELF 檔案大小: $ElfSize bytes" -ForegroundColor Green
}

if (Test-Path $BinFile) {
    $BinSize = (Get-Item $BinFile).Length
    Write-Host "Binary 檔案大小: $BinSize bytes" -ForegroundColor Green
}

# 燒錄
if ($Flash) {
    Write-Host "燒錄到硬體..." -ForegroundColor Yellow
    cd $ProjectPath
    .\flash.ps1
}

# 測試
if ($Test) {
    Write-Host "執行測試..." -ForegroundColor Yellow
    .\test.ps1
}

Write-Host "建構完成!" -ForegroundColor Green
```

---

## 附錄

### A. 快速參考命令

#### 編譯命令
```powershell
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all
```

#### 燒錄命令
```powershell
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen"
& "C:\Program Files\SEGGER\JLink\JLink.exe" -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 -CommanderScript jlink_flash.txt
```

#### 除錯命令
```powershell
& "C:\Program Files\SEGGER\JLink\JLinkGDBServerCL.exe" -device XMC4800-2048 -if SWD -speed 12000 -port 2331
```

### B. 常用 DAVE APIs

#### DIGITAL_IO
```c
DIGITAL_IO_SetOutputHigh(&LED1);
DIGITAL_IO_SetOutputLow(&LED1);
DIGITAL_IO_ToggleOutput(&LED1);
uint32_t state = DIGITAL_IO_GetInput(&BUTTON);
```

#### UART
```c
UART_STATUS_t UART_Transmit(UART_t* handle, uint8_t* data, uint16_t count);
UART_STATUS_t UART_Receive(UART_t* handle, uint8_t* data, uint16_t count);
UART_STATUS_t UART_StartTransmitIRQ(UART_t* handle, uint8_t* data, uint16_t count);
UART_STATUS_t UART_StartReceiveIRQ(UART_t* handle, uint8_t* data, uint16_t count);
```

#### CAN_NODE
```c
CAN_NODE_STATUS_t CAN_NODE_Init(const CAN_NODE_t *handle);
void CAN_NODE_Enable(const CAN_NODE_t *handle);
void CAN_NODE_Disable(const CAN_NODE_t *handle);
CAN_NODE_STATUS_t CAN_NODE_MO_Transmit(const CAN_NODE_LMO_t *lmo_ptr);
CAN_NODE_STATUS_t CAN_NODE_MO_Receive(CAN_NODE_LMO_t *lmo_ptr);
CAN_NODE_STATUS_t CAN_NODE_MO_UpdateData(const CAN_NODE_LMO_t *lmo_ptr, uint8_t *data);
```

### C. 記憶體映射

#### XMC4800 記憶體佈局
```
0x00000000 - 0x001FFFFF : Flash Memory (2MB)
0x0C000000 - 0x0C1FFFFF : Cached Flash Access
0x1FFE8000 - 0x1FFFFFFF : SRAM (96KB)
0x20000000 - 0x2001FFFF : SRAM (128KB)
0x40000000 - 0x5FFFFFFF : Peripheral Space
```

#### Flash 程式記憶體
```
0x0C000000 : Vector Table
0x0C000400 : Application Code Start
0x0C100000 : Flash End (1MB Application Space)
```

### D. 除錯技巧

#### 使用 printf 除錯
```c
// 重新導向 printf 到 UART
int _write(int file, char *ptr, int len)
{
    UART_Transmit(&UART_0, (uint8_t*)ptr, len);
    return len;
}

// 然後可以使用 printf
printf("Debug: value = %d\n", variable);
```

#### LED 除錯模式
```c
typedef enum {
    DEBUG_INIT_SUCCESS = 3,     // 快速閃爍 3 次
    DEBUG_UART_OK = 5,          // 快速閃爍 5 次
    DEBUG_CAN_OK = 7,           // 快速閃爍 7 次
    DEBUG_ERROR_INIT = 255      // 連續快速閃爍
} DebugCode_t;

void Debug_LED_Signal(DebugCode_t code)
{
    for(int i = 0; i < code && code < 10; i++) {
        DIGITAL_IO_SetOutputHigh(&LED1);
        for(volatile int j = 0; j < 50000; j++);
        DIGITAL_IO_SetOutputLow(&LED1);
        for(volatile int j = 0; j < 50000; j++);
    }
}
```

---

## 版本記錄

| 版本 | 日期 | 修改內容 |
|------|------|----------|
| 1.0  | 2025/8/21 | 初始版本，包含基本 UART 和 CAN 功能 |
| 1.1  | 2025/8/21 | 添加詳細建構和燒錄步驟 |

---

**文件作者**: AI Assistant  
**適用版本**: DAVE IDE 4.5.0, XMC4800-2048  
**最後更新**: 2025年8月21日