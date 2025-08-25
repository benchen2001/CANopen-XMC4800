#!/usr/bin/env python3
"""
Wireshark Bridge for XMC4800 CANopen Monitor
接收來自 XMC4800 的 pcap 資料並轉發給 Wireshark
"""

import serial
import time
import struct
import sys
import subprocess
import threading
from datetime import datetime

class WiresharkBridge:
    def __init__(self, port='COM3', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn = None
        self.wireshark_pipe = None
        self.running = False
        
    def find_serial_port(self):
        """自動尋找 J-Link CDC UART Port"""
        import serial.tools.list_ports
        
        for port in serial.tools.list_ports.comports():
            if 'JLink' in port.description or 'CDC' in port.description:
                print(f"找到 J-Link 串列埠: {port.device} - {port.description}")
                return port.device
        
        # 如果沒找到，嘗試常見的 COM 埠
        for i in range(1, 20):
            try:
                port_name = f'COM{i}'
                s = serial.Serial(port_name, self.baudrate, timeout=1)
                s.close()
                print(f"可用串列埠: {port_name}")
                return port_name
            except:
                continue
        
        return None
    
    def start_wireshark(self):
        """啟動 Wireshark 並建立命名管道"""
        try:
            # 創建臨時 pcap 檔案
            self.pcap_file = "canopen_capture.pcap"
            
            # 寫入 pcap 檔案標頭
            with open(self.pcap_file, 'wb') as f:
                # Global Header (24 bytes)
                f.write(struct.pack('<LHHLLLL', 
                    0xa1b2c3d4,  # magic number
                    2,           # version major
                    4,           # version minor
                    0,           # thiszone
                    0,           # sigfigs
                    65535,       # snaplen
                    1            # network (Ethernet)
                ))
            
            print(f"pcap 檔案已創建: {self.pcap_file}")
            return True
            
        except Exception as e:
            print(f"啟動 Wireshark 失敗: {e}")
            return False
    
    def connect_serial(self):
        """連接串列埠"""
        try:
            if not self.port:
                self.port = self.find_serial_port()
                if not self.port:
                    print("錯誤：找不到可用的串列埠")
                    return False
            
            self.serial_conn = serial.Serial(
                self.port, 
                self.baudrate, 
                timeout=1,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS
            )
            
            print(f"成功連接到 {self.port} @ {self.baudrate} baud")
            return True
            
        except Exception as e:
            print(f"串列埠連接失敗: {e}")
            return False
    
    def process_pcap_packet(self, data):
        """處理接收到的 pcap 封包"""
        try:
            if len(data) < 16:  # 最小 pcap 記錄大小
                return
            
            # 寫入 pcap 檔案
            with open(self.pcap_file, 'ab') as f:
                f.write(data)
            
            # 解析封包內容 (簡單版本)
            if len(data) >= 50:  # 有足夠資料解析
                timestamp = struct.unpack('<L', data[0:4])[0]
                packet_len = struct.unpack('<L', data[8:12])[0]
                
                print(f"[{datetime.now().strftime('%H:%M:%S')}] "
                      f"收到封包: {packet_len} bytes, TS={timestamp}")
            
        except Exception as e:
            print(f"處理封包錯誤: {e}")
    
    def run(self):
        """主執行迴圈"""
        print("=== XMC4800 CANopen Wireshark Bridge ===")
        
        if not self.start_wireshark():
            return
        
        if not self.connect_serial():
            return
        
        self.running = True
        packet_count = 0
        
        print(f"開始監控 CANopen 封包...")
        print(f"pcap 檔案: {self.pcap_file}")
        print("按 Ctrl+C 停止監控")
        
        try:
            while self.running:
                # 讀取串列埠資料
                if self.serial_conn.in_waiting > 0:
                    data = self.serial_conn.read(self.serial_conn.in_waiting)
                    
                    if data:
                        self.process_pcap_packet(data)
                        packet_count += 1
                        
                        if packet_count % 10 == 0:
                            print(f"已處理 {packet_count} 個封包")
                
                time.sleep(0.001)  # 1ms 延遲
                
        except KeyboardInterrupt:
            print("\n收到停止信號...")
        except Exception as e:
            print(f"執行錯誤: {e}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        """清理資源"""
        self.running = False
        
        if self.serial_conn:
            self.serial_conn.close()
            print("串列埠已關閉")
        
        print(f"總共處理的封包數: 已儲存到 {self.pcap_file}")
        print("可以用 Wireshark 開啟此檔案進行分析")

if __name__ == "__main__":
    bridge = WiresharkBridge()
    
    # 檢查命令列參數
    if len(sys.argv) > 1:
        bridge.port = sys.argv[1]
    
    bridge.run()