# CANopen XMC4800 完整工作指南

## 📋 **專案概述**

**目標**: 在 XMC4800 微控制器上實現穩定的 CANopen 通訊，支援兩種方式：
1. **MULTICAN_CONFIG** (低階控制)
2. **CAN_NODE APP** (推薦：高階簡化)

**硬體**: XMC4800-F144x2048, DAVE IDE 4.5.0, CANopenNode v2.0
**CAN 速率**: 250 kbps, Node ID: 10

---

## 🔧 **第一部分：兩種 DAVE APP 配置方式**

### **方式一：MULTICAN_CONFIG (原有方式)**

適用於需要精細控制硬體的高級用戶。

#### **1. MULTICAN_CONFIG 設定**

#### **Node Settings**
```
Bit Rate: 250000 bps
Sample Point: 87.5%
Number of Message Objects: 4
Synchronization Jump Width: 1
```

#### **Message Objects 配置**

**LMO_00 (發送 - 基本測試)**
```
Message Type: Tx
Identifier Value: 0x123
Identifier Type: Standard (11-bit)
Acceptance Mask: Matching_ID (0x7FF)
Mask Value: 0x7FF
DLC: 8
Data_H: 0x0
Data_L: 0x0
Transmit Interrupt: ✓ Enable
Receive Interrupt: ✗ Disable
```

**⭐ LMO_03 (接收 - 關鍵配置)**
```
Message Type: Rx
Identifier Value: 0x60A  (0x600 + Node ID 10) ← 重點！
Identifier Type: Standard (11-bit)
Acceptance Mask: Matching_ID (0x7FF)  ← 精確匹配
Mask Value: 0x7FF  ← 避免幽靈數據
DLC: 8
Transmit Interrupt: ✗ Disable
Receive Interrupt: ✓ Enable  ← 必須啟用
```

---

### **方式二：CAN_NODE APP (新增推薦方式)**

適用於快速開發和一般應用，提供更簡化的 API。

#### **為什麼選擇 CAN_NODE APP？**
- ✅ **自動初始化**: DAVE_Init() 自動完成所有硬體配置
- ✅ **簡化 API**: 使用 CAN_NODE_MO_xxx() 函數
- ✅ **內建錯誤處理**: 自動處理常見錯誤
- ✅ **GPIO 自動配置**: 自動設定 TX/RX 引腳
- ✅ **中斷管理**: 簡化事件處理

#### **2. CAN_NODE APP 設定 (推薦)**

**步驟 1: 新增 CAN_NODE APP**
1. 在 DAVE 中右鍵點擊專案
2. 選擇 "Add New APP"
3. 搜尋並選擇 "CAN_NODE"
4. 按照下列參數配置

**CAN_NODE 基本設定**
```
APP Name: CAN_NODE_0
CAN Node: CAN_NODE1 (選擇硬體節點)
Bit Rate: 250000 bps
Sample Point: 87.5%
Number of Message Objects: 4
Auto Initialization: ✓ Enable
Loop Back Mode: ✗ Disable  ← 重要：避免自回收
```

**Message Objects 配置 (CAN_NODE APP)**

**LMO_01 (發送 - 基本測試)**
```
Logical MO: LMO_01
Message Type: Tx
Identifier Type(IDE): Std_11bit
Identifier Value: 0x123
Acceptance Mask: Matching_ID
Mask Value: 0x7FF
DLC: 8
Tx Event: ✓ Checked
Rx Event: ✗ Unchecked
```

**⭐ LMO_04 (接收 - 關鍵配置)**
```
Logical MO: LMO_04
Message Type: Rx ← 重點！
Identifier Type(IDE): Std_11bit
Identifier Value: 0x60A
Acceptance Mask: Matching_ID
Mask Value: 0x7FF
DLC: 8
Tx Event: ✗ Unchecked
Rx Event: ✓ Checked ← 必須啟用！
```

#### **Node Settings**
```
Bit Rate: 250000 bps
Sample Point: 87.5%
Number of Message Objects: 4
Synchronization Jump Width: 1
```

#### **Message Objects 配置**

