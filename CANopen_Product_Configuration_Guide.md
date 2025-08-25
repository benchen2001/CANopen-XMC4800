# CANopen 產品設計配置指南
## XMC4800 + CANopenNode 專業配置

**文檔版本**: 1.0  
**建立日期**: 2025年8月25日  
**適用於**: XMC4800 + DAVE IDE + CANopenNode  
**設計目標**: 專業 CANopen 產品 (4 TPDO + 4 RPDO)

---

## 📋 目錄

1. [設計概述](#設計概述)
2. [CANopen 架構分析](#canopen-架構分析)
3. [Message Object 需求分析](#message-object-需求分析)
4. [DAVE CAN_NODE APP 詳細配置](#dave-can_node-app-詳細配置)
5. [程式碼適配指南](#程式碼適配指南)
6. [測試與驗證](#測試與驗證)
7. [參考資料](#參考資料)

---

## 設計概述

### 🎯 產品規格
- **微控制器**: XMC4800-F144x2048
- **協議棧**: CANopenNode v2.0
- **開發環境**: DAVE IDE 4.5.0
- **CAN 速率**: 250 kbps
- **Node ID**: 可配置 (1-127)

### 📊 CANopen 功能需求
- **TPDO**: 4個傳送程序資料對象
- **RPDO**: 4個接收程序資料對象  
- **SDO**: 完整的服務資料對象支援
- **Emergency**: 緊急訊息支援
- **Heartbeat**: 心跳監控
- **LSS**: 層級設定服務 (可選)

### 🚫 不包含功能
- 測試/除錯 LMO
- 過多的 PDO (支援標準 4+4)
- 開發階段的診斷功能

---

## CANopen 架構分析

### 📡 基於 DS301_profile.eds 的分析

根據您的 DS301_profile.eds 檔案：

```ini
[DeviceInfo]
NrOfRXPDO=4          # 標準支援4個RPDO
NrOfTXPDO=4          # 標準支援4個TPDO  
LSS_Supported=1      # 支援LSS功能
BaudRate_250=1       # 支援250kbps
```

**🎯 完整的標準配置：**
- NrOfRXPDO=4 (完整4個RPDO)
- NrOfTXPDO=4 (完整4個TPDO)
- 符合 CANopen DS301 標準規範

### 🔧 CANopen ID 分配策略

| 功能 | CAN ID 公式 | Node ID=10 範例 | 優先級 | 用途 |
|------|-------------|----------------|--------|------|
| **Emergency** | 0x080 + NodeID | 0x08A | 最高 | 錯誤報告 |
| **TPDO1** | 0x180 + NodeID | 0x18A | 高 | 第1組輸出數據 |
| **TPDO2** | 0x280 + NodeID | 0x28A | 高 | 第2組輸出數據 |
| **TPDO3** | 0x380 + NodeID | 0x38A | 高 | 第3組輸出數據 |
| **TPDO4** | 0x480 + NodeID | 0x48A | 高 | 第4組輸出數據 |
| **RPDO1** | 0x200 + NodeID | 0x20A | 高 | 第1組輸入數據 |
| **RPDO2** | 0x300 + NodeID | 0x30A | 高 | 第2組輸入數據 |
| **RPDO3** | 0x400 + NodeID | 0x40A | 高 | 第3組輸入數據 |
| **RPDO4** | 0x500 + NodeID | 0x50A | 高 | 第4組輸入數據 |
| **SDO TX** | 0x580 + NodeID | 0x58A | 中 | 服務回應 |
| **SDO RX** | 0x600 + NodeID | 0x60A | 中 | 服務請求 |
| **Heartbeat** | 0x700 + NodeID | 0x70A | 低 | 節點監控 |

---

## Message Object 需求分析

### 📊 最小化 LMO 配置

**💡 設計原則：**
- 移除所有測試/除錯 LMO
- 只保留生產必需的功能
- 確保 CANopen 標準相容性
- 最佳化硬體資源使用

### 🎯 **答案：Number of message objects in list = 12**

### 📋 詳細 LMO 分配表

| LMO | 功能 | Type | CAN ID | 說明 | 優先級 |
|-----|------|------|--------|------|--------|
| **LMO_01** | Emergency | TX | 0x080+NodeID | 緊急錯誤訊息 | ⭐⭐⭐ |
| **LMO_02** | TPDO1 | TX | 0x180+NodeID | 第1組傳送數據 | ⭐⭐⭐ |
| **LMO_03** | TPDO2 | TX | 0x280+NodeID | 第2組傳送數據 | ⭐⭐⭐ |
| **LMO_04** | TPDO3 | TX | 0x380+NodeID | 第3組傳送數據 | ⭐⭐ |
| **LMO_05** | TPDO4 | TX | 0x480+NodeID | 第4組傳送數據 | ⭐⭐ |
| **LMO_06** | SDO TX | TX | 0x580+NodeID | SDO 服務回應 | ⭐⭐ |
| **LMO_07** | Heartbeat | TX | 0x700+NodeID | 心跳訊息 | ⭐ |
| **LMO_08** | SDO RX | RX | 0x600+NodeID | SDO 服務請求 | ⭐⭐⭐ |
| **LMO_09** | RPDO1 | RX | 0x200+NodeID | 第1組接收數據 | ⭐⭐⭐ |
| **LMO_10** | RPDO2 | RX | 0x300+NodeID | 第2組接收數據 | ⭐⭐⭐ |
| **LMO_11** | RPDO3 | RX | 0x400+NodeID | 第3組接收數據 | ⭐⭐ |
| **LMO_12** | RPDO4 | RX | 0x500+NodeID | 第4組接收數據 | ⭐⭐ |

**總計：12 個 LMO**
- 傳送 LMO：7 個
- 接收 LMO：5 個

---

## DAVE CAN_NODE APP 詳細配置

### 🛠️ General Settings 配置

```
┌─────────────────────────────────────┐
│ General Settings                    │
├─────────────────────────────────────┤
│ Number of message objects: 12       │
│ Baudrate: 250000                    │
│ Sample Point: 75%                   │
│ Sync Jump Width: 1                  │
│ Node ID: 10 (可依需求調整)           │
└─────────────────────────────────────┘
```

### 📋 MO Settings Page 1 詳細配置

| 欄位 | LMO_01 | LMO_02 | LMO_03 | LMO_04 | LMO_05 | LMO_06 | LMO_07 | LMO_08 | LMO_09 | LMO_10 | LMO_11 | LMO_12 |
|------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|--------|
| **Logical MO** | MO_01 | MO_02 | MO_03 | MO_04 | MO_05 | MO_06 | MO_07 | MO_08 | MO_09 | MO_10 | MO_11 | MO_12 |
| **Message Type** | Tx | Tx | Tx | Tx | Tx | Tx | Tx | Rx | Rx | Rx | Rx | Rx |
| **Identifier Type** | Std_11bit | Std_11bit | Std_11bit | Std_11bit | Std_11bit | Std_11bit | Std_11bit | Std_11bit | Std_11bit | Std_11bit | Std_11bit | Std_11bit |
| **Identifier Value** | 0x08A | 0x18A | 0x28A | 0x38A | 0x48A | 0x58A | 0x70A | 0x60A | 0x20A | 0x30A | 0x40A | 0x50A |
| **Acceptance** | Matching_IDI | Matching_IDI | Matching_IDI | Matching_IDI | Matching_IDI | Matching_IDI | Matching_IDI | Matching_IDI | Matching_IDI | Matching_IDI | Matching_IDI | Matching_IDI |
| **Mask Value** | 0x7FF | 0x7FF | 0x7FF | 0x7FF | 0x7FF | 0x7FF | 0x7FF | 0x7FF | 0x7FF | 0x7FF | 0x7FF | 0x7FF |
| **DLC** | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 |
| **Data_H** | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 |
| **Data_L** | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 | 0x0 |
| **Tx Event** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| **Rx Event** | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ✅ | ✅ | ✅ | ✅ |

**注意**: 以上 Identifier Value 假設 Node ID = 10，請依實際需求調整。

### ⚙️ Advanced Settings 配置

```
┌─────────────────────────────────────┐
│ Advanced Settings                   │
├─────────────────────────────────────┤
│ Service Request Line: 0 (統一中斷)   │
│ CAN Frame Type: Standard            │
│ Loop Back Mode: Disabled            │
│ Monitoring Mode: Disabled           │
└─────────────────────────────────────┘
```

### 🔔 Event Settings 配置

```
┌─────────────────────────────────────┐
│ Event Settings                      │
├─────────────────────────────────────┤
│ TX Event Enabled: Yes               │
│ RX Event Enabled: Yes               │
│ Error Event Enabled: Yes            │
│ Alert Event Enabled: No             │
└─────────────────────────────────────┘
```

---

## 程式碼適配指南

### 🔧 CO_driver_XMC4800.c 修改

#### 1. 更新 LMO 枚舉定義

```c
/* 完整 CANopen 產品 LMO 分配 - 4 TPDO + 4 RPDO */
typedef enum {
    CANOPEN_LMO_EMERGENCY = 0,    /* LMO_01: Emergency (0x080+NodeID) */
    CANOPEN_LMO_TPDO1,            /* LMO_02: TPDO1 (0x180+NodeID) */
    CANOPEN_LMO_TPDO2,            /* LMO_03: TPDO2 (0x280+NodeID) */
    CANOPEN_LMO_TPDO3,            /* LMO_04: TPDO3 (0x380+NodeID) */
    CANOPEN_LMO_TPDO4,            /* LMO_05: TPDO4 (0x480+NodeID) */
    CANOPEN_LMO_SDO_TX,           /* LMO_06: SDO TX (0x580+NodeID) */
    CANOPEN_LMO_HEARTBEAT,        /* LMO_07: Heartbeat (0x700+NodeID) */
    CANOPEN_LMO_SDO_RX,           /* LMO_08: SDO RX (0x600+NodeID) */
    CANOPEN_LMO_RPDO1,            /* LMO_09: RPDO1 (0x200+NodeID) */
    CANOPEN_LMO_RPDO2,            /* LMO_10: RPDO2 (0x300+NodeID) */
    CANOPEN_LMO_RPDO3,            /* LMO_11: RPDO3 (0x400+NodeID) */
    CANOPEN_LMO_RPDO4,            /* LMO_12: RPDO4 (0x500+NodeID) */
    CANOPEN_LMO_COUNT = 12        /* 總共 12 個 LMO */
} canopen_lmo_index_t;
```

#### 2. 更新 ID 計算宏

```c
/* CANopen 標準 ID 定義 - 完整版本 */
#define CANOPEN_EMERGENCY_ID(node_id)      (0x080U + (node_id))
#define CANOPEN_TPDO1_ID(node_id)          (0x180U + (node_id))
#define CANOPEN_TPDO2_ID(node_id)          (0x280U + (node_id))
#define CANOPEN_TPDO3_ID(node_id)          (0x380U + (node_id))
#define CANOPEN_TPDO4_ID(node_id)          (0x480U + (node_id))
#define CANOPEN_RPDO1_ID(node_id)          (0x200U + (node_id))
#define CANOPEN_RPDO2_ID(node_id)          (0x300U + (node_id))
#define CANOPEN_RPDO3_ID(node_id)          (0x400U + (node_id))
#define CANOPEN_RPDO4_ID(node_id)          (0x500U + (node_id))
#define CANOPEN_SDO_TX_ID(node_id)         (0x580U + (node_id))
#define CANOPEN_SDO_RX_ID(node_id)         (0x600U + (node_id))
#define CANOPEN_HEARTBEAT_ID(node_id)      (0x700U + (node_id))
```

#### 3. 更新 LMO 選擇邏輯

```c
/**
 * @brief 根據 CAN ID 選擇合適的 LMO - 完整版本
 */
static uint32_t canopen_get_lmo_index_for_id(uint32_t can_id)
{
    uint8_t node_id = canopen_get_node_id();
    
    /* Emergency 訊息 */
    if (can_id == CANOPEN_EMERGENCY_ID(node_id)) {
        return CANOPEN_LMO_EMERGENCY;
    }
    
    /* TPDO 訊息 */
    if (can_id == CANOPEN_TPDO1_ID(node_id)) {
        return CANOPEN_LMO_TPDO1;
    }
    if (can_id == CANOPEN_TPDO2_ID(node_id)) {
        return CANOPEN_LMO_TPDO2;
    }
    if (can_id == CANOPEN_TPDO3_ID(node_id)) {
        return CANOPEN_LMO_TPDO3;
    }
    if (can_id == CANOPEN_TPDO4_ID(node_id)) {
        return CANOPEN_LMO_TPDO4;
    }
    
    /* SDO TX 訊息 */
    if (can_id == CANOPEN_SDO_TX_ID(node_id)) {
        return CANOPEN_LMO_SDO_TX;
    }
    
    /* Heartbeat 訊息 */
    if (can_id == CANOPEN_HEARTBEAT_ID(node_id)) {
        return CANOPEN_LMO_HEARTBEAT;
    }
    
    /* 預設使用 Emergency LMO */
    return CANOPEN_LMO_EMERGENCY;
}
```

### 📝 OD.c 配置對應

確保物件字典與 LMO 配置一致：

```c
/* TPDO 配置 - 完整配置 4 個 */
.x1800_TPDOCommunicationParameter = {
    .highestSub_indexSupported = 0x06,
    .COB_IDUsedByTPDO = 0xC0000180,  // TPDO1: 0x180+NodeID
    .transmissionType = 0xFE,
    .inhibitTime = 0x0000,
    .eventTimer = 0x0000,
    .SYNCStartValue = 0x00
},
.x1801_TPDOCommunicationParameter = {
    .highestSub_indexSupported = 0x06,
    .COB_IDUsedByTPDO = 0xC0000280,  // TPDO2: 0x280+NodeID
    .transmissionType = 0xFE,
    .inhibitTime = 0x0000,
    .eventTimer = 0x0000,
    .SYNCStartValue = 0x00
},
.x1802_TPDOCommunicationParameter = {
    .highestSub_indexSupported = 0x06,
    .COB_IDUsedByTPDO = 0xC0000380,  // TPDO3: 0x380+NodeID
    .transmissionType = 0xFE,
    .inhibitTime = 0x0000,
    .eventTimer = 0x0000,
    .SYNCStartValue = 0x00
},
.x1803_TPDOCommunicationParameter = {
    .highestSub_indexSupported = 0x06,
    .COB_IDUsedByTPDO = 0xC0000480,  // TPDO4: 0x480+NodeID
    .transmissionType = 0xFE,
    .inhibitTime = 0x0000,
    .eventTimer = 0x0000,
    .SYNCStartValue = 0x00
},

/* RPDO 配置 - 完整配置 4 個 */
.x1400_RPDOCommunicationParameter = {
    .highestSub_indexSupported = 0x05,
    .COB_IDUsedByRPDO = 0x80000200,  // RPDO1: 0x200+NodeID
    .transmissionType = 0xFE,
    .eventTimer = 0x0000
},
.x1401_RPDOCommunicationParameter = {
    .highestSub_indexSupported = 0x05,
    .COB_IDUsedByRPDO = 0x80000300,  // RPDO2: 0x300+NodeID
    .transmissionType = 0xFE,
    .eventTimer = 0x0000
},
.x1402_RPDOCommunicationParameter = {
    .highestSub_indexSupported = 0x05,
    .COB_IDUsedByRPDO = 0x80000400,  // RPDO3: 0x400+NodeID
    .transmissionType = 0xFE,
    .eventTimer = 0x0000
},
.x1403_RPDOCommunicationParameter = {
    .highestSub_indexSupported = 0x05,
    .COB_IDUsedByRPDO = 0x80000500,  // RPDO4: 0x500+NodeID
    .transmissionType = 0xFE,
    .eventTimer = 0x0000
},
```

---

## 測試與驗證

### 🧪 功能測試清單

#### 1. 基本通訊測試
- [ ] Emergency 訊息發送
- [ ] SDO 讀寫操作
- [ ] Heartbeat 定期發送
- [ ] TPDO1/TPDO2/TPDO3/TPDO4 數據傳送
- [ ] RPDO1/RPDO2/RPDO3/RPDO4 數據接收

#### 2. CANopen 相容性測試
- [ ] NMT 狀態機正常運作
- [ ] PDO 映射配置正確
- [ ] 錯誤處理機制
- [ ] 節點保護功能

#### 3. 性能測試
- [ ] CAN 總線負載測試
- [ ] 中斷響應時間
- [ ] 記憶體使用優化
- [ ] 長時間穩定性

### 📊 CAN 分析儀監控

使用 CAN 分析儀驗證以下訊息：

```
Node ID = 10 的預期訊息:
┌──────────────┬─────────┬─────────────────┐
│ 功能         │ CAN ID  │ 週期/觸發       │
├──────────────┼─────────┼─────────────────┤
│ Emergency    │ 0x08A   │ 錯誤時觸發      │
│ TPDO1        │ 0x18A   │ 事件驅動        │
│ TPDO2        │ 0x28A   │ 事件驅動        │
│ TPDO3        │ 0x38A   │ 事件驅動        │
│ TPDO4        │ 0x48A   │ 事件驅動        │
│ SDO Response │ 0x58A   │ SDO請求時回應    │
│ Heartbeat    │ 0x70A   │ 週期性 (1秒)    │
│ SDO Request  │ 0x60A   │ 主站發起        │
│ RPDO1        │ 0x20A   │ 外部發送        │
│ RPDO2        │ 0x30A   │ 外部發送        │
│ RPDO3        │ 0x40A   │ 外部發送        │
│ RPDO4        │ 0x50A   │ 外部發送        │
└──────────────┴─────────┴─────────────────┘
```

---

## 參考資料

### 📚 技術文檔
1. **CANopen DS301 標準** - 應用層和通訊配置
2. **CANopenNode 文檔** - 開源協議棧參考
3. **XMC4800 Reference Manual** - 硬體規格
4. **DAVE APP User Guide** - CAN_NODE 配置

### 🔗 相關檔案
- `DS301_profile.eds` - CANopen 設備描述
- `CO_driver_XMC4800.c` - 硬體驅動層
- `OD.c` - 物件字典定義
- `main.c` - 主應用程式

### 💡 設計原則
1. **最小化配置** - 只保留生產必需功能
2. **標準相容** - 嚴格遵循 CANopen DS301
3. **資源最佳化** - 有效利用 XMC4800 硬體
4. **維護性** - 程式碼清晰易懂
5. **可測試性** - 支援標準測試工具

---

## 🎯 總結

這份配置為您提供了一個完整的專業級 CANopen 產品設計方案：

✅ **12 個 LMO** - 完整標準配置  
✅ **4 TPDO + 4 RPDO** - 符合 DS301 標準  
✅ **無測試/除錯功能** - 生產就緒  
✅ **完整 CANopen 相容** - DS301 標準規範  
✅ **XMC4800 最佳化** - 硬體效能最大化

按照此配置，您將獲得一個高效、穩定、完全符合 CANopen DS301 標準的專業產品！

---

**文檔結束**