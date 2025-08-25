# XMC4800 CAN Service Request 與中斷機制詳解

## 📋 目錄
1. [基本概念](#基本概念)
2. [Service Request 架構](#service-request-架構)
3. [中斷路由機制](#中斷路由機制)
4. [DAVE 配置解析](#dave-配置解析)
5. [實際應用範例](#實際應用範例)
6. [故障排除](#故障排除)

---

## 🎯 基本概念

### Service Request (服務請求) 是什麼？

**Service Request** 是 XMC4800 硬體中的中斷路由系統，它將硬體事件連接到 ARM Cortex-M4 的 NVIC 中斷線。

```
硬體事件 → Service Request → NVIC 中斷線 → 中斷處理函數
```

### 為什麼需要 Service Request？

1. **多對一映射**: 多個硬體事件可以路由到同一個中斷線
2. **靈活路由**: 可以動態改變事件的中斷目標
3. **優先級管理**: 不同的 Service Request 可以有不同的優先級
4. **資源共享**: 減少所需的 NVIC 中斷線數量

---

## 🏗️ Service Request 架構

### XMC4800 CAN 模組的 Service Request 結構

```
CAN 硬體模組
├── CAN Node 事件
│   ├── Alert 事件 → alert_event_sr
│   ├── LEC 錯誤事件 → lec_event_sr
│   ├── TXOK 事件 → txok_event_sr
│   └── Frame Count 事件 → framecount_event_sr
├── Message Object 事件
│   ├── MO32 RX事件 → rx_sr = 0
│   ├── MO33 TX事件 → tx_sr = 0
│   ├── MO34 TX事件 → tx_sr = 0
│   └── MO35 TX事件 → tx_sr = 0
└── Service Request 路由
    ├── SR0 → CAN0_0_IRQn (IRQ 42)
    ├── SR1 → CAN0_1_IRQn (IRQ 43)
    ├── SR2 → CAN0_2_IRQn (IRQ 44)
    └── SR3 → CAN0_3_IRQn (IRQ 45)
```

### Service Request 編號對應

| Service Request | NVIC 中斷線 | IRQ 編號 | 中斷處理函數 |
|----------------|-------------|----------|-------------|
| SR0 | CAN0_0_IRQn | 42 | `CAN0_0_IRQHandler()` |
| SR1 | CAN0_1_IRQn | 43 | `CAN0_1_IRQHandler()` |
| SR2 | CAN0_2_IRQn | 44 | `CAN0_2_IRQHandler()` |
| SR3 | CAN0_3_IRQn | 45 | `CAN0_3_IRQHandler()` |

---

## 🔗 中斷路由機制

### 1. Node 層級 Service Request

在 DAVE 配置中的 Node Service Request：

```c
// 來自 can_node_conf.c
static const CAN_NODE_SR_t CAN_NODE_0_sr = {
  .alert_event_sr      = 0U,        // Alert 事件 → SR0 → CAN0_0_IRQn
  .lec_event_sr        = 0U,        // LEC 錯誤 → SR0 → CAN0_0_IRQn
  .txok_event_sr       = 0U,        // TX OK → SR0 → CAN0_0_IRQn
  .framecount_event_sr = 0U,        // Frame Count → SR0 → CAN0_0_IRQn
};
```

### 2. Message Object 層級 Service Request

每個 Message Object 都有獨立的 TX 和 RX Service Request：

```c
// 來自 can_node_conf.c
const CAN_NODE_LMO_t CAN_NODE_0_LMO_04_Config = {
  .mo_ptr     = (XMC_CAN_MO_t*)&CAN_NODE_0_LMO_04,
  .number     = 32U,                // MO32 (接收用)
  .tx_sr      = 0U,                 // TX 事件 → SR0 → CAN0_0_IRQn
  .rx_sr      = 0U,                 // RX 事件 → SR0 → CAN0_0_IRQn
  .tx_event_enable = false,         // TX 事件未啟用
  .rx_event_enable = true           // RX 事件已啟用
};
```

### 3. 實際硬體暫存器配置

Service Request 最終會配置硬體暫存器：

```c
// MO32 的中斷指針暫存器 (MOIPR)
CAN_MO32->MOIPR = (CAN_MO32->MOIPR & ~CAN_MO_MOIPR_RXINP_Msk) | 
                  ((uint32_t)0 << CAN_MO_MOIPR_RXINP_Pos);
//                          ↑
//                    rx_sr = 0 → SR0
```

---

## ⚙️ DAVE 配置解析

### 當前專案的 Service Request 配置

根據 `can_node_conf.c` 的分析：

```c
// 所有事件都路由到 SR0 (CAN0_0_IRQn)
Node 事件:      SR0 → CAN0_0_IRQHandler()
MO32 RX 事件:   SR0 → CAN0_0_IRQHandler()
MO33 TX 事件:   SR0 → CAN0_0_IRQHandler()
MO34 TX 事件:   SR0 → CAN0_0_IRQHandler()
MO35 TX 事件:   SR0 → CAN0_0_IRQHandler()
```

### 為什麼之前使用 CAN0_3_IRQHandler？

這是配置錯誤！根據 DAVE 配置，所有事件都路由到 SR0，因此應該使用：

```c
// ✅ 正確
void CAN0_0_IRQHandler(void) {
    // 處理所有 CAN 事件
}

// ❌ 錯誤 - 這個中斷永遠不會被觸發
void CAN0_3_IRQHandler(void) {
    // 永遠不會執行
}
```

---

## 💡 實際應用範例

### 範例 1: 單一中斷處理所有事件

```c
// 當前配置：所有事件 → SR0 → CAN0_0_IRQHandler
void CAN0_0_IRQHandler(void) 
{
    uint32_t node_status = XMC_CAN_NODE_GetStatus(CAN_NODE1);
    
    // 檢查 Node 層級事件
    if (node_status & XMC_CAN_NODE_STATUS_TX_OK) {
        printf("Node TX OK事件\n");
        XMC_CAN_NODE_ClearStatus(CAN_NODE1, XMC_CAN_NODE_STATUS_TX_OK);
    }
    
    // 檢查各個 MO 的事件
    for (int i = 0; i < 4; i++) {
        CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[i];
        uint32_t mo_status = XMC_CAN_MO_GetStatus(lmo);
        
        if (mo_status & XMC_CAN_MO_STATUS_RX_PENDING) {
            printf("MO%d 接收事件\n", lmo->number);
            CAN_NODE_MO_Receive(lmo);
            XMC_CAN_MO_ResetStatus(lmo->mo_ptr, XMC_CAN_MO_RESET_STATUS_RX_PENDING);
        }
        
        if (mo_status & XMC_CAN_MO_STATUS_TX_PENDING) {
            printf("MO%d 發送事件\n", lmo->number);
            XMC_CAN_MO_ResetStatus(lmo->mo_ptr, XMC_CAN_MO_RESET_STATUS_TX_PENDING);
        }
    }
}
```

### 範例 2: 分散式中斷處理 (需要重新配置 DAVE)

如果要使用分散式中斷，需要在 DAVE 中修改 Service Request 配置：

```c
// 理想的配置 (需要在 DAVE GUI 中設定)
static const CAN_NODE_SR_t CAN_NODE_0_sr = {
  .alert_event_sr      = 0U,        // Alert → SR0
  .lec_event_sr        = 1U,        // LEC → SR1
  .txok_event_sr       = 2U,        // TXOK → SR2
  .framecount_event_sr = 3U,        // Frame Count → SR3
};

const CAN_NODE_LMO_t CAN_NODE_0_LMO_04_Config = {
  .tx_sr      = 1U,                 // TX → SR1
  .rx_sr      = 0U,                 // RX → SR0
};
```

對應的中斷處理函數：

```c
// 專門處理接收事件
void CAN0_0_IRQHandler(void) {
    // 只處理 RX 和 Alert 事件
}

// 專門處理發送事件
void CAN0_1_IRQHandler(void) {
    // 只處理 TX 和 LEC 事件
}

// 專門處理 TXOK 事件
void CAN0_2_IRQHandler(void) {
    // 只處理 TXOK 事件
}

// 專門處理 Frame Count 事件
void CAN0_3_IRQHandler(void) {
    // 只處理 Frame Count 事件
}
```

---

## 🔧 Service Request 設定 API

### 1. 運行時修改 Service Request

```c
// 設定 MO 接收事件的 Service Request
void SetMO_RX_ServiceRequest(CAN_NODE_LMO_t *lmo, uint8_t sr_number) 
{
    // 設定接收事件路由到指定的 Service Request
    XMC_CAN_MO_SetEventNodePointer(lmo->mo_ptr, 
                                   XMC_CAN_MO_POINTER_EVENT_RECEIVE, 
                                   sr_number);
    
    // 啟用接收事件
    XMC_CAN_MO_EnableEvent(lmo->mo_ptr, XMC_CAN_MO_EVENT_RECEIVE);
}

// 使用範例
void Setup_Distributed_Interrupts(void) 
{
    // MO32 接收事件 → SR0
    SetMO_RX_ServiceRequest(CAN_NODE_0.lmobj_ptr[3], 0);
    
    // MO35 發送事件 → SR1
    XMC_CAN_MO_SetEventNodePointer(CAN_NODE_0.lmobj_ptr[0]->mo_ptr,
                                   XMC_CAN_MO_POINTER_EVENT_TRANSMIT,
                                   1);
    XMC_CAN_MO_EnableEvent(CAN_NODE_0.lmobj_ptr[0]->mo_ptr, 
                          XMC_CAN_MO_EVENT_TRANSMIT);
}
```

### 2. 檢查當前 Service Request 配置

```c
void Debug_ServiceRequest_Config(void) 
{
    printf("=== Service Request 配置檢查 ===\n");
    
    // 檢查 Node Service Request
    printf("Node Alert SR: %d\n", CAN_NODE_0.node_sr_ptr->alert_event_sr);
    printf("Node LEC SR: %d\n", CAN_NODE_0.node_sr_ptr->lec_event_sr);
    printf("Node TXOK SR: %d\n", CAN_NODE_0.node_sr_ptr->txok_event_sr);
    printf("Node Frame Count SR: %d\n", CAN_NODE_0.node_sr_ptr->framecount_event_sr);
    
    // 檢查 MO Service Request
    for (int i = 0; i < 4; i++) {
        CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[i];
        printf("MO%d TX SR: %d, RX SR: %d\n", 
               lmo->number, lmo->tx_sr, lmo->rx_sr);
               
        // 檢查硬體暫存器
        uint32_t moipr = lmo->mo_ptr->can_mo_ptr->MOIPR;
        uint8_t rx_sr = (moipr & CAN_MO_MOIPR_RXINP_Msk) >> CAN_MO_MOIPR_RXINP_Pos;
        uint8_t tx_sr = (moipr & CAN_MO_MOIPR_TXINP_Msk) >> CAN_MO_MOIPR_TXINP_Pos;
        printf("MO%d 實際硬體: TX SR=%d, RX SR=%d\n", lmo->number, tx_sr, rx_sr);
    }
}
```

---

## ⚠️ 故障排除

### 問題 1: 中斷處理函數未被調用

**可能原因**:
1. Service Request 配置錯誤
2. NVIC 未啟用
3. 事件未啟用

**解決方案**:
```c
// 1. 檢查 Service Request 配置
Debug_ServiceRequest_Config();

// 2. 確保 NVIC 啟用
NVIC_EnableIRQ(CAN0_0_IRQn);
NVIC_SetPriority(CAN0_0_IRQn, 1);

// 3. 確保事件啟用
CAN_NODE_MO_EnableRxEvent(CAN_NODE_0.lmobj_ptr[3]);
```

### 問題 2: 錯誤的中斷處理函數

**症狀**: 使用 `CAN0_3_IRQHandler` 但事件路由到 SR0

**解決方案**:
```c
// 根據實際 Service Request 配置選擇正確的處理函數
// 當前配置: 所有事件 → SR0
void CAN0_0_IRQHandler(void) {  // ✅ 正確
    // 處理邏輯
}

// 而不是
void CAN0_3_IRQHandler(void) {  // ❌ 錯誤
    // 永遠不會執行
}
```

### 問題 3: 中斷風暴

**症狀**: 中斷頻繁觸發導致系統卡住

**解決方案**:
```c
void CAN0_0_IRQHandler(void) 
{
    // 必須清除所有觸發的事件標誌
    uint32_t node_status = XMC_CAN_NODE_GetStatus(CAN_NODE1);
    if (node_status & XMC_CAN_NODE_STATUS_TX_OK) {
        XMC_CAN_NODE_ClearStatus(CAN_NODE1, XMC_CAN_NODE_STATUS_TX_OK);
    }
    
    // 清除 MO 事件標誌
    for (int i = 0; i < 4; i++) {
        CAN_NODE_LMO_t *lmo = CAN_NODE_0.lmobj_ptr[i];
        uint32_t mo_status = XMC_CAN_MO_GetStatus(lmo);
        
        if (mo_status & XMC_CAN_MO_STATUS_RX_PENDING) {
            // 必須清除標誌！
            XMC_CAN_MO_ResetStatus(lmo->mo_ptr, XMC_CAN_MO_RESET_STATUS_RX_PENDING);
        }
    }
}
```

---

## 📊 Service Request 最佳實踐

### 1. 合理分配 Service Request

```c
// 建議的 Service Request 分配策略
SR0: 高優先級接收事件 (即時響應)
SR1: 一般發送事件
SR2: 錯誤和狀態事件
SR3: 低優先級維護事件
```

### 2. 中斷優先級設定

```c
void Setup_CAN_Interrupt_Priorities(void) 
{
    // 設定不同 Service Request 的優先級
    NVIC_SetPriority(CAN0_0_IRQn, 0);  // 最高優先級 (接收)
    NVIC_SetPriority(CAN0_1_IRQn, 1);  // 中等優先級 (發送)
    NVIC_SetPriority(CAN0_2_IRQn, 2);  // 低優先級 (錯誤)
    NVIC_SetPriority(CAN0_3_IRQn, 3);  // 最低優先級 (維護)
    
    // 啟用中斷
    NVIC_EnableIRQ(CAN0_0_IRQn);
    NVIC_EnableIRQ(CAN0_1_IRQn);
    NVIC_EnableIRQ(CAN0_2_IRQn);
    NVIC_EnableIRQ(CAN0_3_IRQn);
}
```

### 3. 事件處理最佳化

```c
void CAN0_0_IRQHandler(void) 
{
    // 使用原子操作避免競爭條件
    __disable_irq();
    
    // 快速處理關鍵事件
    if (/* 高優先級事件 */) {
        // 立即處理
    }
    
    // 標記其他事件稍後處理
    if (/* 一般事件 */) {
        event_pending_flag = true;
    }
    
    __enable_irq();
}

// 在主循環中處理非關鍵事件
void main_loop(void) 
{
    if (event_pending_flag) {
        event_pending_flag = false;
        // 處理非關鍵事件
    }
}
```

---

## 📚 總結

Service Request 是 XMC4800 CAN 系統的核心中斷路由機制：

1. **硬體事件** 通過 **Service Request** 路由到 **NVIC 中斷線**
2. **DAVE 配置** 決定了 Service Request 的分配
3. **正確的中斷處理函數** 必須與 Service Request 配置一致
4. **事件標誌清除** 是避免中斷風暴的關鍵
5. **合理的優先級分配** 確保系統響應性能

理解 Service Request 機制是掌握 XMC4800 CAN 中斷系統的關鍵！