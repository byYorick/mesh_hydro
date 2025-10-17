#!/usr/bin/env python3
"""
Тестирование MQTT подключения
"""

import paho.mqtt.client as mqtt
import argparse
import json
import time

def on_connect(client, userdata, flags, rc):
    """Callback при подключении"""
    if rc == 0:
        print("✓ Connected to MQTT broker")
        client.subscribe("hydro/#")
        print("✓ Subscribed to hydro/#")
    else:
        print(f"✗ Connection failed with code {rc}")

def on_message(client, userdata, msg):
    """Callback при получении сообщения"""
    timestamp = time.strftime("%H:%M:%S")
    print(f"\n[{timestamp}] Topic: {msg.topic}")
    
    try:
        data = json.loads(msg.payload.decode())
        print(json.dumps(data, indent=2))
    except:
        print(f"Payload: {msg.payload.decode()}")

def test_mqtt(broker, port, topic):
    """Тестирование MQTT"""
    
    print(f"Connecting to MQTT broker: {broker}:{port}")
    
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        client.connect(broker, port, 60)
        
        # Отправка тестового сообщения
        test_message = {
            "type": "test",
            "timestamp": int(time.time()),
            "message": "Test message from mqtt_tester.py"
        }
        
        client.publish(topic, json.dumps(test_message))
        print(f"\n✓ Test message published to {topic}")
        
        print("\nListening for messages... (Ctrl+C to stop)\n")
        client.loop_forever()
        
    except KeyboardInterrupt:
        print("\nStopped")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        client.disconnect()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MQTT Tester")
    parser.add_argument("--broker", "-b", default="localhost", help="MQTT broker address")
    parser.add_argument("--port", "-p", type=int, default=1883, help="MQTT port")
    parser.add_argument("--topic", "-t", default="hydro/test", help="Test topic")
    
    args = parser.parse_args()
    test_mqtt(args.broker, args.port, args.topic)

