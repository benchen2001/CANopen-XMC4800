# XMC CAN 事件與中斷系統詳細說明

## 📋 目錄
1. [CAN 中斷架構概述](#can-中斷架構概述)
2. [節點事件類型](#節點事件類型)
3. [訊息物件事件](#訊息物件事件)
4. [中斷服務程序設計](#中斷服務程序設計)
5. [事件狀態檢查](#事件狀態檢查)
6. [實際應用範例](#實際應用範例)
7. [最佳實踐指南](#最佳實踐指南)

---

## CAN 中斷架構概述

### XMC CAN 中斷層級結構
```
CAN 模組
├── 節點層級中斷 (Node Level)
│   ├── 節點 0 中斷
│   ├── 節點 1 中斷
│   └── 節點 2 中斷
└── 訊息物件中斷 (Message Object Level)
    ├── MO 0-31 中斷
    ├── MO 32-63 中斷
    └── MO 64-255 中斷
```

### 中斷向量表
```c
// XMC4800 CAN 中斷向量
typedef enum {
    CAN0_0_IRQn = 42,      // CAN Node 0 Line 0
    CAN0_1_IRQn = 43,      // CAN Node 0 Line 1
    CAN0_2_IRQn = 44,      // CAN Node 0 Line 2
    CAN0_3_IRQn = 45,      // CAN Node 0 Line 3
    CAN0_4_IRQn = 46,      // CAN Node 0 Line 4
    CAN0_5_IRQn = 47,      // CAN Node 0 Line 5
    CAN0_6_IRQn = 48,      // CAN Node 0 Line 6
    CAN0_7_IRQn = 49       // CAN Node 0 Line 7
} IRQn_Type;
```

---

## 節點事件類型

### 節點狀態暫存器 (NSR)
```c
// 節點狀態位元定義
#define CAN_NODE_NSR_LEC_Pos     0      // 最後錯誤代碼位置
#define CAN_NODE_NSR_LEC_Msk     (0x7U << CAN_NODE_NSR_LEC_Pos)
#define CAN_NODE_NSR_TXOK_Pos    3      // 傳送成功
#define CAN_NODE_NSR_RXOK_Pos    4      // 接收成功
#define CAN_NODE_NSR_ALERT_Pos   5      // 警報狀態
#define CAN_NODE_NSR_EWRN_Pos    6      // 錯誤警告
#define CAN_NODE_NSR_BOFF_Pos    7      // 匯流排關閉
#define CAN_NODE_NSR_LLE_Pos     8      // 清單長度錯誤
#define CAN_NODE_NSR_LOE_Pos     9      // 清單物件錯誤
```

### 可配置的節點事件
```c
typedef enum {
    XMC_CAN_NODE_EVENT_CFCIE = (1U << 6),    // 時鐘失效檢測
    XMC_CAN_NODE_EVENT_LEIE = (1U << 7),     // 最後錯誤代碼中斷
    XMC_CAN_NODE_EVENT_ALIE = (1U << 5),     // 警報限制中斷
    XMC_CAN_NODE_EVENT_CANDIS = (1U << 1)    // CAN 節點停用
} XMC_CAN_NODE_EVENT_t;
```

### 節點事件啟用/停用
```c
// 啟用節點事件
void XMC_CAN_NODE_EnableEvent(XMC_CAN_NODE_t *const can_node, 
                              const XMC_CAN_NODE_EVENT_t event);

// 停用節點事件
void XMC_CAN_NODE_DisableEvent(XMC_CAN_NODE_t *const can_node, 
                               const XMC_CAN_NODE_EVENT_t event);

// 使用範例
XMC_CAN_NODE_EnableEvent(XMC_CAN_NODE0, XMC_CAN_NODE_EVENT_LEIE);
XMC_CAN_NODE_EnableEvent(XMC_CAN_NODE0, XMC_CAN_NODE_EVENT_ALIE);
```

---

## 訊息物件事件

### MO 控制暫存器 (MOCTR) 事件位元
```c
// MO 事件啟用位元
#define CAN_MO_MOCTR_RXIE_Pos    0      // 接收中斷啟用
#define CAN_MO_MOCTR_TXIE_Pos    1      // 傳送中斷啟用
#define CAN_MO_MOCTR_OVIE_Pos    2      // 溢位中斷啟用
#define CAN_MO_MOCTR_FRREN_Pos   20     // 外部請求啟用
#define CAN_MO_MOCTR_RMM_Pos     21     // 接收多重訊息
#define CAN_MO_MOCTR_SDT_Pos     22     // 單一資料傳輸
#define CAN_MO_MOCTR_STT_Pos     23     // 單一傳輸試驗
```

### MO 狀態暫存器 (MOSTAT) 狀態位元
```c
// MO 狀態檢查位元
#define XMC_CAN_MO_STATUS_RX_PENDING    (1U << 0)   // 接收待處理
#define XMC_CAN_MO_STATUS_TX_PENDING    (1U << 1)   // 傳送待處理
#define XMC_CAN_MO_STATUS_RX_UPDATING   (1U << 2)   // 接收更新中
#define XMC_CAN_MO_STATUS_NEW_DATA      (1U << 3)   // 新資料可用
#define XMC_CAN_MO_STATUS_MESSAGE_LOST  (1U << 4)   // 訊息遺失
#define XMC_CAN_MO_STATUS_MESSAGE_VALID (1U << 5)   // 訊息有效
#define XMC_CAN_MO_STATUS_RX_ENABLE     (1U << 8)   // 接收啟用
#define XMC_CAN_MO_STATUS_TX_REQUEST    (1U << 9)   // 傳送請求
#define XMC_CAN_MO_STATUS_TX_ENABLE0    (1U << 10)  // 傳送啟用 0
#define XMC_CAN_MO_STATUS_TX_ENABLE1    (1U << 11)  // 傳送啟用 1
#define XMC_CAN_MO_STATUS_DIR           (1U << 12)  // 訊息方向
```

### 狀態檢查和重置函數
```c
// 取得 MO 狀態
uint32_t XMC_CAN_MO_GetStatus(const XMC_CAN_MO_t *const can_mo);

// 重置特定狀態位元
void XMC_CAN_MO_ResetStatus(XMC_CAN_MO_t *const can_mo, uint32_t status_mask);

// 設定特定狀態位元
void XMC_CAN_MO_SetStatus(XMC_CAN_MO_t *const can_mo, uint32_t status_mask);
```

---

## 中斷服務程序設計

### 基本中斷處理架構
```c
// CAN 節點中斷處理函數
void CAN0_0_IRQHandler(void) {
    uint32_t node_status;
    uint32_t mo_pending;
    
    // 讀取節點狀態
    node_status = XMC_CAN_NODE0->NSR;
    
    // 處理節點層級事件
    if (node_status & CAN_NODE_NSR_ALERT_Msk) {
        CAN_HandleAlertEvent();
    }
    
    if (node_status & CAN_NODE_NSR_EWRN_Msk) {
        CAN_HandleErrorWarning();
    }
    
    if (node_status & CAN_NODE_NSR_BOFF_Msk) {
        CAN_HandleBusOff();
    }
    
    // 檢查 MO 待處理中斷
    mo_pending = CAN->MSIMASK;  // 取得 MO 中斷遮罩
    
    // 處理個別 MO 中斷
    for (int mo_num = 0; mo_num < 64; mo_num++) {
        if (mo_pending & (1U << mo_num)) {
            CAN_HandleMOInterrupt(mo_num);
        }
    }
}
```

### 詳細的 MO 中斷處理
```c
void CAN_HandleMOInterrupt(uint8_t mo_number) {
    XMC_CAN_MO_t *mo = &message_objects[mo_number];
    uint32_t mo_status = XMC_CAN_MO_GetStatus(mo);
    
    // 處理接收中斷
    if (mo_status & XMC_CAN_MO_STATUS_RX_PENDING) {
        CAN_HandleRxInterrupt(mo);
        // 清除接收待處理狀態
        XMC_CAN_MO_ResetStatus(mo, XMC_CAN_MO_STATUS_RX_PENDING);
    }
    
    // 處理傳送中斷
    if (mo_status & XMC_CAN_MO_STATUS_TX_PENDING) {
        CAN_HandleTxInterrupt(mo);
        // 清除傳送待處理狀態
        XMC_CAN_MO_ResetStatus(mo, XMC_CAN_MO_STATUS_TX_PENDING);
    }
    
    // 處理新資料事件
    if (mo_status & XMC_CAN_MO_STATUS_NEW_DATA) {
        CAN_HandleNewDataEvent(mo);
        // 清除新資料狀態
        XMC_CAN_MO_ResetStatus(mo, XMC_CAN_MO_STATUS_NEW_DATA);
    }
    
    // 處理訊息遺失
    if (mo_status & XMC_CAN_MO_STATUS_MESSAGE_LOST) {
        CAN_HandleMessageLost(mo);
        // 清除遺失狀態
        XMC_CAN_MO_ResetStatus(mo, XMC_CAN_MO_STATUS_MESSAGE_LOST);
    }
}
```

### 接收中斷處理
```c
void CAN_HandleRxInterrupt(XMC_CAN_MO_t *mo) {
    CAN_Message_t received_msg;
    
    // 讀取接收到的資料
    if (XMC_CAN_MO_ReceiveData(mo) == XMC_CAN_STATUS_SUCCESS) {
        // 複製資料到應用層緩衝區
        received_msg.id = XMC_CAN_MO_GetIdentifier(mo);
        received_msg.dlc = mo->can_data_length;
        memcpy(received_msg.data, mo->can_data, mo->can_data_length);
        
        // 將訊息放入接收佇列
        if (rx_queue_put(&received_msg) != QUEUE_SUCCESS) {
            // 佇列滿，記錄錯誤
            rx_overflow_count++;
        }
        
        // 通知應用層有新訊息
        osEventFlagsSet(can_event_flags, CAN_RX_EVENT_FLAG);
    }
}
```

### 傳送完成中斷處理
```c
void CAN_HandleTxInterrupt(XMC_CAN_MO_t *mo) {
    // 標記傳送完成
    tx_complete_flags |= (1U << mo->mo_number);
    
    // 檢查是否有待傳送的訊息
    CAN_Message_t *next_msg = tx_queue_get();
    if (next_msg != NULL) {
        // 準備下一個傳送
        CAN_PrepareTxMessage(mo, next_msg);
        XMC_CAN_MO_Transmit(mo);
    }
    
    // 通知應用層傳送完成
    osEventFlagsSet(can_event_flags, CAN_TX_COMPLETE_FLAG);
}
```

---

## 事件狀態檢查

### 輪詢模式狀態檢查
```c
// 定期檢查 CAN 狀態 (用於非中斷模式)
void CAN_StatusPoll(void) {
    static uint32_t poll_counter = 0;
    
    poll_counter++;
    
    // 檢查節點狀態
    uint32_t node_status = XMC_CAN_NODE0->NSR;
    
    // 檢查錯誤狀態
    if (node_status & CAN_NODE_NSR_EWRN_Msk) {
        uint8_t error_count = (node_status & CAN_NODE_NSR_LEC_Msk) >> CAN_NODE_NSR_LEC_Pos;
        printf("CAN 錯誤警告: 錯誤代碼 = %d\n", error_count);
    }
    
    // 檢查匯流排狀態
    if (node_status & CAN_NODE_NSR_BOFF_Msk) {
        printf("CAN 匯流排關閉\n");
        CAN_RecoverFromBusOff();
    }
    
    // 每1000次輪詢輸出統計
    if ((poll_counter % 1000) == 0) {
        CAN_PrintStatistics();
    }
}
```

### 錯誤統計和診斷
```c
typedef struct {
    uint32_t tx_count;          // 傳送計數
    uint32_t rx_count;          // 接收計數
    uint32_t error_count;       // 錯誤計數
    uint32_t busoff_count;      // 匯流排關閉計數
    uint32_t overflow_count;    // 溢位計數
    uint32_t last_error_code;   // 最後錯誤代碼
} CAN_Statistics_t;

CAN_Statistics_t can_stats = {0};

void CAN_UpdateStatistics(void) {
    uint32_t node_status = XMC_CAN_NODE0->NSR;
    
    // 更新錯誤統計
    if (node_status & CAN_NODE_NSR_ALERT_Msk) {
        can_stats.error_count++;
        can_stats.last_error_code = (node_status & CAN_NODE_NSR_LEC_Msk) >> CAN_NODE_NSR_LEC_Pos;
    }
    
    // 更新匯流排關閉統計
    if (node_status & CAN_NODE_NSR_BOFF_Msk) {
        can_stats.busoff_count++;
    }
    
    // 更新傳送/接收統計
    if (node_status & CAN_NODE_NSR_TXOK_Msk) {
        can_stats.tx_count++;
    }
    
    if (node_status & CAN_NODE_NSR_RXOK_Msk) {
        can_stats.rx_count++;
    }
}
```

---

## 實際應用範例

### 完整的中斷驅動 CAN 系統
```c
#include "xmc_can.h"
#include "cmsis_os2.h"

// 全域變數
static XMC_CAN_MO_t rx_mo[4];    // 4個接收 MO
static XMC_CAN_MO_t tx_mo[4];    // 4個傳送 MO
static osEventFlagsId_t can_events;
static osMessageQueueId_t rx_queue;
static osMessageQueueId_t tx_queue;

// 事件標誌定義
#define CAN_RX_EVENT    (1U << 0)
#define CAN_TX_EVENT    (1U << 1)
#define CAN_ERROR_EVENT (1U << 2)

// CAN 初始化
void CAN_SystemInit(void) {
    // 1. 硬體初始化
    CAN_HardwareInit();
    
    // 2. 建立 RTOS 物件
    can_events = osEventFlagsNew(NULL);
    rx_queue = osMessageQueueNew(32, sizeof(CAN_Message_t), NULL);
    tx_queue = osMessageQueueNew(16, sizeof(CAN_Message_t), NULL);
    
    // 3. 配置中斷
    NVIC_SetPriority(CAN0_0_IRQn, 2);
    NVIC_EnableIRQ(CAN0_0_IRQn);
    
    // 4. 啟用事件
    XMC_CAN_NODE_EnableEvent(XMC_CAN_NODE0, XMC_CAN_NODE_EVENT_LEIE);
    XMC_CAN_NODE_EnableEvent(XMC_CAN_NODE0, XMC_CAN_NODE_EVENT_ALIE);
}

// 硬體層初始化
void CAN_HardwareInit(void) {
    // CAN 模組初始化
    XMC_CAN_Enable(CAN);
    XMC_CAN_Init(CAN, 120000000);
    
    // 節點配置
    XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t bit_time = {
        .can_frequency = 120000000,
        .baudrate = 500000,
        .sample_point = 8000,
        .sjw = 1
    };
    XMC_CAN_NODE_NominalBitTimeConfigure(XMC_CAN_NODE0, &bit_time);
    
    // 配置接收 MO
    for (int i = 0; i < 4; i++) {
        rx_mo[i].can_mo_type = XMC_CAN_MO_TYPE_RECMSGOBJ;
        rx_mo[i].can_identifier = 0x100 + i;  // ID: 0x100-0x103
        rx_mo[i].can_id_mask = 0x7FF;
        rx_mo[i].can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS;
        
        // 啟用接收中斷
        rx_mo[i].can_mo_ptr->MOCTR |= CAN_MO_MOCTR_RXIE_Msk;
        
        XMC_CAN_MO_Config(&rx_mo[i]);
        XMC_CAN_AllocateMOtoNodeList(CAN, 0, i);
    }
    
    // 配置傳送 MO
    for (int i = 0; i < 4; i++) {
        tx_mo[i].can_mo_type = XMC_CAN_MO_TYPE_TRANSMSGOBJ;
        tx_mo[i].can_identifier = 0x200 + i;  // ID: 0x200-0x203
        tx_mo[i].can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS;
        
        // 啟用傳送中斷
        tx_mo[i].can_mo_ptr->MOCTR |= CAN_MO_MOCTR_TXIE_Msk;
        
        XMC_CAN_MO_Config(&tx_mo[i]);
        XMC_CAN_AllocateMOtoNodeList(CAN, 0, 4 + i);
    }
}

// 中斷服務程序
void CAN0_0_IRQHandler(void) {
    uint32_t node_status = XMC_CAN_NODE0->NSR;
    
    // 處理錯誤事件
    if (node_status & (CAN_NODE_NSR_EWRN_Msk | CAN_NODE_NSR_BOFF_Msk)) {
        osEventFlagsSet(can_events, CAN_ERROR_EVENT);
    }
    
    // 檢查 RX MO 中斷
    for (int i = 0; i < 4; i++) {
        uint32_t mo_status = XMC_CAN_MO_GetStatus(&rx_mo[i]);
        if (mo_status & XMC_CAN_MO_STATUS_NEW_DATA) {
            CAN_ProcessRxMO(&rx_mo[i]);
            XMC_CAN_MO_ResetStatus(&rx_mo[i], XMC_CAN_MO_STATUS_NEW_DATA);
        }
    }
    
    // 檢查 TX MO 中斷
    for (int i = 0; i < 4; i++) {
        uint32_t mo_status = XMC_CAN_MO_GetStatus(&tx_mo[i]);
        if (mo_status & XMC_CAN_MO_STATUS_TX_PENDING) {
            CAN_ProcessTxComplete(&tx_mo[i]);
            XMC_CAN_MO_ResetStatus(&tx_mo[i], XMC_CAN_MO_STATUS_TX_PENDING);
        }
    }
}

// 處理接收資料
void CAN_ProcessRxMO(XMC_CAN_MO_t *mo) {
    CAN_Message_t msg;
    
    if (XMC_CAN_MO_ReceiveData(mo) == XMC_CAN_STATUS_SUCCESS) {
        msg.id = XMC_CAN_MO_GetIdentifier(mo);
        msg.dlc = mo->can_data_length;
        memcpy(msg.data, mo->can_data, 8);
        msg.timestamp = osKernelGetTickCount();
        
        // 放入接收佇列
        if (osMessageQueuePut(rx_queue, &msg, 0, 0) == osOK) {
            osEventFlagsSet(can_events, CAN_RX_EVENT);
        }
    }
}

// 應用層傳送函數
osStatus_t CAN_SendMessage(uint32_t id, uint8_t *data, uint8_t length) {
    CAN_Message_t msg;
    
    msg.id = id;
    msg.dlc = length;
    memcpy(msg.data, data, length);
    
    return osMessageQueuePut(tx_queue, &msg, 0, 1000);  // 1秒超時
}

// 應用層接收函數
osStatus_t CAN_ReceiveMessage(CAN_Message_t *msg, uint32_t timeout) {
    return osMessageQueueGet(rx_queue, msg, NULL, timeout);
}

// CAN 處理任務
void CAN_ProcessTask(void *argument) {
    CAN_Message_t tx_msg;
    uint32_t events;
    
    while (1) {
        // 等待事件
        events = osEventFlagsWait(can_events, 
                                 CAN_RX_EVENT | CAN_TX_EVENT | CAN_ERROR_EVENT,
                                 osFlagsWaitAny, 100);
        
        // 處理接收事件
        if (events & CAN_RX_EVENT) {
            // 在主要應用中處理接收到的訊息
            // 這裡可以呼叫特定的處理函數
        }
        
        // 處理傳送請求
        while (osMessageQueueGet(tx_queue, &tx_msg, NULL, 0) == osOK) {
            CAN_TransmitMessage(&tx_msg);
        }
        
        // 處理錯誤事件
        if (events & CAN_ERROR_EVENT) {
            CAN_HandleErrorRecovery();
        }
    }
}
```

---

## 最佳實踐指南

### 1. 中斷優先級設定
```c
// 建議的中斷優先級配置
void CAN_ConfigureInterrupts(void) {
    // CAN 中斷設為較高優先級 (數值越小優先級越高)
    NVIC_SetPriority(CAN0_0_IRQn, 2);   // 高優先級
    NVIC_SetPriority(CAN0_1_IRQn, 2);   
    
    // 其他系統中斷
    NVIC_SetPriority(SysTick_IRQn, 15); // 低優先級
    
    NVIC_EnableIRQ(CAN0_0_IRQn);
    NVIC_EnableIRQ(CAN0_1_IRQn);
}
```

### 2. 中斷服務程序原則
```c
void CAN0_0_IRQHandler(void) {
    // ✅ 好的做法
    // 1. 快速讀取狀態
    uint32_t status = CAN->MSPND[0];
    
    // 2. 最小化處理時間
    if (status & 0x1) {
        rx_pending_flag = 1;  // 設定標誌
    }
    
    // 3. 清除中斷標誌
    CAN->MSPND[0] = status;
    
    // 4. 通知高層任務
    osEventFlagsSet(can_events, CAN_EVENT_FLAG);
    
    // ❌ 避免的做法
    // - 長時間處理
    // - 呼叫阻塞函數
    // - 複雜的邏輯運算
    // - printf 等輸出函數
}
```

### 3. 錯誤恢復策略
```c
void CAN_HandleErrorRecovery(void) {
    uint32_t node_status = XMC_CAN_NODE0->NSR;
    
    // 匯流排關閉恢復
    if (node_status & CAN_NODE_NSR_BOFF_Msk) {
        // 1. 等待匯流排空閒
        osDelay(100);
        
        // 2. 重新初始化節點
        XMC_CAN_NODE_Reset(XMC_CAN_NODE0);
        
        // 3. 重新配置
        CAN_ReconfigureNode();
        
        // 4. 記錄錯誤
        error_log.busoff_count++;
    }
    
    // 錯誤警告處理
    if (node_status & CAN_NODE_NSR_EWRN_Msk) {
        // 降低傳送頻率
        tx_delay_ms += 10;
        
        // 記錄警告
        error_log.warning_count++;
    }
}
```

### 4. 記憶體和資源管理
```c
// 使用記憶體池管理 CAN 訊息
osMemoryPoolId_t can_msg_pool;

void CAN_InitMemoryPool(void) {
    can_msg_pool = osMemoryPoolNew(64, sizeof(CAN_Message_t), NULL);
}

CAN_Message_t* CAN_AllocMessage(void) {
    return (CAN_Message_t*)osMemoryPoolAlloc(can_msg_pool, 100);
}

void CAN_FreeMessage(CAN_Message_t *msg) {
    osMemoryPoolFree(can_msg_pool, msg);
}
```

### 5. 除錯和監控
```c
// CAN 除錯監控結構
typedef struct {
    uint32_t tx_total;
    uint32_t rx_total;
    uint32_t tx_errors;
    uint32_t rx_errors;
    uint32_t bus_errors;
    uint32_t last_activity;
} CAN_Monitor_t;

CAN_Monitor_t can_monitor = {0};

void CAN_UpdateMonitor(void) {
    uint32_t current_time = osKernelGetTickCount();
    
    // 更新活動時間戳
    can_monitor.last_activity = current_time;
    
    // 定期輸出統計
    static uint32_t last_print = 0;
    if ((current_time - last_print) > 10000) {  // 每10秒
        printf("CAN 統計: TX=%lu, RX=%lu, 錯誤=%lu\n", 
               can_monitor.tx_total, 
               can_monitor.rx_total, 
               can_monitor.bus_errors);
        last_print = current_time;
    }
}
```

---

## ⚠️ 重要注意事項

### 中斷安全性
1. **原子操作**: 使用適當的同步機制
2. **中斷優先級**: 避免中斷巢套問題
3. **共享變數**: 使用 volatile 關鍵字
4. **臨界區**: 適當使用中斷禁用/啟用

### 性能優化
1. **中斷延遲**: 保持 ISR 簡短
2. **資料結構**: 使用環形緩衝區
3. **記憶體對齊**: 注意結構體對齊
4. **快取友好**: 考慮資料局部性

### 可靠性
1. **錯誤檢測**: 實現完整的錯誤處理
2. **超時機制**: 避免無限等待
3. **資源洩漏**: 適當釋放資源
4. **看門狗**: 實現系統監控

---

本文檔提供了 XMC CAN 事件與中斷系統的完整說明。正確使用這些功能能夠建立高效、可靠的 CAN 通訊系統。