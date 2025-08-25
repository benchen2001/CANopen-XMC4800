# XMC4800 CANopen 設備開發技術規劃
## 基於 CANopenNode v2.0 與現有專案架構

---

## 📊 **現有專案分析**

### **當前專案結構**
```
C:\prj\AI\CANOpen\
├── CANopenNode/                    # ✅ 已下載 CANopenNode v2.0
│   ├── 301/                        # CiA 301 核心協議實現
│   │   ├── CO_driver.h             # 驅動抽象層接口
│   │   ├── CO_NMT_Heartbeat.*      # NMT & Heartbeat 功能
│   │   ├── CO_SDOserver.*          # SDO 伺服器實現
│   │   ├── CO_SDOclient.*          # SDO 客戶端實現
│   │   ├── CO_PDO.*                # PDO 處理
│   │   ├── CO_SYNC.*               # SYNC 功能
│   │   ├── CO_Emergency.*          # 緊急訊息處理
│   │   └── CO_ODinterface.*        # 物件字典接口
│   ├── 303/                        # LED 指示器規範
│   ├── 305/                        # LSS (Layer Setting Services)
│   ├── example/                    # 參考範例
│   │   ├── main_blank.c            # 主程式範本
│   │   ├── CO_driver_blank.c       # 驅動層範本
│   │   ├── CO_driver_target.h      # 目標平台定義
│   │   ├── OD.c/OD.h              # 物件字典定義
│   │   └── CO_storageBlank.*       # 儲存管理
│   ├── CANopen.h                   # 主要標頭檔
│   └── CANopen.c                   # 主要實現
├── Dave/XMC4800_CANopen/          # ✅ 現有監控程式 (607行)
│   ├── main.c                      # Wireshark 整合監控工具
│   └── Debug/                      # 編譯輸出
└── 相關工具檔案/                   # Python 分析工具、腳本等
```

### **現有技術資產**
- ✅ **DAVE IDE 專案框架**：CAN_NODE, SYSTIMER, DIGITAL_IO, UART 配置完成
- ✅ **XMC4800 CAN 驅動基礎**：CAN 初始化、中斷處理、訊息收發
- ✅ **Wireshark 整合**：UDP 封包格式、pcap 相容輸出
- ✅ **CANopenNode v2.0 完整源碼**：301/303/305 標準實現
- ✅ **測試工具鏈**：Python 監控工具、編譯腳本

---

## 🎯 **開發目標與策略**

### **從監控工具進化為 CANopen 設備**
**現狀**：專業級 CANopen 監控器 (607行程式碼)
**目標**：完整 CANopen 設備 (支援 SDO/PDO/NMT/SYNC)

### **技術路線**
1. **保留現有基礎架構** - DAVE 配置、CAN 驅動、除錯接口
2. **整合 CANopenNode** - 添加標準 CANopen 堆疊
3. **實現設備功能** - SDO 伺服器、PDO 傳輸、物件字典
4. **雙模式運行** - 監控模式 + 設備模式可切換

---

## 🏗️ **實施架構設計**

### **新專案結構規劃**
```
Dave/XMC4800_CANopen_Device/           # 新建 DAVE 專案
├── main.c                             # 主程式 (整合 CANopenNode)
├── CANopenNode/                       # 整合 CANopenNode 源碼
│   ├── 301/                           # 複製核心檔案
│   ├── CO_config.h                    # 堆疊配置
│   └── CANopen.h                      # 主要接口
├── port/                              # XMC4800 移植層
│   ├── CO_driver_XMC4800.c           # CAN 驅動實現
│   ├── CO_driver_target.h            # 平台定義
│   └── CO_storage_XMC4800.c          # 非揮發性儲存
├── application/                       # 應用層
│   ├── OD_XMC4800.c/h                # 物件字典定義
│   ├── app_canopen.c                 # CANopen 應用邏輯
│   └── app_objects.c                 # 應用物件管理
└── config/                           # 配置檔案
    ├── CO_config.h                   # CANopenNode 配置
    └── app_config.h                  # 應用程式配置
```

### **關鍵整合接口**

