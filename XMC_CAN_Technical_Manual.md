# XMC CAN 庫技術手冊
## XMC4800 CAN 控制器完整應用指南

**版本**: 1.0  
**日期**: 2025年8月25日  
**作者**: XMC4800 CANopen 開發團隊  

---

## 📋 目錄

1. [概述](#概述)
2. [架構與特性](#架構與特性)
3. [數據結構](#數據結構)
4. [API 函數詳解](#api-函數詳解)
5. [初始化流程](#初始化流程)
6. [應用範例](#應用範例)
7. [進階功能](#進階功能)
8. [故障排除](#故障排除)

---

## 🎯 概述

XMC CAN 庫是 Infineon XMC4800 微控制器的 Controller Area Network (CAN) 外設驅動程式庫。它提供完整的 CAN 2.0B 規範支援，包括標準和擴展 ID 格式、FIFO 緩衝、閘道模式等高級功能。

### 關鍵特性

- **完整 CAN 2.0B 支援**: 11-bit 標準 ID 和 29-bit 擴展 ID
- **多節點架構**: 支援最多 3 個 CAN 節點
- **Message Objects**: 64 個可配置的消息物件
- **FIFO 支援**: 發送和接收 FIFO 緩衝區
- **閘道模式**: 節點間消息路由
- **靈活的中斷系統**: 8 個 Service Request 線路
- **位時序分析**: 內建波特率檢測和分析功能

---

## 🏗️ 架構與特性

### CAN 模組架構

```
XMC4800 CAN 模組
├── CAN Global Controller
│   ├── 時鐘管理
│   ├── Panel Controller
│   └── Message Object 分配
├── CAN Node 0-2
│   ├── 位時序配置
│   ├── 錯誤處理
│   ├── 幀計數器
│   └── 中斷管理
└── Message Objects (0-63)
    ├── 發送 MO
    ├── 接收 MO
    ├── FIFO MO
    └── 閘道 MO
```

### 五大功能模塊

1. **CAN Global**: 模組級別控制和配置
2. **CAN Node**: 節點級別的通信控制
3. **Message Object**: 消息存儲和傳輸
4. **FIFO**: 消息佇列管理
5. **Gateway**: 節點間消息路由

---

## 📊 數據結構

### 核心數據結構

#### 1. XMC_CAN_MO_t - Message Object 結構

```c
typedef struct XMC_CAN_MO
{
    CAN_MO_TypeDef *can_mo_ptr;        // MO 硬體暫存器指針
    
    // 消息標識符配置
    union {
        struct {
            uint32_t can_identifier: 29;    // 11/29-bit ID
            uint32_t can_id_mode: 1;        // 標準/擴展模式
            uint32_t can_priority: 2;       // 仲裁優先級
        };
        uint32_t mo_ar;
    };
    
    // 接收過濾器配置
    union {
        struct {
            uint32_t can_id_mask: 29;       // ID 過濾遮罩
            uint32_t can_ide_mask: 1;       // IDE 位元遮罩
        };
        uint32_t mo_amr;
    };
    
    uint8_t can_data_length;                // 數據長度 (0-8)
    
    // 數據存儲 (多種存取方式)
    union {
        uint8_t can_data_byte[8];           // 位元組存取
        uint16_t can_data_word[4];          // 字元存取
        uint32_t can_data[2];               // 雙字元存取
        uint64_t can_data_long;             // 長字元存取
    };
    
    XMC_CAN_MO_TYPE_t can_mo_type;         // 發送/接收類型
} XMC_CAN_MO_t;
```

#### 2. XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t - 位時序配置

```c
typedef struct XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG
{
    uint32_t can_frequency;        // CAN 時鐘頻率
    uint32_t baudrate;            // 波特率 (bps)
    uint16_t sample_point;        // 取樣點位置 (%)
    uint16_t sjw;                 // 同步跳躍寬度
} XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t;
```

#### 3. XMC_CAN_FIFO_CONFIG_t - FIFO 配置

```c
typedef struct XMC_CAN_FIFO_CONFIG
{
    uint8_t fifo_bottom;          // FIFO 底部 MO 索引
    uint8_t fifo_base;            // FIFO 基礎 MO 索引
    uint8_t fifo_top;             // FIFO 頂部 MO 索引
    uint8_t fifo_pointer;         // FIFO 當前指針
    bool fifo_data_frame_send;    // 數據幀發送模式
} XMC_CAN_FIFO_CONFIG_t;
```

---

## 🔧 API 函數詳解

### Global 層級 API

#### XMC_CAN_Init()
**功能**: 初始化 CAN 模組全域設定  
**原型**: 
```c
void XMC_CAN_Init(XMC_CAN_t *const obj, XMC_CAN_CANCLKSRC_t clksrc, uint32_t can_frequency)
```

**參數**:
- `obj`: CAN 全域控制器指針
- `clksrc`: 時鐘來源選擇
- `can_frequency`: CAN 模組頻率

**應用範例**:
```c
// 初始化 CAN 模組，使用內部時鐘，頻率 144MHz
XMC_CAN_Init(&MODULE_CAN, XMC_CAN_CANCLKSRC_FPERI, 144000000);
```

#### XMC_CAN_AllocateMOtoNodeList()
**功能**: 將 Message Object 分配給指定節點  
**原型**:
```c
void XMC_CAN_AllocateMOtoNodeList(XMC_CAN_t *const obj, 
                                  const uint8_t node_num, 
                                  const uint8_t mo_num)
```

**應用範例**:
```c
// 將 MO32-35 分配給 CAN Node 1
for (uint8_t mo = 32; mo <= 35; mo++) {
    XMC_CAN_AllocateMOtoNodeList(&MODULE_CAN, 1, mo);
}
```

### Node 層級 API

#### XMC_CAN_NODE_NominalBitTimeConfigure()
**功能**: 配置 CAN 節點的位時序  
**原型**:
```c
XMC_CAN_STATUS_t XMC_CAN_NODE_NominalBitTimeConfigure(
    XMC_CAN_NODE_t *const can_node,
    const XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t *const can_bit_time
)
```

**應用範例**:
```c
// 配置 250Kbps，取樣點 80%
XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t bit_time = {
    .can_frequency = 144000000,
    .baudrate = 250000,
    .sample_point = 8000,  // 80.00%
    .sjw = 1
};

XMC_CAN_NODE_NominalBitTimeConfigure(CAN_NODE1, &bit_time);
```

#### XMC_CAN_NODE_EnableLoopBack()
**功能**: 啟用環回模式 (用於測試)  
**應用範例**:
```c
// 啟用內部環回測試
XMC_CAN_NODE_EnableLoopBack(CAN_NODE1);
```

#### XMC_CAN_NODE_SetReceiveInput()
**功能**: 設定接收輸入信號  
**應用範例**:
```c
// 設定從 RXDC 輸入接收 CAN 信號
XMC_CAN_NODE_SetReceiveInput(CAN_NODE1, XMC_CAN_NODE_RECEIVE_INPUT_RXDCC);
```

### Message Object API

#### XMC_CAN_MO_Config()
**功能**: 配置 Message Object  
**原型**:
```c
XMC_CAN_STATUS_t XMC_CAN_MO_Config(const XMC_CAN_MO_t *const can_mo)
```

**完整應用範例**:
```c
// 配置發送 Message Object
XMC_CAN_MO_t tx_mo = {
    .can_mo_ptr = CAN_MO35,              // 使用 MO35
    .can_identifier = 0x123,             // CAN ID = 0x123
    .can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS,
    .can_priority = XMC_CAN_ARBITRATION_MODE_IDE_DIR_BASED_PRIO_2,
    .can_id_mask = 0x7FF,                // 完全匹配
    .can_ide_mask = 1,
    .can_data_length = 8,                // 8 字節數據
    .can_mo_type = XMC_CAN_MO_TYPE_TRANSMSGOBJ
};

// 設定發送數據
tx_mo.can_data[0] = 0x12345678;          // 低位 4 字節
tx_mo.can_data[1] = 0x9ABCDEF0;          // 高位 4 字節

// 配置 MO
XMC_CAN_MO_Config(&tx_mo);

// 配置接收 Message Object
XMC_CAN_MO_t rx_mo = {
    .can_mo_ptr = CAN_MO32,              // 使用 MO32
    .can_identifier = 0x60A,             // CANopen SDO
    .can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS,
    .can_id_mask = 0x7FF,                // 精確匹配
    .can_ide_mask = 1,
    .can_data_length = 8,
    .can_mo_type = XMC_CAN_MO_TYPE_RECMSGOBJ
};

XMC_CAN_MO_Config(&rx_mo);
```

#### XMC_CAN_MO_Transmit()
**功能**: 發送 CAN 訊息  
**應用範例**:
```c
// 更新數據並發送
tx_mo.can_data_byte[0] = 0x01;
tx_mo.can_data_byte[1] = 0x02;
tx_mo.can_data_byte[2] = 0x03;
tx_mo.can_data_byte[3] = 0x04;

XMC_CAN_STATUS_t status = XMC_CAN_MO_Transmit(&tx_mo);
if (status == XMC_CAN_STATUS_SUCCESS) {
    printf("發送成功!\n");
}
```

#### XMC_CAN_MO_Receive()
**功能**: 接收 CAN 訊息  
**應用範例**:
```c
// 檢查是否有新數據
if (XMC_CAN_MO_GetStatus(&rx_mo) & XMC_CAN_MO_STATUS_RX_PENDING) {
    // 讀取接收數據
    XMC_CAN_MO_Receive(&rx_mo);
    
    printf("接收到數據: ID=0x%03X, DLC=%d\n", 
           rx_mo.can_identifier, rx_mo.can_data_length);
    
    // 清除接收標誌
    XMC_CAN_MO_ResetStatus(&rx_mo, XMC_CAN_MO_RESET_STATUS_RX_PENDING);
}
```

### 中斷管理 API

#### XMC_CAN_MO_SetEventNodePointer()
**功能**: 設定 MO 事件的 Service Request 路由  
**應用範例**:
```c
// 將接收事件路由到 Service Request 3
XMC_CAN_MO_SetEventNodePointer(&rx_mo, 
                               XMC_CAN_MO_POINTER_EVENT_RECEIVE, 
                               3);

// 啟用接收事件
XMC_CAN_MO_EnableEvent(&rx_mo, XMC_CAN_MO_EVENT_RECEIVE);
```

#### XMC_CAN_NODE_EnableEvent()
**功能**: 啟用節點級別事件  
**應用範例**:
```c
// 啟用節點錯誤事件
XMC_CAN_NODE_EnableEvent(CAN_NODE1, XMC_CAN_NODE_EVENT_LEC);
XMC_CAN_NODE_EnableEvent(CAN_NODE1, XMC_CAN_NODE_EVENT_ALERT);
```

---

## 🚀 初始化流程

### 標準初始化順序

```c
void CAN_Initialize(void) 
{
    // 1. 初始化 CAN 模組
    XMC_CAN_Init(&MODULE_CAN, XMC_CAN_CANCLKSRC_FPERI, 144000000);
    
    // 2. 設定節點進入配置模式
    XMC_CAN_NODE_SetInitBit(CAN_NODE1);
    XMC_CAN_NODE_EnableConfigurationChange(CAN_NODE1);
    
    // 3. 配置位時序
    XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t bit_time = {
        .can_frequency = 144000000,
        .baudrate = 250000,
        .sample_point = 8000,
        .sjw = 1
    };
    XMC_CAN_NODE_NominalBitTimeConfigure(CAN_NODE1, &bit_time);
    
    // 4. 配置 GPIO (如果需要)
    XMC_GPIO_Init(P1_12, &can_tx_config);  // CAN TX
    XMC_GPIO_Init(P1_13, &can_rx_config);  // CAN RX
    XMC_CAN_NODE_SetReceiveInput(CAN_NODE1, XMC_CAN_NODE_RECEIVE_INPUT_RXDCC);
    
    // 5. 分配 Message Objects
    for (uint8_t mo = 32; mo <= 35; mo++) {
        XMC_CAN_AllocateMOtoNodeList(&MODULE_CAN, 1, mo);
    }
    
    // 6. 配置 Message Objects
    ConfigureMessageObjects();
    
    // 7. 啟用節點
    XMC_CAN_NODE_DisableConfigurationChange(CAN_NODE1);
    XMC_CAN_NODE_ResetInitBit(CAN_NODE1);
    
    // 8. 啟用中斷
    EnableCANInterrupts();
}
```

---

## 💡 應用範例

### 範例 1: 基本 CAN 通信

```c
// 全域變數
XMC_CAN_MO_t g_tx_mo, g_rx_mo;

void Basic_CAN_Demo(void) 
{
    // 初始化發送 MO
    g_tx_mo.can_mo_ptr = CAN_MO35;
    g_tx_mo.can_identifier = 0x123;
    g_tx_mo.can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS;
    g_tx_mo.can_priority = XMC_CAN_ARBITRATION_MODE_IDE_DIR_BASED_PRIO_2;
    g_tx_mo.can_id_mask = 0x7FF;
    g_tx_mo.can_ide_mask = 1;
    g_tx_mo.can_data_length = 8;
    g_tx_mo.can_mo_type = XMC_CAN_MO_TYPE_TRANSMSGOBJ;
    
    XMC_CAN_MO_Config(&g_tx_mo);
    
    // 初始化接收 MO
    g_rx_mo.can_mo_ptr = CAN_MO32;
    g_rx_mo.can_identifier = 0x456;
    g_rx_mo.can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS;
    g_rx_mo.can_id_mask = 0x7FF;
    g_rx_mo.can_ide_mask = 1;
    g_rx_mo.can_data_length = 8;
    g_rx_mo.can_mo_type = XMC_CAN_MO_TYPE_RECMSGOBJ;
    
    XMC_CAN_MO_Config(&g_rx_mo);
    
    // 發送數據
    g_tx_mo.can_data[0] = 0x12345678;
    g_tx_mo.can_data[1] = 0x9ABCDEF0;
    
    XMC_CAN_MO_Transmit(&g_tx_mo);
    
    // 檢查接收 (輪詢模式)
    while (!(XMC_CAN_MO_GetStatus(&g_rx_mo) & XMC_CAN_MO_STATUS_RX_PENDING)) {
        // 等待接收
    }
    
    // 讀取接收數據
    XMC_CAN_MO_Receive(&g_rx_mo);
    printf("接收: ID=0x%03X, 數據=0x%08X %08X\n", 
           g_rx_mo.can_identifier, g_rx_mo.can_data[1], g_rx_mo.can_data[0]);
}
```

### 範例 2: 中斷驅動的 CAN 通信

```c
// 中斷處理函數
void CAN0_3_IRQHandler(void) 
{
    // 檢查接收事件
    if (XMC_CAN_MO_GetStatus(&g_rx_mo) & XMC_CAN_MO_STATUS_RX_PENDING) {
        // 讀取數據
        XMC_CAN_MO_Receive(&g_rx_mo);
        
        // 處理接收數據
        ProcessReceivedData(&g_rx_mo);
        
        // 清除中斷標誌
        XMC_CAN_MO_ResetStatus(&g_rx_mo, XMC_CAN_MO_RESET_STATUS_RX_PENDING);
    }
}

void Setup_CAN_Interrupts(void) 
{
    // 設定接收事件路由到 Service Request 3
    XMC_CAN_MO_SetEventNodePointer(&g_rx_mo, 
                                   XMC_CAN_MO_POINTER_EVENT_RECEIVE, 
                                   3);
    
    // 啟用接收事件
    XMC_CAN_MO_EnableEvent(&g_rx_mo, XMC_CAN_MO_EVENT_RECEIVE);
    
    // 啟用 NVIC 中斷
    NVIC_SetPriority(CAN0_3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
    NVIC_EnableIRQ(CAN0_3_IRQn);
}
```

### 範例 3: FIFO 緩衝區

```c
void Setup_RX_FIFO(void) 
{
    XMC_CAN_FIFO_CONFIG_t fifo_config = {
        .fifo_bottom = 32,     // MO32 作為 FIFO 底部
        .fifo_base = 32,       // 基礎 MO
        .fifo_top = 35,        // MO35 作為 FIFO 頂部
        .fifo_pointer = 32,    // 初始指針
        .fifo_data_frame_send = false
    };
    
    // 配置 FIFO 基礎 MO
    XMC_CAN_RXFIFO_ConfigMOBaseObject(&MODULE_CAN, &g_rx_mo, &fifo_config);
    
    // 配置從屬 MO
    for (uint8_t mo = 33; mo <= 35; mo++) {
        XMC_CAN_RXFIFO_ConfigMOSlaveObject(&MODULE_CAN, mo, &fifo_config);
    }
}
```

---

## 🔬 進階功能

### 1. 閘道模式 (Gateway Mode)

閘道模式允許自動轉發 CAN 訊息，無需 CPU 介入：

```c
void Setup_CAN_Gateway(void) 
{
    XMC_CAN_GATEWAY_CONFIG_t gateway_config = {
        .gateway_bottom = 40,        // 目標 FIFO 底部
        .gateway_top = 43,           // 目標 FIFO 頂部
        .gateway_base = 40,          // 目標基礎 MO
        .gateway_data_frame_send = true,
        .gateway_identifier_copy = true,
        .gateway_data_length_code_copy = true,
        .gateway_data_copy = true
    };
    
    // 配置來源物件 (接收)
    XMC_CAN_GATEWAY_InitSourceObject(&MODULE_CAN, &g_rx_mo, &gateway_config);
    
    // 配置目標物件 (轉發)
    XMC_CAN_GATEWAY_InitDestinationObject(&MODULE_CAN, 40, &gateway_config);
}
```

### 2. 位時序分析

```c
void Analyze_Bit_Timing(void) 
{
    XMC_CAN_NODE_FRAME_COUNTER_t frame_counter = {
        .can_frame_count_selection = 0,  // 分析模式
        .can_frame_count_mode = XMC_CAN_FRAME_COUNT_MODE_BIT_TIMING
    };
    
    XMC_CAN_NODE_FrameCounterConfigure(CAN_NODE1, &frame_counter);
    
    // 啟用幀計數器事件
    XMC_CAN_NODE_EnableEvent(CAN_NODE1, XMC_CAN_NODE_EVENT_CFCIE);
}
```

### 3. 錯誤處理

```c
void CAN_Error_Handler(void) 
{
    uint32_t node_status = XMC_CAN_NODE_GetStatus(CAN_NODE1);
    
    if (node_status & XMC_CAN_NODE_STATUS_LEC_NO_ERROR) {
        // 無錯誤
    }
    else if (node_status & XMC_CAN_NODE_STATUS_LEC_STUFF_ERROR) {
        printf("位元填充錯誤\n");
        XMC_CAN_NODE_ClearStatus(CAN_NODE1, XMC_CAN_NODE_STATUS_LEC_STUFF_ERROR);
    }
    else if (node_status & XMC_CAN_NODE_STATUS_LEC_FORM_ERROR) {
        printf("格式錯誤\n");
        XMC_CAN_NODE_ClearStatus(CAN_NODE1, XMC_CAN_NODE_STATUS_LEC_FORM_ERROR);
    }
    else if (node_status & XMC_CAN_NODE_STATUS_LEC_ACK_ERROR) {
        printf("ACK 錯誤\n");
        XMC_CAN_NODE_ClearStatus(CAN_NODE1, XMC_CAN_NODE_STATUS_LEC_ACK_ERROR);
    }
    
    // 檢查錯誤計數器
    uint8_t tx_error_count = XMC_CAN_NODE_GetTransmitErrorCounter(CAN_NODE1);
    uint8_t rx_error_count = XMC_CAN_NODE_GetReceiveErrorCounter(CAN_NODE1);
    
    if (tx_error_count > 128 || rx_error_count > 128) {
        printf("警告: 錯誤計數器過高 TX:%d RX:%d\n", tx_error_count, rx_error_count);
    }
}
```

---

## ⚠️ 故障排除

### 常見問題與解決方案

#### 1. CAN 無法發送
**症狀**: `XMC_CAN_MO_Transmit()` 返回錯誤  
**可能原因**:
- MO 未正確配置
- 節點未啟用
- 波特率不匹配

**解決方案**:
```c
// 檢查節點狀態
uint32_t status = XMC_CAN_NODE_GetStatus(CAN_NODE1);
if (status & CAN_NODE_NSR_INIT_Msk) {
    printf("節點仍在初始化模式\n");
    XMC_CAN_NODE_ResetInitBit(CAN_NODE1);
}

// 檢查 MO 狀態
uint32_t mo_status = XMC_CAN_MO_GetStatus(&tx_mo);
printf("MO 狀態: 0x%08X\n", mo_status);
```

#### 2. 中斷未觸發
**症狀**: 中斷處理函數未被調用  
**可能原因**:
- Service Request 路由錯誤
- NVIC 未啟用
- 事件未啟用

**解決方案**:
```c
// 確認 Service Request 配置
uint32_t moipr = tx_mo.can_mo_ptr->MOIPR;
printf("MOIPR: 0x%08X\n", moipr);

// 檢查 NVIC 狀態
if (NVIC_GetEnableIRQ(CAN0_3_IRQn)) {
    printf("中斷已啟用\n");
} else {
    printf("中斷未啟用\n");
    NVIC_EnableIRQ(CAN0_3_IRQn);
}
```

#### 3. 數據損壞
**症狀**: 接收到的數據不正確  
**可能原因**:
- ID 過濾器配置錯誤
- 數據對齊問題
- 時序問題

**解決方案**:
```c
// 檢查 ID 過濾
printf("RX ID: 0x%03X, Mask: 0x%03X\n", 
       rx_mo.can_identifier, rx_mo.can_id_mask);

// 使用位元組存取避免對齊問題
for (int i = 0; i < rx_mo.can_data_length; i++) {
    printf("Data[%d]: 0x%02X\n", i, rx_mo.can_data_byte[i]);
}
```

### 除錯技巧

#### 1. 狀態監控

```c
void Debug_CAN_Status(void) 
{
    printf("=== CAN 節點狀態 ===\n");
    uint32_t status = XMC_CAN_NODE_GetStatus(CAN_NODE1);
    printf("節點狀態: 0x%08X\n", status);
    printf("INIT: %s\n", (status & CAN_NODE_NSR_INIT_Msk) ? "是" : "否");
    printf("TXOK: %s\n", (status & CAN_NODE_NSR_TXOK_Msk) ? "是" : "否");
    printf("RXOK: %s\n", (status & CAN_NODE_NSR_RXOK_Msk) ? "是" : "否");
    
    printf("=== MO 狀態 ===\n");
    uint32_t mo_status = XMC_CAN_MO_GetStatus(&tx_mo);
    printf("TX MO 狀態: 0x%08X\n", mo_status);
    printf("TXPND: %s\n", (mo_status & XMC_CAN_MO_STATUS_TX_PENDING) ? "是" : "否");
    printf("MSGVAL: %s\n", (mo_status & XMC_CAN_MO_STATUS_MESSAGE_VALID) ? "是" : "否");
}
```

#### 2. 暫存器直接存取

```c
void Debug_CAN_Registers(void) 
{
    printf("=== CAN 暫存器 ===\n");
    printf("CLC: 0x%08X\n", MODULE_CAN.CLC);
    printf("ID: 0x%08X\n", MODULE_CAN.ID);
    printf("FDR: 0x%08X\n", MODULE_CAN.FDR);
    
    printf("=== 節點暫存器 ===\n");
    printf("NCR: 0x%08X\n", CAN_NODE1->NCR);
    printf("NSR: 0x%08X\n", CAN_NODE1->NSR);
    printf("NIPR: 0x%08X\n", CAN_NODE1->NIPR);
}
```

---

## 📈 效能最佳化

### 1. 中斷優化

```c
// 使用較高優先級處理關鍵 CAN 事件
NVIC_SetPriority(CAN0_0_IRQn, 0);  // 最高優先級
NVIC_SetPriority(CAN0_3_IRQn, 1);  // 次高優先級

// 在中斷中使用快速處理
void CAN0_0_IRQHandler(void) 
{
    __disable_irq();  // 關閉中斷以獲得原子性
    
    // 快速數據複製
    if (XMC_CAN_MO_GetStatus(&rx_mo) & XMC_CAN_MO_STATUS_RX_PENDING) {
        memcpy(&rx_buffer, &rx_mo.can_data, rx_mo.can_data_length);
        rx_data_ready = true;
        XMC_CAN_MO_ResetStatus(&rx_mo, XMC_CAN_MO_RESET_STATUS_RX_PENDING);
    }
    
    __enable_irq();
}
```

### 2. 記憶體最佳化

```c
// 使用聯合體減少記憶體使用
typedef union {
    struct {
        uint16_t id;
        uint8_t dlc;
        uint8_t data[8];
    } frame;
    uint32_t raw[3];  // 快速複製用
} CAN_Message_t;
```

---

## 📚 參考資料

### 相關文檔
- XMC4800 參考手冊
- CAN 2.0B 規範
- XMCLib 用戶指南
- DAVE APP 說明文檔

### 有用的宏定義

```c
// 常用 CAN ID 範圍
#define CAN_STD_ID_MAX          0x7FF
#define CAN_EXT_ID_MAX          0x1FFFFFFF

// CANopen 標準 ID
#define CANOPEN_NMT             0x000
#define CANOPEN_SYNC            0x080
#define CANOPEN_EMERGENCY       0x080
#define CANOPEN_TPDO1           0x180
#define CANOPEN_RPDO1           0x200
#define CANOPEN_TPDO2           0x280
#define CANOPEN_RPDO2           0x300
#define CANOPEN_TPDO3           0x380
#define CANOPEN_RPDO3           0x400
#define CANOPEN_TPDO4           0x480
#define CANOPEN_RPDO4           0x500
#define CANOPEN_SDO_TX          0x580
#define CANOPEN_SDO_RX          0x600
#define CANOPEN_HEARTBEAT       0x700

// 錯誤檢查宏
#define CHECK_CAN_STATUS(status) \
    do { \
        if ((status) != XMC_CAN_STATUS_SUCCESS) { \
            printf("CAN 錯誤: %d, 行: %d\n", (status), __LINE__); \
            return (status); \
        } \
    } while(0)
```

---

**結語**

本手冊涵蓋了 XMC CAN 庫的核心功能和應用方法。通過理解這些 API 和範例，您可以有效地在 XMC4800 平台上開發 CAN 通信應用。記住始終參考最新的技術文檔和勘誤表，以確保最佳的兼容性和性能。

**版權聲明**: 本文檔基於 Infineon XMCLib v2.2.0 編寫，遵循 Boost Software License - Version 1.0。