**LMO_00 (發送 - 基本測試)**
```
Message Type: Tx
Identifier Value: 0x123
Identifier Type: Standard (11-bit)
Acceptance Mask: Matching_ID (0x7FF)
Mask Value: 0x7FF
DLC: 8
Data_H: 0x0
Data_L: 0x0
Transmit Interrupt: ✓ Enable
Receive Interrupt: ✗ Disable
```

**LMO_01 (發送 - CANopen Emergency)**
```
Message Type: Tx
Identifier Value: 0x08A  (0x080 + Node ID 10)
Identifier Type: Standard (11-bit)
Acceptance Mask: Matching_ID (0x7FF)
Mask Value: 0x7FF
DLC: 8
Transmit Interrupt: ✓ Enable
Receive Interrupt: ✗ Disable
```

**LMO_02 (發送 - CANopen TPDO)**
```
Message Type: Tx
Identifier Value: 0x18A  (0x180 + Node ID 10)
Identifier Type: Standard (11-bit)
Acceptance Mask: Matching_ID (0x7FF)
Mask Value: 0x7FF
DLC: 8
Transmit Interrupt: ✓ Enable
Receive Interrupt: ✗ Disable
```

**⭐ LMO_03 (接收 - 關鍵配置)**
```
Message Type: Rx
Identifier Value: 0x60A  (0x600 + Node ID 10) ← 重點！
Identifier Type: Standard (11-bit)
Acceptance Mask: Matching_ID (0x7FF)  ← 精確匹配
Mask Value: 0x7FF  ← 避免幽靈數據
DLC: 8
Transmit Interrupt: ✗ Disable
Receive Interrupt: ✓ Enable  ← 必須啟用
```

### **3. 兩種方式比較**

| 特性 | MULTICAN_CONFIG | CAN_NODE APP |
|------|-----------------|--------------|
| **抽象層級** | 低階硬體控制 | 高階應用接口 |
| **學習曲線** | 陡峭 (需了解硬體) | 平緩 (API 友善) |
| **初始化複雜度** | 需手動配置多項參數 | 自動初始化 |
| **API 複雜度** | 直接操作 XMC_CAN_MO_xxx | 簡化的 CAN_NODE_MO_xxx |
| **GPIO 配置** | 需要額外配置 | 自動處理 |
| **錯誤處理** | 需要自行實現 | 內建錯誤處理 |
| **靈活性** | 非常高 (完全控制) | 中等 (預設行為) |
| **適用場景** | 特殊需求、效能調優 | 一般應用、快速開發 |
| **推薦對象** | 資深嵌入式工程師 | 一般開發者 |

### **4. 建議選擇**

**🎯 對於大多數 CANopen 專案，建議使用 CAN_NODE APP**

**原因**:
1. **開發速度快**: 自動初始化，減少配置錯誤
2. **維護性好**: API 簡潔，程式碼可讀性高  
3. **錯誤率低**: 內建驗證和錯誤處理
4. **文檔完整**: DAVE 提供完整的 API 文檔

**何時選擇 MULTICAN_CONFIG**:
- 需要精細控制硬體暫存器
- 對效能有極高要求
- 需要實現特殊的 CAN 功能
- 已有基於 MULTICAN_CONFIG 的穩定程式碼

---

## 🎯 **第二部分：canAnalyser3Mini 測試配置**

### **1. 基本連接設定**
```
CAN Speed: 250 kbps
Termination: 120Ω (如果是總線末端)
Connection: CAN_H, CAN_L, GND
```

### **2. 標準測試序列**

#### **Phase 1: 發送測試 (canAnalyser3Mini → XMC4800)**
發送以下訊息測試 XMC4800 接收：
```
測試 1 - SDO 讀取請求:
ID: 0x60A
Data: 40 00 10 00 00 00 00 00
DLC: 8
期望結果: XMC4800 UART 顯示接收成功

測試 2 - NMT 命令:
ID: 0x000
Data: 01 0A 00 00 00 00 00 00
DLC: 2
期望結果: XMC4800 進入 Operational 狀態
```

