#!/usr/bin/env python3
"""
XMC4800 CANopen Monitor - PC Analysis Tool
專業級 CANopen 網路監控與分析工具

功能:
- 接收 XMC4800 傳送的 CANopen 封包
- 解析 CANopen 協議訊息
- 即時顯示網路狀態
- 輸出 Wireshark 相容格式
- 統計分析和診斷
"""

import serial
import struct
import time
import threading
import argparse
import sys
import json
from datetime import datetime
from collections import defaultdict, deque

class CANopenFrame:
    """CANopen 訊息框架類別"""
    
    def __init__(self, timestamp_us, can_id, dlc, data, flags=0):
        self.timestamp_us = timestamp_us
        self.can_id = can_id
        self.dlc = dlc
        self.data = data[:dlc] if dlc <= 8 else data[:8]
        self.flags = flags
        self.msg_type = self._get_message_type()
        self.node_id = self._get_node_id()
        self.type_string = self._get_type_string()
    
    def _get_message_type(self):
        """解析 CANopen 訊息類型"""
        if self.can_id == 0x000: return 0x00  # NMT Control
        if self.can_id == 0x080: return 0x01  # SYNC
        if self.can_id == 0x100: return 0x02  # TIME
        
        if 0x081 <= self.can_id <= 0x0FF: return 0x10  # Emergency
        if 0x180 <= self.can_id <= 0x1FF: return 0x20  # PDO1 TX
        if 0x200 <= self.can_id <= 0x27F: return 0x21  # PDO1 RX
        if 0x280 <= self.can_id <= 0x2FF: return 0x22  # PDO2 TX
        if 0x300 <= self.can_id <= 0x37F: return 0x23  # PDO2 RX
        if 0x380 <= self.can_id <= 0x3FF: return 0x24  # PDO3 TX
        if 0x400 <= self.can_id <= 0x47F: return 0x25  # PDO3 RX
        if 0x480 <= self.can_id <= 0x4FF: return 0x26  # PDO4 TX
        if 0x500 <= self.can_id <= 0x57F: return 0x27  # PDO4 RX
        if 0x580 <= self.can_id <= 0x5FF: return 0x30  # SDO TX
        if 0x600 <= self.can_id <= 0x67F: return 0x31  # SDO RX
        if 0x700 <= self.can_id <= 0x77F: return 0x40  # Heartbeat
        
        return 0xFF  # Unknown
    
    def _get_node_id(self):
        """解析節點 ID"""
        if self.can_id in [0x000, 0x080, 0x100]:
            return 0  # 廣播訊息
        return self.can_id & 0x7F
    
    def _get_type_string(self):
        """取得訊息類型字串"""
        type_map = {
            0x00: "NMT_CTRL", 0x01: "SYNC", 0x02: "TIME", 0x10: "EMERGENCY",
            0x20: "PDO1_TX", 0x21: "PDO1_RX", 0x22: "PDO2_TX", 0x23: "PDO2_RX",
            0x24: "PDO3_TX", 0x25: "PDO3_RX", 0x26: "PDO4_TX", 0x27: "PDO4_RX",
            0x30: "SDO_TX", 0x31: "SDO_RX", 0x40: "HEARTBEAT", 0xFF: "UNKNOWN"
        }
        return type_map.get(self.msg_type, "UNKNOWN")
    
    def __str__(self):
        timestamp_ms = self.timestamp_us / 1000.0
        data_hex = ' '.join(f'{b:02X}' for b in self.data)
        return (f"[{timestamp_ms:12.3f}] "
                f"ID:0x{self.can_id:03X} "
                f"Type:{self.type_string:12s} "
                f"Node:{self.node_id:2d} "
                f"DLC:{self.dlc} "
                f"Data:[{data_hex}]")

