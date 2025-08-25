# XMC4800 CANopen Monitor - UART & CAN 通訊實現

## 功能描述

本程式實現了基於 DAVE Libraries 的 XMC4800 CANopen 監控器，具備 UART 和 CAN 通訊功能。

## 主要功能

### 1. LED 狀態指示
- **LED1 (P5.9)**：系統狀態指示
  - 初始化成功：快速閃爍 3 次
  - 正常運行：穩定閃爍
  - 初始化失敗：快速連續閃爍

- **LED2**：通訊活動指示
  - 閃爍表示通訊活動

### 2. UART 通訊 (UART_0)
- **功能**：除錯訊息輸出和狀態報告
- **設定**：根據 DAVE 配置（通常 115200 baud, 8N1）
- **輸出訊息**：
  - 系統初始化完成通知
  - CAN 發送狀態報告
  - 週期性狀態更新

### 3. CAN 通訊 (CAN_NODE_0)
- **功能**：CAN 匯流排通訊
- **設定**：根據 DAVE 配置的 CAN 節點
- **Message Object**：CAN_NODE_0_LMO_01_Config
- **測試資料**：8 位元組遞增資料 {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}

## 程式流程

### 初始化階段
1. 呼叫 `DAVE_Init()` 初始化所有 DAVE APPs
2. LED1 快速閃爍 3 次確認初始化成功
3. 初始化 UART 通訊
4. 初始化 CAN 通訊並啟用節點
5. 發送初始化完成訊息到 UART

### 主迴圈
1. **每 500,000 次迴圈**：
   - LED2 閃爍表示活動
   - 發送 CAN 測試資料（遞增）
   - 發送 UART 狀態訊息

2. **每 100,000 次迴圈**：
   - LED1 切換狀態（心跳指示）

## 函數說明

### `UART_Debug_Init()`
- 初始化 UART 通訊（UART_0 已由 DAVE_Init() 初始化）

### `UART_Send_String(const char* str)`
- 發送字串到 UART
- 使用阻塞式傳輸確保資料完整

### `CAN_Communication_Init()`
- 啟用 CAN 節點
- 設定 CAN 通訊參數

### `CAN_Send_Test_Data()`
- 更新測試資料（遞增）
- 發送 8 位元組測試資料到 CAN 匯流排

## 硬體需求

- **開發板**：XMC4800 Relax Kit
- **除錯器**：J-Link（板載 J-Link Lite-XMC4200）
- **連接**：
  - UART：根據 DAVE 配置的 TX/RX 腳位
  - CAN：根據 DAVE 配置的 CAN TX/RX 腳位
  - LED：P5.9 (LED1), LED2（根據板卡配置）

## 編譯和燒錄

### 編譯
```bash
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"
make all
```

### 生成 Binary
```bash
arm-none-eabi-objcopy -O binary XMC4800_CANopen.elf XMC4800_CANopen.bin
```

### 燒錄
```bash
JLink.exe -device XMC4800-2048 -if SWD -speed 12000 -autoconnect 1 -CommanderScript jlink_script.txt
```

## 驗證測試

1. **系統初始化**：觀察 LED1 快速閃爍 3 次
2. **UART 通訊**：連接 UART 終端機檢查除錯訊息
3. **CAN 通訊**：使用 CAN 分析儀監控 CAN 匯流排流量
4. **LED 狀態**：確認 LED1 穩定閃爍，LED2 活動指示

## 故障排除

- **LED1 快速連續閃爍**：DAVE 初始化失敗，檢查硬體連接
- **無 UART 輸出**：檢查 UART 配置和連接
- **無 CAN 活動**：檢查 CAN 收發器和匯流排配置

## 技術特點

- 使用正確的 DAVE Libraries API
- 避免直接記憶體映射操作
- 實現可靠的系統初始化檢查
- 提供清晰的狀態指示
- 支援 UART 和 CAN 雙重通訊

## 版本資訊

- **DAVE IDE**：4.5.0.202105191637
- **ARM-GCC**：4.9
- **目標設備**：XMC4800-2048
- **編譯時間**：2025/8/21 下午 04:58