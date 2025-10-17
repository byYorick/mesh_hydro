#!/usr/bin/env python3
"""
Мониторинг mesh-сети в реальном времени
"""

import serial
import argparse
import json
from datetime import datetime

def monitor_mesh(port, baudrate=115200):
    """Мониторинг serial порта и вывод mesh данных"""
    
    print(f"Connecting to {port} at {baudrate} baud...")
    
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected! Monitoring mesh network...\n")
        
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if line:
                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    
                    # Подсветка важных строк
                    if "MESH" in line or "mesh" in line:
                        print(f"[{timestamp}] \033[92m{line}\033[0m")  # Зеленый
                    elif "ERROR" in line or "error" in line:
                        print(f"[{timestamp}] \033[91m{line}\033[0m")  # Красный
                    elif "WARN" in line or "warn" in line:
                        print(f"[{timestamp}] \033[93m{line}\033[0m")  # Желтый
                    else:
                        print(f"[{timestamp}] {line}")
                        
    except KeyboardInterrupt:
        print("\nMonitoring stopped")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Mesh Network Monitor")
    parser.add_argument("--port", "-p", required=True, help="Serial port (e.g., COM3 or /dev/ttyUSB0)")
    parser.add_argument("--baud", "-b", type=int, default=115200, help="Baudrate (default: 115200)")
    
    args = parser.parse_args()
    monitor_mesh(args.port, args.baud)