class CANopenMonitor:
    """CANopen 監控主類別"""
    
    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn = None
        self.running = False
        self.stats = {
            'total_packets': 0,
            'total_frames': 0,
            'frame_types': defaultdict(int),
            'node_activity': defaultdict(int),
            'errors': 0,
            'start_time': None
        }
        self.recent_frames = deque(maxlen=1000)
        self.pcap_writer = None
        
    def connect(self):
        """連接到串口"""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1.0,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS
            )
            print(f"✅ 已連接到 {self.port} (波特率: {self.baudrate})")
            return True
        except Exception as e:
            print(f"❌ 連接失敗: {e}")
            return False
    
    def disconnect(self):
        """斷開串口連接"""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            print("🔌 已斷開連接")
    
    def read_packet(self):
        """讀取一個 CANopen 封包"""
        try:
            # 讀取封包標頭 (magic + version + frame_count)
            header = self.serial_conn.read(8)
            if len(header) < 8:
                return None
            
            magic, version, frame_count = struct.unpack('<LHH', header)
            
            # 驗證魔術字元
            if magic != 0x43414E4F:  # "CANO"
                self.stats['errors'] += 1
                return None
            
            # 讀取 CAN 訊息
            frames = []
            for _ in range(frame_count):
                frame_data = self.serial_conn.read(16)  # sizeof(canopen_frame_t)
                if len(frame_data) < 16:
                    self.stats['errors'] += 1
                    return None
                
                # 解析訊息結構
                timestamp_us, can_id, dlc, d0, d1, d2, d3, d4, d5, d6, d7, flags, reserved = \
                    struct.unpack('<LLBBBBBBBBBBBB', frame_data)
                
                data = [d0, d1, d2, d3, d4, d5, d6, d7]
                frame = CANopenFrame(timestamp_us, can_id, dlc, data, flags)
                frames.append(frame)
            
            # 讀取校驗碼
            checksum_data = self.serial_conn.read(4)
            if len(checksum_data) < 4:
                self.stats['errors'] += 1
                return None
            
            checksum = struct.unpack('<L', checksum_data)[0]
            
            return frames
            
        except Exception as e:
            print(f"❌ 讀取封包錯誤: {e}")
            self.stats['errors'] += 1
            return None
    
    def process_frames(self, frames):
        """處理 CANopen 訊息"""
        for frame in frames:
            # 更新統計
            self.stats['total_frames'] += 1
            self.stats['frame_types'][frame.type_string] += 1
            if frame.node_id > 0:
                self.stats['node_activity'][frame.node_id] += 1
            
            # 儲存到最近訊息
            self.recent_frames.append(frame)
            
            # 顯示訊息
            print(frame)
            
            # 寫入 PCAP (如果需要)
            if self.pcap_writer:
                self.write_to_pcap(frame)
    
    def write_to_pcap(self, frame):
        """寫入 PCAP 檔案 (待實現)"""
        # TODO: 實現 PCAP 寫入功能
        pass
    
    def display_statistics(self):
        """顯示統計資訊"""
        if not self.stats['start_time']:
            return
        
        elapsed = time.time() - self.stats['start_time']
        print(f"\n📊 === CANopen 監控統計 ===")
        print(f"運行時間: {elapsed:.1f}s")
        print(f"總封包數: {self.stats['total_packets']}")
        print(f"總訊息數: {self.stats['total_frames']}")
        print(f"訊息速率: {self.stats['total_frames']/elapsed:.1f} msg/s")
        print(f"錯誤數量: {self.stats['errors']}")
        
        print(f"\n📋 訊息類型統計:")
        for msg_type, count in sorted(self.stats['frame_types'].items()):
            percentage = (count / self.stats['total_frames']) * 100 if self.stats['total_frames'] > 0 else 0
            print(f"  {msg_type:12s}: {count:6d} ({percentage:5.1f}%)")
        
        print(f"\n🏠 節點活動統計:")
        for node_id, count in sorted(self.stats['node_activity'].items()):
            percentage = (count / self.stats['total_frames']) * 100 if self.stats['total_frames'] > 0 else 0
            print(f"  節點 {node_id:2d}: {count:6d} ({percentage:5.1f}%)")
        print()
    
    def start_monitoring(self, show_stats_interval=10):
        """開始監控"""
        if not self.connect():
            return False
        
        self.running = True
        self.stats['start_time'] = time.time()
        
        print("🔍 開始 CANopen 網路監控...")
        print("按 Ctrl+C 停止監控\n")
        
        last_stats_time = time.time()
        
        try:
            while self.running:
                frames = self.read_packet()
                if frames:
                    self.stats['total_packets'] += 1
                    self.process_frames(frames)
                
                # 定期顯示統計
                current_time = time.time()
                if current_time - last_stats_time >= show_stats_interval:
                    self.display_statistics()
                    last_stats_time = current_time
                
        except KeyboardInterrupt:
            print("\n⏹️  停止監控...")
        except Exception as e:
            print(f"❌ 監控錯誤: {e}")
        finally:
            self.running = False
            self.display_statistics()
            self.disconnect()
        
        return True

def main():
    """主程式"""
    parser = argparse.ArgumentParser(description='XMC4800 CANopen Monitor - PC Analysis Tool')
    parser.add_argument('port', help='串口名稱 (例: COM3 或 /dev/ttyUSB0)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='波特率 (預設: 115200)')
    parser.add_argument('-s', '--stats-interval', type=int, default=10, help='統計顯示間隔 (秒)')
    parser.add_argument('-o', '--output', help='PCAP 輸出檔案')
    
    args = parser.parse_args()
    
    print("🚀 XMC4800 CANopen Professional Monitor")
    print("="*50)
    
    monitor = CANopenMonitor(args.port, args.baudrate)
    
    if args.output:
        print(f"📝 PCAP 輸出: {args.output}")
        # TODO: 初始化 PCAP 寫入器
    
    success = monitor.start_monitoring(args.stats_interval)
    
    if success:
        print("✅ 監控完成")
    else:
        print("❌ 監控失敗")
        sys.exit(1)

if __name__ == "__main__":
    main()