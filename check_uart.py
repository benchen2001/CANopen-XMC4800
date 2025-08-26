#!/usr/bin/env python3
"""
XMC4800 CANopen UART 監控工具
自動檢測 UART 連接埠並監控 CANopen 訊息
"""

import serial
import serial.tools.list_ports
import time
import sys

def find_xmc4800_port():
    """尋找 XMC4800 的 UART 連接埠"""
    ports = serial.tools.list_ports.comports()
    
    print("可用的串口設備：")
    for i, port in enumerate(ports):
        print(f"{i+1}. {port.device} - {port.description}")
        if "USB" in port.description or "COM" in port.device:
            print(f"   推薦: {port.device}")
    
    if not ports:
        print("❌ 未找到任何串口設備")
        return None
    
    # 自動選擇第一個可用端口
    selected_port = ports[0].device
    print(f"\n🎯 自動選擇: {selected_port}")
    return selected_port

def monitor_uart(port_name, baudrate=115200):
    """監控 UART 輸出"""
    try:
        print(f"\n🔌 正在連接到 {port_name} (波特率: {baudrate})")
        
        with serial.Serial(port_name, baudrate, timeout=1) as ser:
            print(f"✅ 成功連接到 {port_name}")
            print("=" * 60)
            print("📡 CANopen XMC4800 UART 監控開始...")
            print("   按 Ctrl+C 停止監控")
            print("=" * 60)
            
            start_time = time.time()
            line_count = 0
            
            while True:
                try:
                    # 讀取一行數據
                    if ser.in_waiting > 0:
                        line = ser.readline().decode('utf-8', errors='replace').strip()
                        if line:
                            current_time = time.time() - start_time
                            line_count += 1
                            print(f"[{current_time:7.2f}s] {line}")
                    
                    time.sleep(0.01)  # 短暫延遲
                    
                except KeyboardInterrupt:
                    print(f"\n\n🛑 監控停止 (共收到 {line_count} 行)")
                    break
                    
    except serial.SerialException as e:
        print(f"❌ 串口錯誤: {e}")
        return False
    except Exception as e:
        print(f"❌ 未知錯誤: {e}")
        return False
    
    return True

def main():
    print("🚀 XMC4800 CANopen UART 監控工具")
    print("=" * 50)
    
    # 尋找串口
    port = find_xmc4800_port()
    if not port:
        return 1
    
    # 開始監控
    success = monitor_uart(port)
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())