#### **CO_driver_XMC4800.c 實現要點**
```c
// 基於現有 main.c 的 CAN 驅動經驗
CO_ReturnError_t CO_CANmodule_init(
    CO_CANmodule_t *CANmodule,
    void *CANptr,                     // CAN_NODE_0
    CO_CANrx_t rxArray[],
    uint16_t rxSize,
    CO_CANtx_t txArray[],
    uint16_t txSize,
    uint16_t CANbitRate              // 500kbps
);

void CO_CANinterrupt(CO_CANmodule_t *CANmodule);
CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer);
```

#### **物件字典設計 (基於現有監控功能)**
```c
// 0x1000: Device Type - CANopen Device + Monitor
#define OD_DEVICE_TYPE    0x00000000UL  // Generic I/O device

// 0x1018: Identity Object
typedef struct {
    uint8_t  numberOfEntries;
    uint32_t vendorID;        // 自定義廠商 ID  
    uint32_t productCode;     // XMC4800 CANopen Device
    uint32_t revisionNumber;  // 版本資訊
    uint32_t serialNumber;    // 序號
} OD_identity_t;

// 0x6000-0x6FFF: 數位輸入 (LED 狀態)
// 0x7000-0x7FFF: 數位輸出 (LED 控制)
// 0x2000-0x2FFF: 監控功能控制物件
```

---

## 📋 **實施步驟**

### **Phase 1: 基礎整合 (1-2天)**

#### **1.1 建立新 DAVE 專案**
- [ ] 複製現有 `XMC4800_CANopen` 為 `XMC4800_CANopen_Device`
- [ ] 保留所有 DAVE APP 配置 (CAN_NODE, SYSTIMER, DIGITAL_IO, UART)
- [ ] 確認編譯環境正常

#### **1.2 整合 CANopenNode 源碼**
```powershell
# 複製核心檔案到新專案
Copy-Item "C:\prj\AI\CANOpen\CANopenNode\301\*" "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen_Device\CANopenNode\301\"
Copy-Item "C:\prj\AI\CANOpen\CANopenNode\CANopen.*" "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen_Device\CANopenNode\"
Copy-Item "C:\prj\AI\CANOpen\CANopenNode\example\OD.*" "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen_Device\application\"
```

#### **1.3 建立編譯框架**
- [ ] 修改 Makefile，包含 CANopenNode 源碼
- [ ] 建立基礎 CO_config.h
- [ ] 確認無編譯錯誤

### **Phase 2: 驅動層移植 (2-3天)**

#### **2.1 實現 CO_driver_XMC4800.c**
**基於現有 main.c 的 CAN 處理經驗**：
```c
// 現有程式碼的 CAN 處理邏輯
void CAN_NODE_0_LMO_01_IRQHandler(void) {
    // 已實現：CAN 接收中斷處理
    // 已實現：訊息解析和佇列管理
    // 需要：適配到 CO_CANinterrupt()
}

void USB_Transmit_Process(void) {
    // 已實現：UART/USB 傳輸邏輯
    // 已實現：Wireshark 封包格式
    // 需要：適配到 CANopen 格式
}
```

#### **2.2 時間管理整合**
```c
// 利用現有 SYSTIMER 實現
uint32_t CO_timer1ms = 0;
#define CO_timer1ms_getElapsed() (SYSTIMER_GetTime() / 1000)
```

### **Phase 3: CANopen 功能實現 (3-4天)**

#### **3.1 基礎 NMT & Heartbeat**
```c
// 參考 example/main_blank.c 實現
CO_t* CO = NULL;
uint8_t activeNodeId = 10;      // 可配置節點 ID
uint16_t pendingBitRate = 500;  // 500kbps CAN 速率

// 整合現有 LED 控制邏輯
extern DIGITAL_IO_t LED1, LED2;  // 來自現有專案
```

#### **3.2 SDO 伺服器實現**
- [ ] 基於 CO_SDOserver.* 實現標準 SDO 服務
- [ ] 物件字典存取接口
- [ ] 支援 Expedited 和 Segmented 傳輸