#### **Phase 2: 接收測試 (XMC4800 → canAnalyser3Mini)**
監控以下 XMC4800 發送的訊息：
```
基本測試訊息:
ID: 0x123
Data: AA BB CC DD EE FF 12 34 或其他測試數據

Emergency 訊息:
ID: 0x08A (0x080 + Node ID 10)
Data: 錯誤代碼 + 錯誤暫存器

Heartbeat:
ID: 0x70A (0x700 + Node ID 10)
Data: NMT 狀態
```

#### **Phase 3: 雙向通訊驗證**
```
1. canAnalyser3Mini 發送 SDO 請求 (ID: 0x60A)
2. XMC4800 應該回覆 SDO 響應 (ID: 0x58A)
3. 確認數據正確性和時序
```

---

## 💻 **第三部分：程式碼實現 (支援兩種方式)**

### **選擇 1: MULTICAN_CONFIG 程式碼**

#### **CO_CANrxBufferInit (MULTICAN_CONFIG 版本)**
```c
CO_ReturnError_t CO_CANrxBufferInit(...)
{
    /* 使用 MULTICAN_CONFIG */
    if (MULTICAN_CONFIG_0.lmobj_ptr[3] != NULL) {
        XMC_CAN_MO_t *rx_mo = MULTICAN_CONFIG_0.lmobj_ptr[3]->mo_ptr;
        
        /* 固定配置 */
        rx_mo->can_identifier = 0x60A;    // SDO 接收
        rx_mo->can_id_mask = 0x7FF;       // 精確匹配
        rx_mo->can_ide_mask = 1U;         // 標準幀
        
        XMC_CAN_MO_Config(rx_mo);
        XMC_CAN_MO_EnableEvent(rx_mo, XMC_CAN_MO_EVENT_RECEIVE);
        
        Debug_Printf("✅ MULTICAN_CONFIG LMO_03: RX ID=0x60A\r\n");
    }
    return CO_ERROR_NO;
}
```

### **選擇 2: CAN_NODE APP 程式碼 (推薦)**

#### **CO_CANrxBufferInit (CAN_NODE APP 版本)**
```c
CO_ReturnError_t CO_CANrxBufferInit(...)
{
    /* 使用 CAN_NODE APP */
    if (CAN_NODE_0.lmobj_ptr[3] != NULL) {  // LMO_04 = index 3
        CAN_NODE_LMO_t *rx_lmo = CAN_NODE_0.lmobj_ptr[3];
        
        /* 動態配置接收 ID */
        rx_lmo->mo_ptr->can_identifier = 0x60A;    // SDO 接收
        rx_lmo->mo_ptr->can_id_mask = 0x7FF;       // 精確匹配
        
        /* 使用 CAN_NODE API 重新配置 */
        CAN_NODE_MO_Init(rx_lmo);
        CAN_NODE_MO_EnableRxEvent(rx_lmo);
        
        Debug_Printf("✅ CAN_NODE APP LMO_04: RX ID=0x60A\r\n");
    }
    return CO_ERROR_NO;
}
```

#### **CO_CANsend (CAN_NODE APP 版本)**
```c
CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
    uint8_t lmo_index;
    
    /* 智能選擇 LMO */
    if (buffer->ident == 0x123) {
        lmo_index = 0;  // LMO_01 基本測試
    } else if ((buffer->ident & 0x780) == 0x080) {
        lmo_index = 1;  // LMO_02 Emergency
    } else if ((buffer->ident & 0x780) == 0x180) {
        lmo_index = 2;  // LMO_03 TPDO
    } else if ((buffer->ident & 0x780) == 0x580) {
        lmo_index = 1;  // LMO_02 SDO Response
    } else {
        lmo_index = 0;  // 預設 LMO_01
    }
    
    /* 使用 CAN_NODE APP API */
    if (lmo_index < 3 && CAN_NODE_0.lmobj_ptr[lmo_index] != NULL) {
        CAN_NODE_LMO_t *tx_lmo = CAN_NODE_0.lmobj_ptr[lmo_index];
        
        /* 更新發送配置 */
        tx_lmo->mo_ptr->can_identifier = buffer->ident;
        tx_lmo->mo_ptr->can_data_length = buffer->DLC;
        
        /* 使用 CAN_NODE API 發送 */
        CAN_NODE_STATUS_t status = CAN_NODE_MO_UpdateData(tx_lmo, buffer->data);
        if (status == CAN_NODE_STATUS_SUCCESS) {
            status = CAN_NODE_MO_Transmit(tx_lmo);
            if (status == CAN_NODE_STATUS_SUCCESS) {
                Debug_Printf("TX: ID=0x%03X via LMO_%02d\r\n", buffer->ident, lmo_index + 1);
                return CO_ERROR_NO;
            }
        }
    }
    
    return CO_ERROR_TX_OVERFLOW;
}
```

