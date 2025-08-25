# XMC4800 CANopen 監控程式設計規劃書

## 1. 專案目標與需求分析

### 1.1 核心目標
- 實現專業級 CANopen 網路監控程式
- 支援 CANopen 協議標準 (CiA 301)
- 提供即時網路狀態監控
- 支援多節點通訊分析

### 1.2 功能需求
- **網路監控**: 監控 CANopen 網路上所有訊息
- **節點管理**: 檢測網路上的 CANopen 節點
- **訊息解析**: 解析 PDO、SDO、NMT、Emergency 等訊息
- **狀態顯示**: 透過 LED 和 UART 顯示系統狀態
- **資料記錄**: 記錄網路活動和異常事件

## 2. 技術方案比較

### 2.1 方案一：使用 CANopenNode 庫
**優點：**
- ✅ 完整的 CANopen 協議實現
- ✅ 符合 CiA 301 標準
- ✅ 豐富的功能模組 (PDO, SDO, NMT, Emergency)
- ✅ 經過驗證的穩定性
- ✅ 開源且持續維護

**缺點：**
- ❌ 程式碼複雜度高
- ❌ 記憶體需求較大
- ❌ 整合難度較高
- ❌ 學習曲線陡峭

### 2.2 方案二：自製輕量級監控程式
**優點：**
- ✅ 程式碼簡潔易懂
- ✅ 記憶體需求小
- ✅ 針對監控需求優化
- ✅ 快速開發
- ✅ 容易除錯和維護

**缺點：**
- ❌ 需要手動實現協議解析
- ❌ 可能不完全符合標準
- ❌ 功能較為有限
- ❌ 需要大量測試驗證

## 3. 推薦方案：自製輕量級監控程式

### 3.1 選擇理由
對於監控程式來說，我們**不需要**成為完整的 CANopen 節點，只需要能夠：
- 接收和解析 CAN 訊息
- 識別 CANopen 訊息類型
- 提供監控和診斷功能

因此，**自製輕量級方案**更適合，因為：
1. **監控程式不需要完整的 CANopen 堆疊**
2. **簡化的實現更容易理解和維護**
3. **可以專注於監控功能而非完整節點功能**

## 4. 系統架構設計

### 4.1 硬體架構
```
XMC4800 Microcontroller
├── CAN Controller (120Ω 終端電阻)
├── UART Debug Interface (115200 baud)
├── LED Status Indicators
│   ├── LED1: Power/Activity
│   └── LED2: CAN Communication
└── J-Link Debug Interface
```

### 4.2 軟體架構
```
Application Layer
├── CANopen Message Parser
├── Network Monitor
├── Status Reporter
└── Debug Interface

Protocol Layer
├── CAN Message Filter
├── CANopen ID Decoder
├── Message Type Classifier
└── Data Formatter

Hardware Layer
├── DAVE CAN Driver
├── DAVE UART Driver
└── DAVE GPIO Driver
```

## 5. CANopen 協議實現範圍

### 5.1 訊息類型支援
```
CANopen 訊息 ID 分配 (11-bit CAN ID)
├── 0x000: NMT (Network Management)
├── 0x080: SYNC/TIME
├── 0x100: Emergency Messages  
├── 0x180-0x1FF: PDO1 Transmit
├── 0x200-0x27F: PDO1 Receive
├── 0x280-0x2FF: PDO2 Transmit
├── 0x300-0x37F: PDO2 Receive
├── 0x380-0x3FF: PDO3 Transmit
├── 0x400-0x47F: PDO3 Receive
├── 0x480-0x4FF: PDO4 Transmit
├── 0x500-0x57F: PDO4 Receive
├── 0x580-0x5FF: SDO Server Response
├── 0x600-0x67F: SDO Client Request
└── 0x700-0x77F: Heartbeat/Node Guard
```

### 5.2 監控功能實現
1. **NMT 狀態監控**: 監控節點狀態變化
2. **PDO 資料監控**: 解析過程資料
3. **SDO 通訊監控**: 監控服務資料傳輸
4. **心跳監控**: 檢測節點活動狀態
5. **緊急訊息監控**: 檢測系統異常

## 6. 程式模組設計

### 6.1 核心模組
```c
/* CANopen 監控核心 */
typedef struct {
    uint32_t node_id;
    uint32_t msg_count;
    uint32_t last_heartbeat;
    uint8_t nmt_state;
    bool is_active;
} canopen_node_t;

typedef struct {
    uint32_t can_id;
    uint8_t data[8];
    uint8_t dlc;
    uint32_t timestamp;
    uint8_t msg_type;
} canopen_message_t;
```

### 6.2 功能模組
```c
/* 訊息解析器 */
void canopen_parse_message(canopen_message_t* msg);
uint8_t canopen_get_message_type(uint32_t can_id);
uint8_t canopen_get_node_id(uint32_t can_id);

/* 網路監控器 */
void canopen_monitor_init(void);
void canopen_monitor_process(void);
void canopen_update_node_status(uint8_t node_id, uint8_t state);

/* 狀態報告器 */
void canopen_report_network_status(void);
void canopen_report_node_status(uint8_t node_id);
void canopen_report_message_statistics(void);
```

## 7. 實現計劃

### 7.1 開發階段
**階段 1: 基礎 CAN 通訊** (預估 2-3 小時)
- DAVE CAN 驅動初始化
- CAN 訊息接收處理
- UART 除錯輸出
- LED 狀態指示

**階段 2: CANopen 協議解析** (預估 3-4 小時)
- CAN ID 解碼實現
- 訊息類型分類
- 基本資料格式化
- 節點狀態追蹤

**階段 3: 監控功能實現** (預估 2-3 小時)
- 網路狀態監控
- 節點活動檢測
- 訊息統計分析
- 異常事件檢測

**階段 4: 優化與測試** (預估 2-3 小時)
- 效能優化
- 穩定性測試
- 功能驗證
- 文件完善

### 7.2 開發優先級
1. **高優先級**: CAN 接收、基本解析、UART 輸出
2. **中優先級**: 節點監控、狀態追蹤、LED 指示
3. **低優先級**: 統計分析、高級診斷、儲存功能

## 8. 測試策略

### 8.1 單元測試
- CAN 訊息解析正確性
- 節點狀態更新邏輯
- 資料格式化輸出

### 8.2 整合測試
- 與真實 CANopen 設備互動
- 多節點網路環境測試
- 長時間穩定性測試

### 8.3 效能測試
- 高負載 CAN 訊息處理
- 記憶體使用量分析
- 即時性能評估

## 9. 預期成果

### 9.1 核心功能
- ✅ 監控 CANopen 網路所有訊息
- ✅ 識別和追蹤網路節點
- ✅ 解析主要 CANopen 訊息類型
- ✅ 提供即時狀態報告
- ✅ 檢測網路異常事件

### 9.2 技術指標
- **訊息處理能力**: >1000 msgs/sec
- **節點支援數量**: 127 nodes (CANopen 標準)
- **記憶體使用**: <64KB Flash, <16KB RAM
- **即時響應**: <1ms 訊息處理延遲

## 10. 結論

**推薦採用自製輕量級監控程式方案**，理由：

1. **適用性**: 監控程式不需要完整 CANopen 堆疊
2. **簡潔性**: 程式碼清晰易懂，便於維護
3. **效率性**: 針對監控需求優化，效能更好
4. **可控性**: 完全掌控實現細節，便於客製化

這個方案將提供一個專業、高效、易維護的 CANopen 監控解決方案。