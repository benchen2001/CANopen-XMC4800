#!/usr/bin/env python3
"""
UART 監控腳本 - 監控 XMC4800 CANopen API 測試輸出
"""

import serial
import time
import sys

def monitor_uart():
    """監控 UART 輸出"""
    try:
        # 配置 UART 連接 (需要根據實際 COM 口修改)
        # 常見的 COM 口: COM3, COM4, COM5...
        for port in ['COM3', 'COM4', 'COM5', 'COM6', 'COM7', 'COM8']:
            try:
                ser = serial.Serial(
                    port=port,
                    baudrate=115200,
                    bytesize=serial.EIGHTBITS,
                    parity=serial.PARITY_NONE,
                    stopbits=serial.STOPBITS_ONE,
                    timeout=1
                )
                print(f"成功連接到 {port}")
                break
            except:
                continue
        else:
            print("無法找到可用的 COM 口")
            return
        
        print("開始監控 XMC4800 CANopen API 測試輸出...")
        print("=" * 60)
        
        buffer = ""
        
        while True:
            try:
                # 讀取 UART 數據
                data = ser.read(100)
                if data:
                    # 解碼並添加到緩衝區
                    text = data.decode('utf-8', errors='ignore')
                    buffer += text
                    
                    # 處理完整的行
                    while '\r\n' in buffer:
                        line, buffer = buffer.split('\r\n', 1)
                        if line.strip():
                            timestamp = time.strftime("%H:%M:%S")
                            print(f"[{timestamp}] {line}")
                            
                            # 特殊標記重要訊息
                            if "ERROR:" in line:
                                print("  *** 錯誤訊息 ***")
                            elif "API 功能測試" in line:
                                print("  >>> CANopen API 測試開始 <<<")
                            elif "Emergency" in line:
                                print("  --> Emergency 訊息")
                            elif "TPDO" in line:
                                print("  --> TPDO 訊息")
                            elif "CAN 傳送" in line:
                                print("  --> 直接 CAN 傳送")
                            elif "TX:" in line or "ID=0x" in line:
                                print("  ==> CAN 傳輸確認")
                
                time.sleep(0.01)  # 短暫延遲避免 CPU 滿載
                
            except KeyboardInterrupt:
                print("\n監控停止")
                break
            except Exception as e:
                print(f"讀取錯誤: {e}")
                time.sleep(1)
        
        ser.close()
        
    except Exception as e:
        print(f"UART 監控錯誤: {e}")

if __name__ == "__main__":
    monitor_uart()