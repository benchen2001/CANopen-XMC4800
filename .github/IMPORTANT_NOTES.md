# 重要開發注意事項

## 🚨 關鍵提醒

### 開發環境約束
- **必須使用 DAVE APP**: 所有硬體配置透過 DAVE 進行
- **必須使用 UART 輸出**: 所有狀態和除錯訊息透過 UART_0
- **固定工具鏈**: DAVE IDE 4.5.0 + ARM-GCC-49
- **專業工程師標準**: 認真研讀程式碼，不簡化工作和程式碼

### 嚴格禁止
- ❌ 不要亂加 .c 檔案
- ❌ 不要亂加 batch 檔案  
- ❌ 不要亂建 J-Link 命令檔案
- ❌ 不要簡化工作流程
- ❌ 不要簡化程式碼實現

### DAVE API 限制
- `CAN_NODE_MO_UpdateID()` 函數在 DAVE 4.5.0 中不存在
- 必須使用直接硬體暫存器操作來設定 CAN ID
- 數據處理仍可使用 `CAN_NODE_MO_UpdateData()`

### 中斷處理整合
- CANopen 中斷處理函數需要手動整合到 DAVE 中斷處理器中
- `CO_timer1ms` 必須在 SYSTIMER 中斷中更新
- CAN 接收中斷需要呼叫 `CO_CANinterrupt_Rx()`

### 記憶體和效能
- UART 除錯緩衝區限制 256 字節
- 避免在中斷中進行長時間處理
- 簡化除錯輸出格式以防緩衝區溢位

## 🔧 快速修復參考

### CO_CANsend() 核心實現
```c
// 混合式實現：硬體暫存器 + DAVE API
mo->can_mo_ptr->MOAR = (buffer->ident & 0x7FF) << CAN_MO_MOAR_ID_Pos;
mo->can_mo_ptr->MOFCR = (buffer->DLC & 0x0F) << CAN_MO_MOFCR_DLC_Pos;
CAN_NODE_MO_UpdateData(lmo, buffer->data);
return CAN_NODE_MO_Transmit(lmo);
```

### 計時器整合
```c
volatile uint32_t CO_timer1ms = 0;

void SYSTIMER_Callback(void) {
    CO_timer1ms++;
}
```

### 除錯輸出格式
```c
Debug_Printf("TX: ID=0x%03X DLC=%d\r\n", buffer->ident, buffer->DLC);
Debug_Printf("ERROR: %s failed: %d\r\n", function_name, error_code);
```

## 📋 編譯和燒錄
```powershell
# 編譯
cd "C:\prj\AI\CANOpen\Dave\XMC4800_CANopen\Debug"
& "C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\make.exe" all

# 燒錄
& "C:\Program Files\SEGGER\JLink\JLink.exe" -CommandFile "C:\prj\AI\CANOpen\flash_canopen.jlink"
```

## 🎯 CANopen 測試 ID
- 測試訊息: 0x123
- Emergency: 0x080 + Node ID  
- TPDO: 0x180 + Node ID
- Heartbeat: 0x700 + Node ID