#### **中斷處理 (CAN_NODE APP 版本)**
```c
// 使用 CAN_NODE APP 的中斷處理
void CAN0_0_IRQHandler(void) 
{
    /* 檢查接收事件 */
    if (CAN_NODE_0.lmobj_ptr[3] != NULL) {  // LMO_04
        CAN_NODE_LMO_t *rx_lmo = CAN_NODE_0.lmobj_ptr[3];
        uint32_t status = CAN_NODE_MO_GetStatus(rx_lmo);
        
        if (status & XMC_CAN_MO_STATUS_RX_PENDING) {
            /* 接收數據 */
            CAN_NODE_STATUS_t rx_status = CAN_NODE_MO_ReceiveData(rx_lmo);
            if (rx_status == CAN_NODE_STATUS_SUCCESS) {
                Debug_Printf("RX: ID=0x%03X, DLC=%d\r\n", 
                           rx_lmo->mo_ptr->can_identifier,
                           rx_lmo->mo_ptr->can_data_length);
                
                /* 呼叫 CANopen 處理函數 */
                CO_CANinterrupt_Rx(&CO_CANmodule[0], 3);
            }
            
            /* 清除中斷標誌 */
            CAN_NODE_MO_ClearStatus(rx_lmo, XMC_CAN_MO_RESET_STATUS_RX_PENDING);
        }
    }
}
```

#### **CO_CANsend (發送邏輯)**
```c
CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
    uint8_t lmo_index;
    
    /* 根據 CANopen ID 範圍智能選擇 LMO */
    if (buffer->ident == 0x123) {
        lmo_index = 0;  // 基本測試
    } else if ((buffer->ident & 0x780) == 0x080) {
        lmo_index = 1;  // Emergency (0x080-0x0FF)
    } else if ((buffer->ident & 0x780) == 0x180) {
        lmo_index = 2;  // TPDO (0x180-0x1FF)
    } else if ((buffer->ident & 0x780) == 0x580) {
        lmo_index = 1;  // SDO Response (0x580-0x5FF)
    } else if ((buffer->ident & 0x780) == 0x700) {
        lmo_index = 0;  // Heartbeat (0x700-0x77F)
    } else {
        lmo_index = 0;  // 預設使用 LMO_00
    }
    
    /* 使用 MULTICAN_CONFIG LMO 發送 */
    if (lmo_index < 3 && MULTICAN_CONFIG_0.lmobj_ptr[lmo_index] != NULL) {
        MULTICAN_CONFIG_NODE_LMO_t *config_lmo = MULTICAN_CONFIG_0.lmobj_ptr[lmo_index];
        CAN_NODE_LMO_t *lmo = (CAN_NODE_LMO_t*)config_lmo;
        
        /* 更新 MO 配置 */
        config_lmo->mo_ptr->can_identifier = buffer->ident;
        config_lmo->mo_ptr->can_data_length = buffer->DLC;
        
        /* 使用純 DAVE API */
        CAN_NODE_STATUS_t status = CAN_NODE_MO_UpdateData(lmo, buffer->data);
        if (status == CAN_NODE_STATUS_SUCCESS) {
            status = CAN_NODE_MO_Transmit(lmo);
            if (status == CAN_NODE_STATUS_SUCCESS) {
                Debug_Printf("TX OK: ID=0x%03X via LMO_%02d\r\n", buffer->ident, lmo_index);
                return CO_ERROR_NO;
            }
        }
    }
    
    return CO_ERROR_TX_OVERFLOW;
}
```

