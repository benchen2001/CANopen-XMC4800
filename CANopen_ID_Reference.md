# CANopen CAN ID 標準對照表

## Node ID = 10 (0x0A) 的情況

### 基本規則
CANopen 標準 CAN ID = 基礎 ID + Node ID

### 詳細對照表

| 功能名稱 | 基礎 ID | Node ID | 完整 CAN ID | 十六進位 |
|----------|---------|---------|-------------|----------|
| NMT      | 0x000   | 不適用  | 0x000       | 0x000    |
| SYNC     | 0x080   | 不適用  | 0x080       | 0x080    |
| Emergency| 0x080   | 10      | 0x08A       | 0x08A    |
| TPDO1    | 0x180   | 10      | 0x18A       | 0x18A    |
| TPDO2    | 0x280   | 10      | 0x28A       | 0x28A    |
| TPDO3    | 0x380   | 10      | 0x38A       | 0x38A    |
| TPDO4    | 0x480   | 10      | 0x48A       | 0x48A    |
| RPDO1    | 0x200   | 10      | 0x20A       | 0x20A    |
| RPDO2    | 0x300   | 10      | 0x30A       | 0x30A    |
| RPDO3    | 0x400   | 10      | 0x40A       | 0x40A    |
| RPDO4    | 0x500   | 10      | 0x50A       | 0x50A    |
| SDO TX   | 0x580   | 10      | 0x58A       | 0x58A    |
| SDO RX   | 0x600   | 10      | 0x60A       | 0x60A    |
| Heartbeat| 0x700   | 10      | 0x70A       | 0x70A    |

### 測試訊息
| 測試用   | 固定    | 不適用  | 0x123       | 0x123    |

## 如果 CAN 分析儀顯示的 ID 不符合上表

### 可能的問題：

1. **Node ID 設定錯誤**
   - 檢查 `pendingNodeId` 變數是否為 10
   - 確認 CANopen 初始化是否成功

2. **CAN ID 計算錯誤**
   - Emergency 應該是 0x08A (不是其他值)
   - TPDO1 應該是 0x18A (不是其他值)
   - Heartbeat 應該是 0x70A (不是其他值)

3. **CAN 分析儀設定問題**
   - 確認分析儀使用標準 11-bit CAN ID
   - 檢查是否使用擴展 29-bit 格式 (應該用標準格式)

4. **程式實作問題**
   - 檢查 `CO_driver_XMC4800.c` 中的 ID 設定
   - 確認硬體暫存器操作是否正確

## 除錯步驟

1. **查看 UART 輸出**
   - 尋找 "CANopen CAN ID 對照表" 輸出
   - 確認 Node ID 顯示為 10
   - 檢查預期 ID vs 實際 ID

2. **驗證 CAN 分析儀設定**
   - 波特率: 250 kbps
   - 格式: 標準 11-bit ID
   - 終端電阻: 120Ω

3. **檢查常見錯誤**
   - ID=0x123 應該每 3 秒出現一次 (測試訊息)
   - ID=0x08A 應該有 Emergency 資料
   - ID=0x70A 應該每 500ms 出現一次 (Heartbeat)

## 預期的 CAN 流量模式

正常運行時應該看到：
- **0x123**: 每 3 秒 (測試訊息，資料: A1 A2 A3 A4 A5 A6 A7 A8)
- **0x08A**: 每 3 秒 (Emergency 測試)
- **0x18A**: 每 3 秒 (TPDO1 測試)
- **0x70A**: 每 500ms (Heartbeat，1 字節資料 = NMT 狀態)

如果看到其他 ID 值，請檢查程式實作或 CAN 分析儀設定。