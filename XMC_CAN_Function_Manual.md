# XMC CAN 函數使用手冊

## 📋 目錄
1. [CAN 模組初始化](#can-模組初始化)
2. [CAN 節點配置](#can-節點配置)
3. [訊息物件 (Message Object) 配置](#訊息物件-message-object-配置)
4. [CAN 傳送功能](#can-傳送功能)
5. [CAN 接收功能](#can-接收功能)
6. [CAN 事件與中斷](#can-事件與中斷)
7. [FIFO 操作](#fifo-操作)
8. [閘道功能](#閘道功能)
9. [錯誤處理](#錯誤處理)
10. [實用工具函數](#實用工具函數)

---

## CAN 模組初始化

### `XMC_CAN_Enable()`
**功能**: 啟用 CAN 周邊設備
```c
void XMC_CAN_Enable(XMC_CAN_t *const obj);
```
**參數**:
- `obj`: CAN 物件指標 (例如: `CAN`)

**說明**: 
- 取消時鐘閘控 (Clock Gating)
- 解除周邊設備重置
- 等待模組準備就緒

**使用範例**:
```c
// 啟用 CAN 模組
XMC_CAN_Enable(CAN);
```

### `XMC_CAN_Disable()`
**功能**: 停用 CAN 周邊設備
```c
void XMC_CAN_Disable(XMC_CAN_t *const obj);
```

### `XMC_CAN_Init()` / `XMC_CAN_InitEx()`
**功能**: 初始化 CAN 模組
```c
// 基本版本
void XMC_CAN_Init(XMC_CAN_t *const obj, uint32_t can_frequency);

// 進階版本 (支援時鐘源選擇)
uint32_t XMC_CAN_InitEx(XMC_CAN_t *const obj, 
                        XMC_CAN_CANCLKSRC_t clksrc, 
                        uint32_t can_frequency);
```

**參數**:
- `obj`: CAN 物件指標
- `can_frequency`: CAN 時鐘頻率 (Hz)
- `clksrc`: 時鐘源選擇 (僅 InitEx)

**使用範例**:
```c
// 基本初始化
XMC_CAN_Init(CAN, 120000000);  // 120MHz

// 進階初始化
uint32_t actual_freq = XMC_CAN_InitEx(CAN, XMC_CAN_CANCLKSRC_FPERI, 120000000);
```

---

## CAN 節點配置

### `XMC_CAN_NODE_NominalBitTimeConfigure()`
**功能**: 配置 CAN 節點的位元時序
```c
void XMC_CAN_NODE_NominalBitTimeConfigure(
    XMC_CAN_NODE_t *const can_node,
    const XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t *const can_bit_time
);
```

**XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t 結構**:
```c
typedef struct {
    uint32_t can_frequency;    // CAN 時鐘頻率
    uint32_t baudrate;         // 波特率
    uint16_t sample_point;     // 採樣點 (‰)
    uint16_t sjw;              // 同步跳躍寬度
} XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t;
```

**使用範例**:
```c
// 配置 500kbps 波特率
XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t bit_time_config = {
    .can_frequency = 120000000,  // 120MHz
    .baudrate = 500000,          // 500kbps
    .sample_point = 8000,        // 80% 採樣點
    .sjw = 1                     // SJW = 1
};

XMC_CAN_NODE_NominalBitTimeConfigure(&XMC_CAN_NODE0, &bit_time_config);
```

### `XMC_CAN_AllocateMOtoNodeList()`
**功能**: 將訊息物件分配給節點
```c
void XMC_CAN_AllocateMOtoNodeList(XMC_CAN_t *const obj, 
                                  const uint8_t node_num, 
                                  const uint8_t mo_num);
```

---

## 訊息物件 (Message Object) 配置

### `XMC_CAN_MO_Config()`
**功能**: 配置訊息物件
```c
void XMC_CAN_MO_Config(const XMC_CAN_MO_t *const can_mo);
```

**XMC_CAN_MO_t 結構重要成員**:
```c
typedef struct {
    XMC_CAN_MO_TYPE_t can_mo_type;      // MO 類型
    uint32_t can_identifier;            // CAN ID
    uint32_t can_id_mask;               // ID 遮罩
    uint8_t can_id_mode;                // ID 模式 (標準/擴展)
    uint8_t can_data_length;            // 資料長度
    uint8_t can_data[8];                // 資料內容
    XMC_CAN_MO_t *can_mo_ptr;          // MO 指標
    // 其他成員...
} XMC_CAN_MO_t;
```

**訊息物件類型**:
```c
typedef enum {
    XMC_CAN_MO_TYPE_RECMSGOBJ = 1,     // 接收訊息物件
    XMC_CAN_MO_TYPE_TRANSMSGOBJ = 2    // 傳送訊息物件
} XMC_CAN_MO_TYPE_t;
```

**使用範例**:
```c
// 定義傳送 MO
XMC_CAN_MO_t tx_mo = {
    .can_mo_type = XMC_CAN_MO_TYPE_TRANSMSGOBJ,
    .can_identifier = 0x123,
    .can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS,
    .can_data_length = 8,
    .can_data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
};

// 配置 MO
XMC_CAN_MO_Config(&tx_mo);
```

### `XMC_CAN_MO_SetIdentifier()` / `XMC_CAN_MO_GetIdentifier()`
**功能**: 設定/取得 CAN ID
```c
void XMC_CAN_MO_SetIdentifier(XMC_CAN_MO_t *const can_mo, 
                              const uint32_t can_identifier);

uint32_t XMC_CAN_MO_GetIdentifier(const XMC_CAN_MO_t *const can_mo);
```

### `XMC_CAN_MO_SetAcceptanceMask()` / `XMC_CAN_MO_GetAcceptanceMask()`
**功能**: 設定/取得接收遮罩
```c
void XMC_CAN_MO_SetAcceptanceMask(XMC_CAN_MO_t *const can_mo, 
                                  const uint32_t can_id_mask);

uint32_t XMC_CAN_MO_GetAcceptanceMask(const XMC_CAN_MO_t *const can_mo);
```

---

## CAN 傳送功能

### `XMC_CAN_MO_UpdateData()`
**功能**: 更新訊息物件的資料
```c
XMC_CAN_STATUS_t XMC_CAN_MO_UpdateData(const XMC_CAN_MO_t *const can_mo);
```

### `XMC_CAN_MO_Transmit()`
**功能**: 傳送訊息
```c
XMC_CAN_STATUS_t XMC_CAN_MO_Transmit(const XMC_CAN_MO_t *const can_mo);
```

**返回值**: `XMC_CAN_STATUS_t` 枚舉
```c
typedef enum {
    XMC_CAN_STATUS_SUCCESS = 0,
    XMC_CAN_STATUS_ERROR,
    XMC_CAN_STATUS_BUSY
} XMC_CAN_STATUS_t;
```

**完整傳送範例**:
```c
// 1. 設定資料
uint8_t tx_data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
memcpy(tx_mo.can_data, tx_data, 8);
tx_mo.can_data_length = 8;

// 2. 更新資料到硬體
XMC_CAN_STATUS_t status = XMC_CAN_MO_UpdateData(&tx_mo);
if (status == XMC_CAN_STATUS_SUCCESS) {
    // 3. 發送訊息
    status = XMC_CAN_MO_Transmit(&tx_mo);
    if (status == XMC_CAN_STATUS_SUCCESS) {
        printf("訊息發送成功\n");
    }
}
```

---

## CAN 接收功能

### `XMC_CAN_MO_Receive()`
**功能**: 接收訊息 (包含狀態檢查)
```c
XMC_CAN_STATUS_t XMC_CAN_MO_Receive(XMC_CAN_MO_t *can_mo);
```

### `XMC_CAN_MO_ReceiveData()`
**功能**: 僅讀取資料 (不檢查狀態)
```c
XMC_CAN_STATUS_t XMC_CAN_MO_ReceiveData(XMC_CAN_MO_t *can_mo);
```

**接收範例**:
```c
// 定義接收 MO
XMC_CAN_MO_t rx_mo = {
    .can_mo_type = XMC_CAN_MO_TYPE_RECMSGOBJ,
    .can_identifier = 0x123,
    .can_id_mask = 0x7FF,  // 精確匹配
    .can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS
};

// 配置接收 MO
XMC_CAN_MO_Config(&rx_mo);

// 接收處理 (通常在中斷或輪詢中)
XMC_CAN_STATUS_t status = XMC_CAN_MO_Receive(&rx_mo);
if (status == XMC_CAN_STATUS_SUCCESS) {
    printf("收到訊息 ID=0x%03X, DLC=%d\n", 
           rx_mo.can_identifier, rx_mo.can_data_length);
    
    for (int i = 0; i < rx_mo.can_data_length; i++) {
        printf("資料[%d] = 0x%02X\n", i, rx_mo.can_data[i]);
    }
}
```

---

## CAN 事件與中斷

### `XMC_CAN_NODE_EnableEvent()` / `XMC_CAN_NODE_DisableEvent()`
**功能**: 啟用/停用節點事件
```c
void XMC_CAN_NODE_EnableEvent(XMC_CAN_NODE_t *const can_node, 
                              const XMC_CAN_NODE_EVENT_t event);

void XMC_CAN_NODE_DisableEvent(XMC_CAN_NODE_t *const can_node, 
                               const XMC_CAN_NODE_EVENT_t event);
```

**CAN 節點事件類型**:
```c
typedef enum {
    XMC_CAN_NODE_EVENT_CFCIE = CAN_NODE_NCR_CFCIE_Msk,    // 時鐘失效
    XMC_CAN_NODE_EVENT_LEIE = CAN_NODE_NCR_LEIE_Msk,      // 最後錯誤代碼
    XMC_CAN_NODE_EVENT_ALIE = CAN_NODE_NCR_ALIE_Msk,      // 警報限制
    XMC_CAN_NODE_EVENT_CANDIS = CAN_NODE_NCR_CANDIS_Msk   // 節點停用
} XMC_CAN_NODE_EVENT_t;
```

### 訊息物件事件

**重要的 MO 狀態位元**:
```c
// 在 xmc_can.h 中定義的狀態常數
#define XMC_CAN_MO_STATUS_RX_PENDING    (1U << 0)   // 接收待處理
#define XMC_CAN_MO_STATUS_TX_PENDING    (1U << 1)   // 傳送待處理
#define XMC_CAN_MO_STATUS_NEW_DATA      (1U << 3)   // 新資料
#define XMC_CAN_MO_STATUS_MESSAGE_VALID (1U << 5)   // 訊息有效
```

**檢查 MO 狀態**:
```c
uint32_t XMC_CAN_MO_GetStatus(const XMC_CAN_MO_t *const can_mo);
void XMC_CAN_MO_ResetStatus(XMC_CAN_MO_t *const can_mo, uint32_t status_mask);
```

---

## FIFO 操作

### 傳送 FIFO
```c
// 配置 FIFO 基本物件
void XMC_CAN_TXFIFO_ConfigMOBaseObject(const XMC_CAN_MO_t *const can_mo, 
                                       const XMC_CAN_FIFO_CONFIG_t can_fifo);

// 配置 FIFO 從屬物件
void XMC_CAN_TXFIFO_ConfigMOSlaveObject(const XMC_CAN_MO_t *const can_mo, 
                                        const XMC_CAN_FIFO_CONFIG_t can_fifo);

// FIFO 傳送
XMC_CAN_STATUS_t XMC_CAN_TXFIFO_Transmit(const XMC_CAN_MO_t *const can_mo);
```

### 接收 FIFO
```c
// 配置接收 FIFO 基本物件
void XMC_CAN_RXFIFO_ConfigMOBaseObject(const XMC_CAN_MO_t *const can_mo, 
                                       const XMC_CAN_FIFO_CONFIG_t can_fifo);
```

**FIFO 配置結構**:
```c
typedef struct {
    uint8_t fifo_depth;         // FIFO 深度
    uint8_t fifo_pointer;       // FIFO 指標
    // 其他配置...
} XMC_CAN_FIFO_CONFIG_t;
```

---

## 閘道功能

### `XMC_CAN_GATEWAY_InitSourceObject()`
**功能**: 初始化閘道源物件
```c
void XMC_CAN_GATEWAY_InitSourceObject(const XMC_CAN_MO_t *const can_mo, 
                                      const XMC_CAN_GATEWAY_CONFIG_t can_gateway);
```

---

## 錯誤處理

### 錯誤狀態檢查
```c
// 取得節點狀態
uint32_t node_status = can_node->NSR;

// 檢查特定錯誤
if (node_status & CAN_NODE_NSR_EWRN_Msk) {
    printf("警告限制到達\n");
}

if (node_status & CAN_NODE_NSR_BOFF_Msk) {
    printf("匯流排關閉\n");
}
```

### 常見錯誤代碼
```c
typedef enum {
    XMC_CAN_ERROR_NONE = 0,
    XMC_CAN_ERROR_STUFF,        // 填充錯誤
    XMC_CAN_ERROR_FORM,         // 格式錯誤
    XMC_CAN_ERROR_ACK,          // 確認錯誤
    XMC_CAN_ERROR_BIT1,         // 位元 1 錯誤
    XMC_CAN_ERROR_BIT0,         // 位元 0 錯誤
    XMC_CAN_ERROR_CRC           // CRC 錯誤
} XMC_CAN_ERROR_t;
```

---

## 實用工具函數

### 時鐘相關
```c
// 設定/取得波特率時鐘源
void XMC_CAN_SetBaudrateClockSource(XMC_CAN_t *const obj, 
                                    const XMC_CAN_CANCLKSRC_t source);

XMC_CAN_CANCLKSRC_t XMC_CAN_GetBaudrateClockSource(XMC_CAN_t *const obj);

// 取得時鐘頻率
uint32_t XMC_CAN_GetBaudrateClockFrequency(XMC_CAN_t *const obj);
uint32_t XMC_CAN_GetClockFrequency(XMC_CAN_t *const obj);
```

### 位元時序計算
```c
// 進階位元時序配置 (返回實際配置結果)
int32_t XMC_CAN_NODE_NominalBitTimeConfigureEx(
    XMC_CAN_NODE_t *const can_node,
    const XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t *const bit_time_config
);
```

---

## 📚 完整使用範例

### 基本傳送接收範例
```c
#include "xmc_can.h"

// 全域變數
XMC_CAN_MO_t tx_mo, rx_mo;

void CAN_Init(void) {
    // 1. 啟用 CAN 模組
    XMC_CAN_Enable(CAN);
    
    // 2. 初始化 CAN
    XMC_CAN_Init(CAN, 120000000);
    
    // 3. 配置位元時序
    XMC_CAN_NODE_NOMINAL_BIT_TIME_CONFIG_t bit_time = {
        .can_frequency = 120000000,
        .baudrate = 500000,
        .sample_point = 8000,
        .sjw = 1
    };
    XMC_CAN_NODE_NominalBitTimeConfigure(XMC_CAN_NODE0, &bit_time);
    
    // 4. 配置傳送 MO
    tx_mo.can_mo_type = XMC_CAN_MO_TYPE_TRANSMSGOBJ;
    tx_mo.can_identifier = 0x123;
    tx_mo.can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS;
    tx_mo.can_data_length = 8;
    XMC_CAN_MO_Config(&tx_mo);
    
    // 5. 配置接收 MO
    rx_mo.can_mo_type = XMC_CAN_MO_TYPE_RECMSGOBJ;
    rx_mo.can_identifier = 0x456;
    rx_mo.can_id_mask = 0x7FF;
    rx_mo.can_id_mode = XMC_CAN_FRAME_TYPE_STANDARD_11BITS;
    XMC_CAN_MO_Config(&rx_mo);
    
    // 6. 分配 MO 到節點
    XMC_CAN_AllocateMOtoNodeList(CAN, 0, 0);  // TX MO
    XMC_CAN_AllocateMOtoNodeList(CAN, 0, 1);  // RX MO
}

void CAN_SendMessage(uint32_t id, uint8_t *data, uint8_t length) {
    tx_mo.can_identifier = id;
    tx_mo.can_data_length = length;
    memcpy(tx_mo.can_data, data, length);
    
    if (XMC_CAN_MO_UpdateData(&tx_mo) == XMC_CAN_STATUS_SUCCESS) {
        XMC_CAN_MO_Transmit(&tx_mo);
    }
}

bool CAN_ReceiveMessage(uint32_t *id, uint8_t *data, uint8_t *length) {
    if (XMC_CAN_MO_Receive(&rx_mo) == XMC_CAN_STATUS_SUCCESS) {
        *id = rx_mo.can_identifier;
        *length = rx_mo.can_data_length;
        memcpy(data, rx_mo.can_data, rx_mo.can_data_length);
        return true;
    }
    return false;
}
```

---

## ⚠️ 重要注意事項

1. **初始化順序**: 必須先啟用模組，再進行其他配置
2. **時鐘配置**: 確保 CAN 時鐘頻率與系統時鐘匹配
3. **MO 分配**: 每個 MO 必須分配給相應的節點
4. **中斷處理**: 在中斷服務程序中使用非阻塞函數
5. **錯誤處理**: 始終檢查函數返回值
6. **資源管理**: 適當釋放不使用的 MO

---

本手冊涵蓋了 XMC CAN 庫的主要功能。如需更詳細的信息，請參考 Infineon 官方文檔和範例程式碼。