#### **3.3 PDO 通訊**
```c
// 輸入 PDO: 接收來自網路的控制指令
// 輸出 PDO: 發送設備狀態和監控資料
typedef struct {
    uint16_t led_status;      // LED 狀態
    uint32_t message_count;   // 監控訊息計數  
    uint16_t can_bus_load;    // CAN 總線負載
    uint16_t error_count;     // 錯誤計數
} device_status_pdo_t;
```

### **Phase 4: 雙模式整合 (1-2天)**

#### **4.1 模式切換機制**
```c
typedef enum {
    MODE_CANOPEN_DEVICE,    // CANopen 設備模式
    MODE_MONITOR,           // 監控模式 (原有功能)
    MODE_HYBRID            // 混合模式
} operation_mode_t;

// 通過 SDO 或 DIP 開關切換模式
```

#### **4.2 功能整合**
- [ ] 保留現有 Wireshark 整合功能
- [ ] CANopen 設備功能與監控並存
- [ ] 統一的除錯和診斷接口

---

## 🔧 **技術細節**

### **CO_config.h 關鍵配置**
```c
#define CO_CONFIG_NMT           (CO_CONFIG_NMT_ENABLE)
#define CO_CONFIG_SDO_SRV       (CO_CONFIG_SDO_SRV_ENABLE)
#define CO_CONFIG_PDO           (CO_CONFIG_PDO_ENABLE)
#define CO_CONFIG_SYNC          (CO_CONFIG_SYNC_ENABLE)
#define CO_CONFIG_EMERGENCY     (CO_CONFIG_EMERGENCY_ENABLE)

// 基於 XMC4800 資源配置
#define CO_CONFIG_SDO_SRV_BUFFER_SIZE   1000
#define CO_CONFIG_RXFIFO_SIZE           16
#define CO_CONFIG_TXFIFO_SIZE           16
```

### **記憶體使用評估**
```c
// CANopenNode 堆疊記憶體需求
// 基於 example/main_blank.c 的實際測量
uint32_t heapMemoryUsed = ~8-12KB;  // 實際值由 CO_new() 返回

// XMC4800 資源
// Flash: 2MB (目前使用 ~30KB, 剩餘充足)
// RAM: 368KB (目前使用 ~12KB, 剩餘充足)
```

---

## ✅ **驗收標準**

### **功能驗收**
- [ ] 標準 CANopen 協議一致性 (CiA 301)
- [ ] SDO 讀寫操作正常 (< 10ms 回應時間)
- [ ] PDO 傳輸穩定 (支援事件驅動和週期性)
- [ ] NMT 狀態機正確運作
- [ ] 保留原有監控功能
- [ ] Wireshark 分析相容性

### **品質驗收**  
- [ ] 編譯無警告
- [ ] 記憶體使用 < 64KB (Flash + RAM)
- [ ] CAN 總線負載 < 30%
- [ ] 與現有 Python 監控工具相容

### **測試驗收**
- [ ] 與標準 CANopen 工具通訊測試
- [ ] 長時間穩定性測試 (24小時)
- [ ] 錯誤處理和恢復測試

---

## 🚀 **立即行動計劃**

### **第一步：建立新專案 (今天)**
```powershell
# 在 DAVE IDE 中：
# 1. File -> Import -> Existing Projects
# 2. 複製 XMC4800_CANopen 為 XMC4800_CANopen_Device
# 3. 清理舊的 main.c，準備整合 CANopenNode
```

### **第二步：整合 CANopenNode (明天)**
```c
// 新的 main.c 框架
#include "DAVE.h"           // 保留現有 DAVE 配置
#include "CANopen.h"        // 添加 CANopenNode
#include "OD.h"             // 物件字典

// 整合現有的優秀基礎架構
extern DIGITAL_IO_t LED1, LED2;  // LED 控制
extern SYSTIMER_t SYSTIMER_0;    // 時間管理  
extern CAN_NODE_t CAN_NODE_0;    // CAN 通訊
extern UART_t UART_0;            // 除錯輸出
```

**您準備好開始第一步了嗎？我可以立即協助您建立新的 DAVE 專案並開始整合工作。**