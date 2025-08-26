# XMC4800 CANopen 專業開發規劃文件

## 專案概述

基於深入研究 CANopenNode 及 CanOpenSTM32 範例，為 XMC4800 + DAVE 平台制定的專業級 CANopen slave 開發計畫。目標是實現與 TwinCAT + EL6751 master 完全相容的工業級 CANopen 節點。

## 1. 架構分析與技術決策

### 1.1 STM32 參考實現分析

根據對 `CanOpenSTM32` 專案的深度分析，發現以下關鍵架構模式：

#### 核心檔案結構
```
Dave/XMC4800_CANopen/
├── CO_app_XMC4800.h/c              (應用層介面) 【新建】
├── port/CO_driver_XMC4800.c        (硬體抽象層) ✅ 已存在
├── application/OD.h/OD.c           (物件字典) ✅ 已存在
└── application/XMC4800_profile.eds (EDS設定檔) 【新建】
```

**對應 STM32 參考實現**:
```
CO_app_STM32.h/c     → CO_app_XMC4800.h/c     (應用層介面)
CO_driver_STM32.c    → CO_driver_XMC4800.c    (硬體抽象層) 
OD.h/OD.c           → OD.h/OD.c              (物件字典)
DS301_profile.eds   → XMC4800_profile.eds    (EDS設定檔)
```

#### 關鍵設計模式
1. **分層架構**: 應用層 → CANopen協議層 → 硬體抽象層
2. **配置驅動**: 使用 EDS 檔案自動生成 OD.h/OD.c
3. **中斷處理**: 1ms 精確計時器 + CAN 中斷處理
4. **狀態管理**: LED 狀態反饋 + 錯誤處理機制

### 1.2 XMC4800 + DAVE 適配策略

#### DAVE API 完全整合方案
```c
// STM32 HAL 風格
HAL_CAN_Start(&hcan);
HAL_TIM_Base_Start_IT(&htim);

// XMC4800 DAVE 風格  
CAN_NODE_Start(&CAN_NODE_0);
SYSTIMER_Start(&SYSTIMER_0);
```

#### 關鍵適配點
1. **CAN 介面**: HAL_CAN → CAN_NODE_0 APP
2. **計時器**: HAL_TIM → SYSTIMER_0 APP  
3. **中斷處理**: HAL callbacks → DAVE interrupt handlers
4. **初始化**: CubeMX → DAVE Project

## 2. 物件字典 (OD) 專業設計

### 2.1 基於 DS301 標準的 OD 結構

參考 `DS301_profile.eds` 的標準實現：

#### 必要物件 (Mandatory Objects)
```ini
[1000] Device type
[1001] Error register  
[1018] Identity object
```

#### RPDO 配置 (接收 PDO) - 4個
```ini
[1400] RPDO1 通訊參數
[1401] RPDO2 通訊參數  
[1402] RPDO3 通訊參數
[1403] RPDO4 通訊參數

[1600] RPDO1 映射參數
[1601] RPDO2 映射參數
[1602] RPDO3 映射參數  
[1603] RPDO4 映射參數
```

#### TPDO 配置 (傳送 PDO) - 4個
```ini
[1800] TPDO1 通訊參數
[1801] TPDO2 通訊參數
[1802] TPDO3 通訊參數  
[1803] TPDO4 通訊參數

[1A00] TPDO1 映射參數
[1A01] TPDO2 映射參數
[1A02] TPDO3 映射參數
[1A03] TPDO4 映射參數
```

### 2.2 CAN ID 分配方案

基於 DS301 標準的 COB-ID 計算：

```c
// RPDO COB-IDs (Master → Slave)
#define RPDO1_COBID    (0x200 + NodeID)
#define RPDO2_COBID    (0x300 + NodeID)  
#define RPDO3_COBID    (0x400 + NodeID)
#define RPDO4_COBID    (0x500 + NodeID)

// TPDO COB-IDs (Slave → Master)  
#define TPDO1_COBID    (0x180 + NodeID)
#define TPDO2_COBID    (0x280 + NodeID)
#define TPDO3_COBID    (0x380 + NodeID) 
#define TPDO4_COBID    (0x480 + NodeID)

// 標準服務 COB-IDs
#define SDO_TX_COBID   (0x580 + NodeID)  // SDO Response
#define SDO_RX_COBID   (0x600 + NodeID)  // SDO Request
#define EMCY_COBID     (0x080 + NodeID)  // Emergency
#define HEARTBEAT_COBID (0x700 + NodeID) // Heartbeat
```

### 2.3 DAVE CAN_NODE LMO 映射

根據先前的 LMO 配置指南，實現標準 CANopen 服務：

