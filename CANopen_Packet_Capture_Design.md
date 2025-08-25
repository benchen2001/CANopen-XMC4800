# CANopen 封包捕獲與分析系統設計方案

## 1. 系統架構概覽

```
XMC4800 Hardware
      ↓
CANopen Monitor Firmware
      ↓
Interface Layer (USB/Ethernet)
      ↓
PC Analysis Software
      ↓
Wireshark Integration
```

## 2. 推薦的實現方案

### 方案 A：USB CDC (虛擬串口) - **推薦**
**優點：**
- ✅ 實現簡單，DAVE 有現成的 USB 庫
- ✅ 跨平台支援 (Windows/Linux/macOS)
- ✅ 即插即用，無需額外驅動
- ✅ 高速傳輸 (USB 2.0 = 480 Mbps)
- ✅ 可直接整合 Wireshark

**缺點：**
- ❌ 需要 USB 連線
- ❌ 單點對點連接

### 方案 B：Ethernet UDP - **進階選項**
**優點：**
- ✅ 網路化，可遠端監控
- ✅ 多客戶端同時連接
- ✅ 整合現有網路基礎設施
- ✅ Wireshark 原生支援 UDP

**缺點：**
- ❌ 實現複雜度較高
- ❌ 需要額外的 Ethernet 模組

## 3. 封包格式設計

### 3.1 CANopen 訊息封裝格式
```c
typedef struct {
    uint32_t timestamp_us;    // 微秒時間戳
    uint32_t can_id;         // CAN ID (11-bit)
    uint8_t  dlc;            // 資料長度 (0-8)
    uint8_t  data[8];        // CAN 資料
    uint8_t  flags;          // 狀態標誌 (錯誤、遠程幀等)
    uint8_t  reserved;       // 保留位元組
} __attribute__((packed)) canopen_frame_t;
```

### 3.2 傳輸協議設計
```c
typedef struct {
    uint32_t magic;          // 0x43414E4F ("CANO")
    uint16_t version;        // 協議版本
    uint16_t frame_count;    // 本次傳輸的訊息數量
    canopen_frame_t frames[];// 訊息陣列
    uint32_t checksum;       // CRC32 校驗
} __attribute__((packed)) canopen_packet_t;
```

## 4. Wireshark 整合方案

### 4.1 Wireshark 解析器參考 (基於原始碼分析)
從 Wireshark 的 `packet-canopen.c` 可以看出支援的訊息類型：

```c
// 主要 CANopen 訊息類型
#define MT_NMT_CTRL        0x00  // NMT 控制
#define MT_SYNC            0x01  // 同步訊息
#define MT_TIME_STAMP      0x02  // 時間戳
#define MT_PDO             0x03  // 過程資料物件
#define MT_SDO             0x04  // 服務資料物件
#define MT_NMT_ERR_CTRL    0x0E  // NMT 錯誤控制
#define MT_LSS_MASTER      0x7E5 // LSS 主站
#define MT_LSS_SLAVE       0x7E4 // LSS 從站
```

### 4.2 Wireshark Lua 插件方案
建立自定義的 Wireshark 解析器：

```lua
-- CANopen XMC4800 Monitor Protocol
local xmc_canopen_proto = Proto("xmc_canopen", "XMC4800 CANopen Monitor")

-- 定義欄位
local f_magic = ProtoField.uint32("xmc_canopen.magic", "Magic", base.HEX)
local f_version = ProtoField.uint16("xmc_canopen.version", "Version", base.DEC)
local f_frame_count = ProtoField.uint16("xmc_canopen.frame_count", "Frame Count", base.DEC)
local f_timestamp = ProtoField.uint32("xmc_canopen.timestamp", "Timestamp", base.DEC)
local f_can_id = ProtoField.uint32("xmc_canopen.can_id", "CAN ID", base.HEX)
local f_dlc = ProtoField.uint8("xmc_canopen.dlc", "DLC", base.DEC)
local f_data = ProtoField.bytes("xmc_canopen.data", "Data")

xmc_canopen_proto.fields = {f_magic, f_version, f_frame_count, f_timestamp, f_can_id, f_dlc, f_data}
```

## 5. XMC4800 韌體實現

### 5.1 USB CDC 實現架構
```c
/* USB CDC 傳輸緩衝區 */
#define USB_TX_BUFFER_SIZE 1024
#define CAN_MSG_QUEUE_SIZE 256

typedef struct {
    canopen_frame_t buffer[CAN_MSG_QUEUE_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} can_msg_queue_t;

/* 全域變數 */
static can_msg_queue_t can_queue;
static uint8_t usb_tx_buffer[USB_TX_BUFFER_SIZE];
static uint32_t system_timestamp_us = 0;
```

### 5.2 CAN 訊息捕獲
```c
/* CAN 中斷處理函數 */
void CAN_RX_Interrupt_Handler(void)
{
    canopen_frame_t frame;
    CAN_NODE_STATUS_t status;
    
    // 讀取 CAN 訊息
    status = CAN_NODE_MO_ReceiveData(&CAN_NODE_0_LMO_02_Config, frame.data);
    
    if (status == CAN_NODE_STATUS_SUCCESS) {
        frame.timestamp_us = system_timestamp_us;
        frame.can_id = CAN_NODE_0_LMO_02_Config.mo_ptr->can_identifier;
        frame.dlc = CAN_NODE_0_LMO_02_Config.mo_ptr->can_data_length;
        frame.flags = 0; // 正常訊息
        
        // 加入佇列
        canopen_queue_push(&can_queue, &frame);
    }
}
```