#### **CO_CANinterrupt_Rx (接收處理)**
```c
static void CO_CANinterrupt_Rx(CO_CANmodule_t *CANmodule, uint32_t index)
{
    if (MULTICAN_CONFIG_0.lmobj_ptr[3] != NULL) {
        MULTICAN_CONFIG_NODE_LMO_t *rx_config = MULTICAN_CONFIG_0.lmobj_ptr[3];
        CAN_NODE_LMO_t *lmo = (CAN_NODE_LMO_t*)rx_config;
        
        CAN_NODE_STATUS_t status = CAN_NODE_MO_ReceiveData(lmo);
        if (status == CAN_NODE_STATUS_SUCCESS) {
            XMC_CAN_MO_t *mo = rx_config->mo_ptr;
            
            /* 讀取接收數據 */
            rcvMsg.ident = mo->can_identifier & 0x07FFU;
            rcvMsg.DLC = mo->can_data_length;
            
            /* 嚴格的幽靈數據過濾 */
            if (rcvMsg.ident == 0x000 || 
                (rcvMsg.ident == 0x123 && 全零數據) ||
                rcvMsg.ident > 0x7FF) {
                Debug_Printf("🚨 幽靈數據過濾: ID=0x%03X\r\n", rcvMsg.ident);
                CAN_NODE_MO_ClearStatus(lmo, XMC_CAN_MO_RESET_STATUS_RX_PENDING);
                return;
            }
            
            /* 正常處理邏輯 */
            Debug_Printf("*** 接收成功: ID=0x%03X, DLC=%d ***\r\n", rcvMsg.ident, rcvMsg.DLC);
            /* 執行 CANopen 回調處理 */
        }
    }
}
```

---

## 📝 **第四部分：工作流程檢查清單**

### **方式選擇**
- [ ] **選擇方式一**: MULTICAN_CONFIG (適合高級用戶)
- [ ] **選擇方式二**: CAN_NODE APP (推薦給一般用戶)

### **階段 1: DAVE 配置檢查**

#### **如果選擇 MULTICAN_CONFIG:**
- [ ] MULTICAN_CONFIG LMO_03 設為 Rx, ID=0x60A, Mask=0x7FF
- [ ] LMO_03 Receive Interrupt = Enable
- [ ] LMO_00-02 設為 Tx, 使用不同 ID
- [ ] Generate Code 重新生成

#### **如果選擇 CAN_NODE APP:**
- [ ] 新增 CAN_NODE APP (APP Name: CAN_NODE_0)
- [ ] LMO_04 設為 Rx, ID=0x60A, Rx Event = Checked
- [ ] LMO_01-03 設為 Tx, Tx Event = Checked
- [ ] Auto Initialization = Enable
- [ ] Loop Back Mode = Disable
- [ ] Generate Code 重新生成

### **階段 2: 程式碼檢查** ✅
- [ ] 選擇對應的程式碼版本 (MULTICAN_CONFIG 或 CAN_NODE)
- [ ] CO_CANrxBufferInit 正確設定 RX ID=0x60A
- [ ] CO_CANsend 使用正確的 LMO 路由邏輯
- [ ] 中斷處理函數正確實現
- [ ] Debug_Printf UART 輸出正常

### **階段 3: 編譯與燒錄** ✅
- [ ] Clean & Build 成功，無編譯錯誤
- [ ] J-Link 燒錄成功
- [ ] UART 顯示初始化訊息

### **階段 4: canAnalyser3Mini 測試** ✅
- [ ] 設定 250 kbps，連接 CAN_H, CAN_L
- [ ] 發送 ID=0x60A 測試訊息
- [ ] 確認 XMC4800 UART 顯示接收成功
- [ ] 監控 XMC4800 發送的 ID=0x123 訊息