```c
typedef enum {
    LMO_SDO_RX = 0,        // 0x600 + NodeID
    LMO_SDO_TX,            // 0x580 + NodeID  
    LMO_EMCY,              // 0x080 + NodeID
    LMO_HEARTBEAT,         // 0x700 + NodeID
    LMO_RPDO1,             // 0x200 + NodeID
    LMO_RPDO2,             // 0x300 + NodeID
    LMO_RPDO3,             // 0x400 + NodeID
    LMO_RPDO4,             // 0x500 + NodeID
    LMO_TPDO1,             // 0x180 + NodeID
    LMO_TPDO2,             // 0x280 + NodeID  
    LMO_TPDO3,             // 0x380 + NodeID
    LMO_TPDO4,             // 0x480 + NodeID
    LMO_COUNT = 12
} CANopen_LMO_t;
```

## 3. 專業實現架構

### 3.1 應用層介面設計 (CO_app_XMC4800.h/c)

參考 `CO_app_STM32.c` 的專業架構：

```c
typedef struct {
    uint8_t desiredNodeID;      // 期望的節點 ID
    uint8_t activeNodeID;       // 實際分配的節點 ID  
    uint16_t baudrate;          // CAN 波特率 (kbps)
    
    // DAVE APP 控制項
    CAN_NODE_t* canHandle;      // &CAN_NODE_0
    SYSTIMER_t* timerHandle;    // &SYSTIMER_0
    void (*HWInitFunction)();   // DAVE_Init
    
    // 狀態 LED 輸出
    uint8_t outStatusLEDGreen;  // 綠色 LED 狀態
    uint8_t outStatusLEDRed;    // 紅色 LED 狀態
    
    CO_t* canOpenStack;         // CANopen 協議棧指標
} CANopenNodeXMC4800;

// 核心 API 函數
int canopen_app_init(CANopenNodeXMC4800* canopenXMC4800);
int canopen_app_resetCommunication(void);
void canopen_app_process(void);
void canopen_app_interrupt(void);
```

### 3.2 硬體抽象層實現 (CO_driver_XMC4800.c)

基於先前開發的 DAVE 完全整合版本，實現以下關鍵函數：

```c
// CANopen 標準介面 → DAVE API 適配
CO_ReturnError_t CO_CANmodule_init(...);
CO_ReturnError_t CO_CANsend(CO_CANmodule_t* CANmodule, CO_CANtx_t* buffer);
void CO_CANinterrupt(CO_CANmodule_t* CANmodule);
void CO_CANsetConfigurationMode(void* CANptr);
void CO_CANsetNormalMode(CO_CANmodule_t* CANmodule);
```

#### DAVE API 完全整合模式
```c
// 配置驅動的 LMO 管理
static void configure_canopen_lmo(uint8_t lmo_index, uint32_t can_id, uint8_t dlc) {
    const CAN_NODE_LMO_t* lmo = &CAN_NODE_0.lmobj_ptr[lmo_index];
    
    // 使用 DAVE 配置資料
    XMC_CAN_MO_SetIdentifier(lmo->mo_ptr, can_id);
    XMC_CAN_MO_SetDataLengthCode(lmo->mo_ptr, dlc);
    XMC_CAN_MO_Enable(lmo->mo_ptr);
}

// 動態 Node ID 更新
static void update_canopen_node_id(uint8_t node_id) {
    for (int i = 0; i < LMO_COUNT; i++) {
        uint32_t base_id = get_base_can_id(i);
        configure_canopen_lmo(i, base_id + node_id, 8);
    }
}
```

### 3.3 中斷處理與計時管理

參考 STM32 的專業計時架構：

```c
// 1ms 精確計時器中斷 (SYSTIMER_0)
void SYSTIMER_Callback(void) {
    canopen_app_interrupt();
}

// CAN 接收中斷處理
void CAN_NODE_0_IRQHandler(void) {
    CO_CANinterrupt(canopen_stack->CANmodule);
}

// 主循環處理
void canopen_app_process(void) {
    static uint32_t time_old = 0;
    uint32_t time_current = SYSTIMER_GetTime();
    
    if ((time_current - time_old) >= 1) { // 1ms minimum
        uint32_t timeDifference_us = (time_current - time_old) * 1000;
        time_old = time_current;
        
        CO_NMT_reset_cmd_t reset_status = CO_process(CO, false, timeDifference_us, NULL);
        
        // 處理復位命令
        if (reset_status == CO_RESET_COMM) {
            canopen_app_resetCommunication();
        } else if (reset_status == CO_RESET_APP) {
            NVIC_SystemReset(); // 重啟 XMC4800
        }
    }
}
```

## 4. TwinCAT 整合測試策略

### 4.1 EDS 檔案準備

為 TwinCAT 準備標準的 XMC4800_profile.eds：

```ini
[DeviceInfo]
VendorName=Infineon Technologies
VendorNumber=0x00000042
ProductName=XMC4800 CANopen Node
ProductNumber=0x4800
RevisionNumber=0x00010001
BaudRate_125=1
BaudRate_250=1  
BaudRate_500=1
NrOfRXPDO=4
NrOfTXPDO=4
LSS_Supported=1
```