### 5.3 USB 傳輸處理
```c
/* USB 傳輸處理 */
void USB_Transmit_Process(void)
{
    static uint32_t last_transmit = 0;
    uint32_t current_time = system_timestamp_us;
    
    // 每 10ms 或佇列滿時傳輸
    if ((current_time - last_transmit > 10000) || (can_queue.count > CAN_MSG_QUEUE_SIZE/2)) {
        canopen_packet_t* packet = (canopen_packet_t*)usb_tx_buffer;
        
        packet->magic = 0x43414E4F; // "CANO"
        packet->version = 1;
        packet->frame_count = can_queue.count;
        
        // 複製 CAN 訊息
        for (uint16_t i = 0; i < packet->frame_count; i++) {
            canopen_queue_pop(&can_queue, &packet->frames[i]);
        }
        
        // 計算校驗碼
        packet->checksum = calculate_crc32((uint8_t*)packet, 
                                         sizeof(canopen_packet_t) + 
                                         packet->frame_count * sizeof(canopen_frame_t));
        
        // USB 傳輸
        USB_CDC_Device_SendData(packet, sizeof(canopen_packet_t) + 
                               packet->frame_count * sizeof(canopen_frame_t) + 4);
        
        last_transmit = current_time;
    }
}
```

## 6. PC 端分析軟體

### 6.1 Python 分析工具
```python
import serial
import struct
import time
from scapy.all import *

class CANopenMonitor:
    def __init__(self, port='COM3', baudrate=115200):
        self.ser = serial.Serial(port, baudrate)
        self.pcap_writer = None
        
    def start_capture(self, filename=None):
        if filename:
            self.pcap_writer = PcapWriter(filename, linktype=DLT_CAN_SOCKETCAN)
            
        while True:
            packet = self.read_packet()
            if packet:
                self.process_packet(packet)
                
    def read_packet(self):
        # 讀取封包標頭
        header = self.ser.read(8)
        if len(header) < 8:
            return None
            
        magic, version, frame_count = struct.unpack('<LHH', header)
        if magic != 0x43414E4F:  # "CANO"
            return None
            
        # 讀取 CAN 訊息
        frames = []
        for _ in range(frame_count):
            frame_data = self.ser.read(16)  # sizeof(canopen_frame_t)
            frame = struct.unpack('<LLBB8BB', frame_data)
            frames.append(frame)
            
        return frames
        
    def process_packet(self, frames):
        for frame in frames:
            timestamp, can_id, dlc, data, flags = frame[:5]
            
            # 解析 CANopen 訊息類型
            msg_type = self.parse_canopen_type(can_id)
            node_id = can_id & 0x7F
            
            print(f"[{timestamp:010d}] ID:0x{can_id:03X} Type:{msg_type} Node:{node_id:02d} Data:{data[:dlc].hex()}")
            
            # 寫入 PCAP 檔案
            if self.pcap_writer:
                self.write_to_pcap(frame)
```

### 6.2 即時 Wireshark 整合
```bash
# 建立具名管道 (Linux/macOS)
mkfifo /tmp/canopen_pipe

# 啟動監控程式，輸出到管道
python3 canopen_monitor.py --output-pipe /tmp/canopen_pipe

# Wireshark 讀取管道
wireshark -k -i /tmp/canopen_pipe
```

## 7. 實施建議

### 7.1 開發階段
1. **階段 1：基礎 USB CDC 實現** (1-2 天)
   - DAVE USB CDC 設定
   - 基本訊息捕獲和傳輸
   - PC 端接收驗證

2. **階段 2：協議格式化** (1 天)
   - 封包格式實現
   - 時間戳和校驗碼
   - 錯誤處理機制

3. **階段 3：Wireshark 整合** (1-2 天)
   - Lua 插件開發
   - PCAP 格式輸出
   - 即時捕獲測試

4. **階段 4：進階功能** (1-2 天)
   - 過濾和觸發功能
   - 統計和分析功能
   - GUI 分析工具

### 7.2 測試驗證
- 與真實 CANopen 設備測試
- 高負載壓力測試
- 長時間穩定性測試
- Wireshark 解析正確性驗證

## 8. 預期成果

### 8.1 核心功能
- ✅ 即時 CANopen 網路監控
- ✅ Wireshark 完整整合
- ✅ 專業級封包分析
- ✅ 多格式資料輸出 (PCAP, CSV, JSON)
- ✅ 即時統計和診斷

### 8.2 技術規格
- **捕獲速度**：>10,000 msgs/sec
- **時間精度**：1μs
- **緩衝深度**：>1000 訊息
- **傳輸延遲**：<10ms
- **資料完整性**：CRC32 校驗

這個方案結合了 XMC4800 的強大 CAN 功能和 Wireshark 的專業分析能力，將提供一個專業級的 CANopen 監控解決方案。您傾向於哪個方案？我建議先從 USB CDC 方案開始實現。