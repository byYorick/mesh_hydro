#!/usr/bin/env python3
"""
Упрощенное тестирование PID системы с моковыми данными pH
Без внешних зависимостей
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

# Моковые данные pH (симуляция реального датчика)
class MockPHSensor:
    def __init__(self, initial_ph=7.0, target_ph=6.5):
        self.current_ph = initial_ph
        self.target_ph = target_ph
        self.time = 0
        self.noise_level = 0.05  # Уровень шума
        self.drift_rate = 0.001  # Скорость дрейфа
        
    def read_ph(self):
        """Читает моковое значение pH с шумом и дрейфом"""
        # Добавляем шум
        noise = random.gauss(0, self.noise_level)
        
        # Добавляем дрейф (медленное изменение)
        drift = math.sin(self.time * 0.01) * self.drift_rate
        
        # Обновляем значение
        self.current_ph += noise + drift
        
        # Ограничиваем диапазон
        self.current_ph = max(5.0, min(9.0, self.current_ph))
        
        self.time += 1
        return round(self.current_ph, 2)
    
    def simulate_pump_effect(self, pump_type, volume_ml):
        """Симулирует эффект работы насоса на pH"""
        if pump_type == "ph_up":
            # pH UP увеличивает pH
            self.current_ph += volume_ml * 0.1  # 1 мл = 0.1 pH
        elif pump_type == "ph_down":
            # pH DOWN уменьшает pH
            self.current_ph -= volume_ml * 0.1  # 1 мл = 0.1 pH
        
        # Ограничиваем диапазон
        self.current_ph = max(5.0, min(9.0, self.current_ph))

# Создаем моковый датчик
mock_sensor = MockPHSensor(initial_ph=7.2, target_ph=TARGET_PH)

def send_telemetry(ph_value, node_id=NODE_ID):
    """Отправляет телеметрию на сервер"""
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
    """Отправляет событие на сервер"""
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
            print(f"🔔 Событие отправлено: {level} - {message}")
        else:
            print(f"❌ Ошибка отправки события: {response.getcode()}")
            
    except Exception as e:
        print(f"❌ Ошибка: {e}")

def check_api():
    """Проверяет доступность API"""
    try:
        req = urllib.request.Request(f"{API_BASE}/nodes")
        response = urllib.request.urlopen(req, timeout=5)
        if response.getcode() == 200:
            print("✅ API доступен")
            return True
        else:
            print("❌ API недоступен")
            return False
    except Exception as e:
        print(f"❌ Ошибка подключения к API: {e}")
        return False

def simulate_pid_control():
    """Симулирует работу PID контроллера"""
    print("🎯 Запуск симуляции PID системы с моковыми данными pH")
    print(f"📊 Целевое значение pH: {TARGET_PH}")
    print(f"🔧 Начальное значение pH: {mock_sensor.current_ph}")
    print("-" * 60)
    
    iteration = 0
    last_correction_time = 0
    correction_interval = 10  # Интервал между коррекциями (секунд)
    
    while True:
        iteration += 1
        
        # Читаем моковое значение pH
        current_ph = mock_sensor.read_ph()
        
        # Отправляем телеметрию
        send_telemetry(current_ph)
        
        # Вычисляем ошибку
        error = TARGET_PH - current_ph
        abs_error = abs(error)
        
        # Определяем зону PID
        if abs_error <= 0.1:
            zone = "DEAD"
            zone_color = "🟢"
        elif abs_error <= 0.3:
            zone = "CLOSE"
            zone_color = "🟡"
        else:
            zone = "FAR"
            zone_color = "🔴"
        
        print(f"[{iteration:03d}] pH={current_ph:.2f} | Error={error:+.2f} | Zone={zone_color} {zone}")
        
        # Симуляция PID коррекции
        current_time = time.time()
        if current_time - last_correction_time >= correction_interval:
            if zone != "DEAD" and abs_error > 0.05:
                # Вычисляем объем коррекции (упрощенный PID)
                if zone == "CLOSE":
                    kp, ki, kd = 0.5, 0.01, 0.1
                else:  # FAR
                    kp, ki, kd = 1.0, 0.015, 0.05
                
                # Простой PID расчет
                p_term = kp * error
                output_ml = abs(p_term) * 2.0  # Масштабирование
                output_ml = min(output_ml, 5.0)  # Ограничение максимума
                
                if output_ml > 0.1:  # Минимальный порог
                    pump_type = "ph_up" if error < 0 else "ph_down"
                    
                    print(f"    🔧 PID коррекция: {pump_type} {output_ml:.2f}ml (Kp={kp}, Ki={ki}, Kd={kd})")
                    
                    # Симулируем эффект насоса
                    mock_sensor.simulate_pump_effect(pump_type, output_ml)
                    
                    # Отправляем событие
                    if zone == "FAR":
                        send_event("warning", f"pH far from target, aggressive correction", current_ph)
                    else:
                        send_event("info", f"pH correction in progress", current_ph)
                    
                    last_correction_time = current_time
        
        # Проверяем аварийные условия
        if current_ph < 5.5 or current_ph > 7.5:
            send_event("critical", f"pH out of range: {current_ph:.2f}", current_ph)
            print(f"    🚨 АВАРИЯ: pH вне диапазона!")
        
        # Ждем перед следующей итерацией
        time.sleep(2)

def main():
    print("Sимулятор PID системы для pH узла")
    print("=" * 60)
    
    # Проверяем доступность API
    if not check_api():
        print("❌ API недоступен. Убедитесь, что сервер запущен.")
        return
    
    # Запускаем симуляцию
    try:
        simulate_pid_control()
    except KeyboardInterrupt:
        print("\n🛑 Симуляция остановлена пользователем")
    except Exception as e:
        print(f"\n❌ Ошибка симуляции: {e}")

if __name__ == "__main__":
    main()