### 4.2 測試階段規劃

#### Phase 1: 基本連接測試
- Heartbeat 訊號確認
- LSS (Layer Setting Services) 功能
- SDO 讀寫測試

#### Phase 2: PDO 通訊測試  
- 4 RPDO 接收測試
- 4 TPDO 傳送測試
- PDO 映射參數驗證

#### Phase 3: 工業應用測試
- Emergency 錯誤處理
- Network Management 狀態機
- 長時間穩定性測試

### 4.3 除錯與監控工具

```c
// UART 除錯輸出 (115200 baud)
#define DEBUG_PRINTF(fmt, ...) \
    UART_Transmit(&UART_0, printf_buffer, \
    snprintf(printf_buffer, 256, fmt, ##__VA_ARGS__))

// CANopen 狀態監控
void canopen_status_monitor(void) {
    DEBUG_PRINTF("NodeID: %d, State: %s\r\n", 
                 active_node_id, nmt_state_string[CO->NMT->operatingState]);
    DEBUG_PRINTF("Heartbeat: %d ms\r\n", CO->NMT->HBproducerTime_ms);
    DEBUG_PRINTF("Error: 0x%02X\r\n", CO->em->errorRegister);
}
```

## 5. 開發工作流程

### 5.1 實現順序

1. **基礎架構建立** (1-2天)
   - 建立 CO_app_XMC4800.h/c 檔案
   - 整合現有的 CO_driver_XMC4800.c
   - 基本 OD.h/OD.c 結構

2. **CANopen 協議整合** (2-3天)  
   - CANopenNode 庫整合
   - PDO 配置與測試
   - SDO 服務實現

3. **TwinCAT 整合測試** (2-3天)
   - EDS 檔案準備
   - TwinCAT 配置
   - 端到端通訊測試

### 5.2 品質保證

#### 程式碼標準
- **100% DAVE API 使用**: 不得使用直接暫存器操作
- **專業錯誤處理**: 所有 API 呼叫必須檢查返回值
- **完整 UART 除錯**: 關鍵狀態變化必須輸出
- **記憶體管理**: 動態記憶體分配必須檢查成功

#### 測試標準
- **功能完整性**: 所有 DS301 必要功能
- **時序準確性**: 1ms 計時精度 ±50μs
- **穩定性**: 24小時連續運行無錯誤
- **相容性**: TwinCAT + EL6751 完全相容

## 6. 專案交付成果

### 6.1 核心檔案
```
Dave/XMC4800_CANopen/
├── main.c                    (主程式整合)
├── CO_app_XMC4800.h/c       (應用層介面)  
├── CO_driver_XMC4800.c      (硬體抽象層)
├── OD.h/OD.c               (物件字典)
├── XMC4800_profile.eds     (EDS設定檔)
└── Generated/              (DAVE自動生成)
```

### 6.2 文件交付
- **API 參考手冊**: 完整函數說明
- **TwinCAT 整合指南**: 逐步配置說明  
- **測試報告**: 完整功能驗證
- **部署指南**: 生產環境配置

### 6.3 品質指標
- **程式碼覆蓋率**: >95%
- **DAVE API 純度**: 100%
- **記憶體使用**: <64KB Flash, <16KB RAM
- **即時性能**: 1ms 響應時間保證

## 7. 技術風險與緩解策略

### 7.1 關鍵風險點
1. **DAVE API 限制**: 某些 CANopen 功能可能無法完全對應
2. **記憶體限制**: XMC4800 記憶體限制可能影響 CANopenNode
3. **時序精度**: SYSTIMER 精度可能不足 1ms 需求
4. **TwinCAT 相容性**: EDS 檔案格式相容性問題

### 7.2 緩解策略
1. **混合式實現**: DAVE API 優先，必要時使用 XMC_LIB
2. **記憶體優化**: 精簡 OD 配置，移除非必要功能
3. **高精度計時**: 使用 CCU4/CCU8 補強時序精度
4. **標準符合**: 嚴格遵循 DS301/DS401 標準

## 8. 總結

本規劃文件基於對 CANopenNode 和 CanOpenSTM32 的深度研究，結合 XMC4800 + DAVE 平台特性，提供了完整的專業級 CANopen slave 開發方案。

**核心優勢**:
- ✅ **工業級標準**: 完全符合 DS301 規範
- ✅ **DAVE 完全整合**: 100% DAVE API 實現
- ✅ **TwinCAT 相容**: 支援主流工業自動化平台
- ✅ **專業架構**: 參考 STM32 成熟實現
- ✅ **完整測試**: 端到端驗證方案

這個規劃將確保 XMC4800 CANopen 節點達到工業級品質標準，可直接應用於生產環境。