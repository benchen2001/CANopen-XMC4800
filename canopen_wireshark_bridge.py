#!/usr/bin/env python3
"""
CANopen Wireshark 橋接程式
從 XMC4800 接收 pcap 格式資料並餵給 Wireshark

使用方法:
1. 連接 XMC4800 開發板
2. 執行: python canopen_wireshark_bridge.py
3. 在另一個終端執行: wireshark -k -i - < canopen_live.pcap
"""

import serial
import sys
import threading
import time
from datetime import datetime

class CANopenWiresharkBridge:
    def __init__(self, serial_port='COM3', baudrate=115200, output_file='canopen_live.pcap'):
        """
        初始化橋接程式
        
        Args:
            serial_port: 串列埠名稱 (Windows: COM3, Linux: /dev/ttyUSB0)
            baudrate: 鮑率 (預設 115200)
            output_file: 輸出的 pcap 檔案名稱
        """
        self.serial_port = serial_port
        self.baudrate = baudrate
        self.output_file = output_file
        self.running = False
        self.serial_conn = None
        self.output_fp = None
        
    def start(self):
        """啟動橋接服務"""
        try:
            # 開啟串列埠
            print(f"正在連接到 {self.serial_port}...")
            self.serial_conn = serial.Serial(
                port=self.serial_port,
                baudrate=self.baudrate,
                timeout=1,
                bytesize=8,
                parity='N',
                stopbits=1
            )
            
            # 開啟輸出檔案
            print(f"正在建立 pcap 檔案: {self.output_file}")
            self.output_fp = open(self.output_file, 'wb')
            
            self.running = True
            print("CANopen Wireshark 橋接程式已啟動")
            print("按 Ctrl+C 停止...")
            print("-" * 50)
            
            # 統計變數
            packet_count = 0
            start_time = time.time()
            
            # 主要接收迴圈
            while self.running:
                try:
                    # 讀取資料
                    data = self.serial_conn.read(1024)
                    if data:
                        # 寫入 pcap 檔案
                        self.output_fp.write(data)
                        self.output_fp.flush()
                        
                        packet_count += 1
                        if packet_count % 100 == 0:
                            elapsed = time.time() - start_time
                            rate = packet_count / elapsed if elapsed > 0 else 0
                            print(f"[{datetime.now().strftime('%H:%M:%S')}] "
                                  f"已處理 {packet_count} 個封包 "
                                  f"(速率: {rate:.1f} pkt/s)")
                    
                except serial.SerialTimeoutException:
                    continue
                except KeyboardInterrupt:
                    break
                    
        except serial.SerialException as e:
            print(f"串列埠錯誤: {e}")
            print("請檢查:")
            print("1. XMC4800 是否已連接")
            print("2. 串列埠名稱是否正確")
            print("3. 其他程式是否佔用串列埠")
            
        except Exception as e:
            print(f"未預期的錯誤: {e}")
            
        finally:
            self.stop()
    
    def stop(self):
        """停止橋接服務"""
        self.running = False
        
        if self.serial_conn:
            self.serial_conn.close()
            print("串列埠已關閉")
            
        if self.output_fp:
            self.output_fp.close()
            print(f"pcap 檔案已儲存: {self.output_file}")

def main():
    """主程式"""
    print("=== CANopen Wireshark 橋接程式 ===")
    print("版本: 1.0")
    print("作者: CANopen Monitor Team")
    print()
    
    # 檢查命令列參數
    serial_port = 'COM3'  # 預設值
    if len(sys.argv) > 1:
        serial_port = sys.argv[1]
    
    print(f"使用串列埠: {serial_port}")
    print("如需更改，請使用: python canopen_wireshark_bridge.py COMx")
    print()
    
    # 建立並啟動橋接服務
    bridge = CANopenWiresharkBridge(serial_port=serial_port)
    
    try:
        bridge.start()
    except KeyboardInterrupt:
        print("\n正在停止...")
        bridge.stop()
    
    print("\n橋接程式已停止")

if __name__ == '__main__':
    main()