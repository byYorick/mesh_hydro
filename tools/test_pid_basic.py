#!/usr/bin/env python3
"""
Базовое тестирование PID системы с моковыми данными pH
"""

import time
import json
import urllib.request
import urllib.parse
import random
import math
from datetime import datetime

# Конфигурация
API_BASE = "http://localhost:3000/api"
NODE_ID = "ph_3f0c00"  # ID pH узла
TARGET_PH = 6.5  # Целевое значение pH

# Моковые данные pH
class MockPHSensor:
    def __init__(self, initial_ph=7.0, target_ph=6.5):
        self.current_ph = initial_ph
        self.target_ph = target_ph
        self.time = 0
        self.noise_level = 0.05
        self.drift_rate = 0.001
        
    def read_ph(self):
        noise = random.gauss(0, self.noise_level)
        drift = math.sin(self.time * 0.01) * self.drift_rate
        self.current_ph += noise + drift
        self.current_ph = max(5.0, min(9.0, self.current_ph))
        self.time += 1
        return round(self.current_ph, 2)
    
    def simulate_pump_effect(self, pump_type, volume_ml):
        if pump_type == "ph_up":
            self.current_ph += volume_ml * 0.1
        elif pump_type == "ph_down":
            self.current_ph -= volume_ml * 0.1
        self.current_ph = max(5.0, min(9.0, self.current_ph))

mock_sensor = MockPHSensor(initial_ph=7.2, target_ph=TARGET_PH)

def send_telemetry(ph_value, node_id=NODE_ID):
    try:
        data = {
            "node_id": node_id,
            "node_type": "ph",
            "data": {
                "ph": ph_value,
                "ph_target": TARGET_PH,
                "ph_min": 5.5,
                "ph_max": 7.5,
                "temperature": 25.0 + random.uniform(-2, 2),
                "voltage": 3.3 + random.uniform(-0.1, 0.1),
                "uptime": int(time.time()),
                "heap_free": 50000 + random.randint(-5000, 5000)
            },
            "received_at": datetime.now().isoformat()
        }
        
        json_data = json.dumps(data).encode('utf-8')
        req = urllib.request.Request(
            f"{API_BASE}/telemetry",
            data=json_data,
            headers={'Content-Type': 'application/json'}
        )
        
        response = urllib.request.urlopen(req, timeout=5)
        if response.getcode() == 200:
            print(f"OK Телеметрия отправлена: pH={ph_value}")
        else:
            print(f"ERROR Ошибка отправки телеметрии: {response.getcode()}")
            
    except Exception as e:
        print(f"ERROR Ошибка: {e}")

def send_event(level, message, ph_value, node_id=NODE_ID):
    try:
        data = {
            "node_id": node_id,
            "level": level,
            "message": message,
            "data": {
                "ph": ph_value,
                "ph_target": TARGET_PH,
                "pump_id": "ph_up" if "UP" in message else "ph_down",
                "kp": 1.0,
                "ki": 0.05,
                "kd": 0.3,
                "current_value": ph_value,
                "target_value": TARGET_PH
            },
            "created_at": datetime.now().isoformat()
        }
        
        json_data = json.dumps(data).encode('utf-8')
        req = urllib.request.Request(
            f"{API_BASE}/events",
            data=json_data,
            headers={'Content-Type': 'application/json'}
        )
        
        response = urllib.request.urlopen(req, timeout=5)
        if response.getcode() == 200:
            print(f"EVENT Событие отправлено: {level} - {message}")
        else:
            print(f"ERROR Ошибка отправки события: {response.getcode()}")
            
    except Exception as e:
        print(f"ERROR Ошибка: {e}")

def check_api():
    try:
        req = urllib.request.Request(f"{API_BASE}/nodes")
        response = urllib.request.urlopen(req, timeout=5)
        if response.getcode() == 200:
            print("OK API доступен")
            return True
        else:
            print("ERROR API недоступен")
            return False
    except Exception as e:
        print(f"ERROR Ошибка подключения к API: {e}")
        return False

def simulate_pid_control():
    print("Запуск симуляции PID системы с моковыми данными pH")
    print(f"Целевое значение pH: {TARGET_PH}")
    print(f"Начальное значение pH: {mock_sensor.current_ph}")
    print("-" * 60)
    
    iteration = 0
    last_correction_time = 0
    correction_interval = 10
    
    while True:
        iteration += 1
        
        current_ph = mock_sensor.read_ph()
        send_telemetry(current_ph)
        
        error = TARGET_PH - current_ph
        abs_error = abs(error)
        
        if abs_error <= 0.1:
            zone = "DEAD"
            zone_color = "GREEN"
        elif abs_error <= 0.3:
            zone = "CLOSE"
            zone_color = "YELLOW"
        else:
            zone = "FAR"
            zone_color = "RED"
        
        print(f"[{iteration:03d}] pH={current_ph:.2f} | Error={error:+.2f} | Zone={zone_color} {zone}")
        
        current_time = time.time()
        if current_time - last_correction_time >= correction_interval:
            if zone != "DEAD" and abs_error > 0.05:
                if zone == "CLOSE":
                    kp, ki, kd = 0.5, 0.01, 0.1
                else:
                    kp, ki, kd = 1.0, 0.015, 0.05
                
                p_term = kp * error
                output_ml = abs(p_term) * 2.0
                output_ml = min(output_ml, 5.0)
                
                if output_ml > 0.1:
                    pump_type = "ph_up" if error < 0 else "ph_down"
                    
                    print(f"    PID коррекция: {pump_type} {output_ml:.2f}ml (Kp={kp}, Ki={ki}, Kd={kd})")
                    
                    mock_sensor.simulate_pump_effect(pump_type, output_ml)
                    
                    if zone == "FAR":
                        send_event("warning", f"pH far from target, aggressive correction", current_ph)
                    else:
                        send_event("info", f"pH correction in progress", current_ph)
                    
                    last_correction_time = current_time
        
        if current_ph < 5.5 or current_ph > 7.5:
            send_event("critical", f"pH out of range: {current_ph:.2f}", current_ph)
            print(f"    АВАРИЯ: pH вне диапазона!")
        
        time.sleep(2)

def main():
    print("Симулятор PID системы для pH узла")
    print("=" * 60)
    
    if not check_api():
        print("ERROR API недоступен. Убедитесь, что сервер запущен.")
        return
    
    try:
        simulate_pid_control()
    except KeyboardInterrupt:
        print("\nСимуляция остановлена пользователем")
    except Exception as e:
        print(f"\nERROR Ошибка симуляции: {e}")

if __name__ == "__main__":
    main()
