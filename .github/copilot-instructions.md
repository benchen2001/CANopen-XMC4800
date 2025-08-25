# CANopen XMC4800 專案 AI 助手提示

## 專案概述
這是一個在 XMC4800 微控制器上實現 CANopen 協議的專案，使用 DAVE APP 框架開發。

## 核心技術棧
- **硬體**: XMC4800-F144x2048 微控制器
- **開發環境**: DAVE IDE 4.5.0
- **編譯器**: ARM-GCC-49
- **協議棧**: CANopenNode v2.0
- **CAN 速率**: 250 kbps
- **除錯介面**: UART_0 (115200 baud)

## 關鍵檔案說明
- `main.c`: 主應用程式，包含 CANopen 初始化和主循環
- `port/CO_driver_XMC4800.c`: 硬體抽象層，橋接 CANopenNode 與 XMC4800
- `application/OD.c`: CANopen 物件字典
- `Dave/Generated/`: DAVE APP 自動產生的硬體配置檔案

## 重要編程約定

### DAVE API 使用注意事項
```c
// ❌ 錯誤：這個 API 在 DAVE 4.5.0 中不存在
CAN_NODE_MO_UpdateID(&CAN_NODE_0_LMO[buffer->ident], buffer->ident);

// ✅ 正確：使用混合式方法 - 直接硬體暫存器 + DAVE API
mo->can_mo_ptr->MOAR = (buffer->ident & 0x7FF) << CAN_MO_MOAR_ID_Pos;
CAN_NODE_MO_UpdateData(lmo, buffer->data);
```

### 除錯輸出格式
```c
// 使用標準化的除錯輸出格式
Debug_Printf("TX: ID=0x%03X DLC=%d\r\n", buffer->ident, buffer->DLC);
Debug_Printf("RX: ID=0x%03X, DLC=%d\r\n", rcvMsg.ident, rcvMsg.DLC);
Debug_Printf("ERROR: %s failed: %d\r\n", function_name, error_code);
```

### 計時器處理
```c
// CANopen 需要 1ms 精確計時
volatile uint32_t CO_timer1ms = 0;  // 全域計時器變數

// 在 SYSTIMER 中斷中更新
void SYSTIMER_Callback(void) {
    CO_timer1ms++;
}
```

## 常見問題解決

### 問題 1: CAN 無法傳送
檢查順序：
1. DAVE_Init() 是否成功
2. CAN_NODE_0 配置是否正確
3. 硬體連接和波特率設定
4. CO_CANsend() 函數實現

### 問題 2: UART 無輸出
檢查項目：
1. UART_0 初始化狀態
2. Debug_Printf() 函數實現
3. 鮑率設定 (115200)
4. UART 緩衝區大小 (256 字節限制)

### 問題 3: CANopen 初始化失敗
重要檢查：
1. CO_new() 記憶體分配
2. 物件字典 OD 配置
3. Node ID 設定 (1-127 有效範圍)
4. 初始化順序必須遵循 STM32 參考實現

## 編譯和燒錄命令
```powershell
# 編譯
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all

# 燒錄
& "C:\Program Files\SEGGER\JLink\JLink.exe" -CommandFile "C:\prj\AI\CANOpen\flash_canopen.jlink"
```

## CANopen 測試訊息
- **測試 CAN ID**: 0x123 (基本傳輸測試)
- **Emergency**: 0x080 + Node ID
- **TPDO**: 0x180 + Node ID  
- **RPDO**: 0x200 + Node ID
- **SDO TX**: 0x580 + Node ID
- **SDO RX**: 0x600 + Node ID
- **Heartbeat**: 0x700 + Node ID

## 程式碼風格指南
- 使用 4 空格縮排
- 函數名稱使用 snake_case
- 常數使用 UPPER_CASE
- 註解使用中文，程式碼使用英文
- 錯誤處理必須包含詳細的 UART 輸出

## 與 STM32 版本的主要差異
1. **硬體 API**: STM32 HAL vs XMC4800 DAVE
2. **中斷處理**: STM32 回調函數 vs DAVE 中斷處理函數
3. **計時器**: HAL_TIM vs SYSTIMER
4. **記憶體管理**: 基本相同，但需要注意 XMC4800 的限制

## 開發工作流程與約定

### 必須使用的開發環境
- **使用 DAVE APP**: 所有硬體配置必須透過 DAVE APP 進行
- **使用 UART 輸出訊息**: 所有除錯和狀態資訊必須透過 UART_0 輸出

### 標準編譯與燒錄流程
```powershell
# 編譯命令 (固定使用此路徑和工具)
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug" && & "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all

# 燒錄命令 (使用既有的 J-Link 腳本)
& "C:\Program Files\SEGGER\JLink\JLink.exe" -CommandFile "C:\prj\AI\CANOpen\flash_canopen.jlink"
```

### 🚨 嚴格禁止事項
- **不要簡化工作及簡化程式碼**: 保持專業工程師的完整實現
- **不要亂加 .c 檔**: 只修改現有檔案，不新增源檔案
- **不要亂加 batch 檔**: 使用上述標準 PowerShell 命令
- **不要亂建 J-Link 命令檔案**: 使用既有的 `flash_canopen.jlink`

### 專業工程師標準
- **認真研讀程式碼**: 程式碼總量不到 1000 行，必須完全理解
- **保持資深工程師的專業精神**: 
  - 深度分析問題根本原因
  - 提供完整的技術解決方案
  - 考慮系統整體架構和相容性
  - 撰寫清晰的技術文檔和註解

## 重要提醒
- 絕對不要修改 `Dave/Generated/` 目錄下的檔案
- 所有硬體相關修改都應該在 `CO_driver_XMC4800.c` 中進行
- 使用 CAN 分析儀驗證實際傳輸
- 保持 UART 除錯輸出的即時性