### **階段 5: CANopen 功能驗證** ✅
- [ ] SDO 通訊 (0x60A → 0x58A)
- [ ] Emergency 訊息 (0x08A)
- [ ] Heartbeat (0x70A)
- [ ] NMT 狀態控制

---

## 🎯 **第五部分：預期結果與故障排除**

### **成功指標**
```
UART 輸出應顯示:
=== 完全依賴 MULTICAN_CONFIG + DAVE API ===
✅ LMO_03 固定配置: RX ID=0x60A, Mask=0x7FF (CANopen SDO)
✅ 避免自回收: TX(0x123) ≠ RX(0x60A)
*** MULTICAN_CONFIG 接收成功 ***
ID=0x60A, DLC=8
Data: 40 00 10 00 00 00 00 00
==> 呼叫 CANopen 處理函數
```

### **故障排除**

#### **問題 1: 持續收到幽靈數據 (ID=0x000)**
**原因**: RX Mask 設定錯誤或自回收
**解決**: 
1. 確認 DAVE 中 LMO_03 Mask = 0x7FF
2. 確認 can_identifier = 0x60A (不是 0x123)
3. 重新 Generate Code

#### **問題 2: canAnalyser3Mini 發送訊息未被接收**
**原因**: ID 不匹配或接收中斷未啟用
**解決**:
1. 確認發送 ID = 0x60A
2. 檢查 DAVE 中 LMO_03 Receive Interrupt = Enable
3. 檢查程式中 XMC_CAN_MO_EnableEvent 是否被呼叫

#### **問題 3: 發送功能異常**
**原因**: LMO 選擇邏輯錯誤
**解決**:
1. 檢查 CO_CANsend 中的 lmo_index 計算
2. 確認 MULTICAN_CONFIG_0.lmobj_ptr[0-2] 不為 NULL
3. 驗證 CAN_NODE_MO_UpdateData 返回值

#### **問題 4: UART 無輸出**
**原因**: UART 初始化失敗或緩衝區問題
**解決**:
1. 檢查 UART_0 配置 (115200 baud)
2. 確認 Debug_Printf 函數實現
3. 檢查 UART 連接和終端設定

---

## ⚠️ **關鍵原則與禁令**

### **✅ 必須遵守**
1. **TX 和 RX 使用不同 ID** (TX: 0x123, RX: 0x60A)
2. **RX Mask 精確匹配** (0x7FF)
3. **只使用 DAVE API，不直接操作暫存器**
4. **嚴格過濾幽靈數據**
5. **按照 CANopen ID 範圍路由發送**

### **❌ 絕對禁止**
1. **不要設定 RX ID = TX ID** (避免自回收)
2. **不要使用 Mask = 0x000** (會接收所有 ID)
3. **不要隨意修改 MULTICAN_CONFIG 生成的程式碼**
4. **不要添加主動輪詢接收邏輯**
5. **不要忽略編譯警告**

---

## 📚 **附錄：CANopen ID 分配表**

| 功能 | ID 範圍 | Node 10 ID | 說明 |
|------|---------|------------|------|
| NMT | 0x000 | 0x000 | 網路管理 |
| Emergency | 0x080-0x0FF | 0x08A | 緊急訊息 |
| TPDO1 | 0x180-0x1FF | 0x18A | 發送 PDO |
| RPDO1 | 0x200-0x27F | 0x20A | 接收 PDO |
| SDO TX | 0x580-0x5FF | 0x58A | SDO 響應 |
| SDO RX | 0x600-0x67F | 0x60A | SDO 請求 |
| Heartbeat | 0x700-0x77F | 0x70A | 心跳訊息 |

---

## 🔄 **版本記錄**

**v1.0** (2025-08-24)
- 初始版本，包含完整 DAVE 配置和程式碼實現
- 解決自回收和幽靈數據問題
- 建立標準測試流程

**使用說明**: 
- 此文檔為完整工作指南，請嚴格按照步驟執行
- 遇到問題時，請參考故障排除章節
- 修改前請先備份程式碼
- 有任何疑問請參考此文檔相關章節

---

**© 2025 XMC4800 CANopen Team**