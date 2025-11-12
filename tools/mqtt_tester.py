#!/usr/bin/env python3
"""
Тестирование MQTT подключения
"""

import argparse
import json
import time
from typing import Tuple, Any, Dict

import paho.mqtt.client as mqtt

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

def build_sample_message(args: argparse.Namespace) -> Tuple[str, Dict[str, Any]]:
    timestamp = int(time.time())
    mesh_id = args.mesh_id or args.zone
    node_id = args.node_id
    root_id = args.root_id
    node_type = args.node_type

    if args.mode == 'custom':
        if not args.topic:
            raise ValueError("Для режима custom необходимо указать --topic.")
        if not args.payload:
            raise ValueError("Для режима custom необходимо указать --payload (JSON).")
        payload = json.loads(args.payload)
        return args.topic, payload

    if not mesh_id:
        raise ValueError("Необходимо указать mesh_id или zone.")

    base_payload = {
        "node_id": node_id,
        "root_node_id": root_id,
        "mesh_network_id": mesh_id,
        "node_type": node_type,
        "timestamp": timestamp,
    }

    if args.mode == 'discovery':
        topic = f"hydro/{mesh_id}/discovery"
        payload = {
            **base_payload,
            "type": "discovery",
            "firmware": args.firmware,
            "hardware": args.hardware,
            "mac_address": args.mac,
            "sensors": ["temperature", "humidity", "ph"],
            "actuators": ["pump_a", "pump_b"],
        }
    elif args.mode == 'heartbeat':
        topic = f"hydro/{mesh_id}/heartbeat/{node_id}"
        payload = {
            **base_payload,
            "type": "heartbeat",
            "uptime": args.uptime,
            "heap_free": 125000,
            "rssi": -48,
            "status": "ok",
        }
    elif args.mode == 'telemetry':
        topic = f"hydro/{mesh_id}/telemetry/{node_id}"
        payload = {
            **base_payload,
            "type": "telemetry",
            "data": {
                "temperature": 23.7,
                "humidity": 64.2,
                "ph": 6.48,
                "ec": 1.72,
                "tank_level": 78,
            },
        }
    elif args.mode == 'command':
        topic = f"hydro/{mesh_id}/command/{node_id}"
        command_payload = args.command_payload
        try:
            params = json.loads(command_payload) if command_payload else {}
        except json.JSONDecodeError as exc:
            raise ValueError(f"Некорректный JSON в --command-payload: {exc}") from exc

        payload = {
            "type": "command",
            "command": args.command_name,
            "params": params,
            "command_id": int(time.time() * 1000),
            "issued_by": "mqtt_tester",
            "mesh_network_id": mesh_id,
            "root_node_id": root_id,
            "timestamp": timestamp,
        }
    else:
        raise ValueError(f"Неизвестный режим: {args.mode}")

    return topic, payload


def test_mqtt(args: argparse.Namespace):
    """Тестирование MQTT"""
    
    broker = args.broker
    port = args.port
    topic, payload = build_sample_message(args)

    mesh_id = args.mesh_id or args.zone

    print(f"Connecting to MQTT broker: {broker}:{port}")
    
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        client.connect(broker, port, 60)
        
        subscribe_topic = f"hydro/{mesh_id}/#"
        client.subscribe(subscribe_topic)
        print(f"✓ Subscribed to {subscribe_topic}")

        client.publish(topic, json.dumps(payload))
        print(f"\n✓ Test message ({args.mode}) published to {topic}")
        print(json.dumps(payload, indent=2, ensure_ascii=False))
        
        print("\nListening for messages... (Ctrl+C to stop)\n")
        client.loop_forever()
        
    except KeyboardInterrupt:
        print("\nStopped")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        client.disconnect()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MQTT Tester для мультизонной системы")
    parser.add_argument("--broker", "-b", default="localhost", help="MQTT broker address")
    parser.add_argument("--port", "-p", type=int, default=1883, help="MQTT port")
    parser.add_argument(
        "--mode",
        "-m",
        default="discovery",
        choices=["discovery", "heartbeat", "telemetry", "command", "custom"],
        help="Тип тестового сообщения",
    )
    parser.add_argument("--zone", default="zone_lab_1", help="Имя зоны (используется если mesh_id не указан)")
    parser.add_argument("--mesh-id", default=None, help="Явное указание mesh_network_id")
    parser.add_argument("--node-id", default="test_node_001", help="Идентификатор узла")
    parser.add_argument("--node-type", default="ph", help="Тип узла (ph, ph_ec, climate, relay, water, display, root)")
    parser.add_argument("--root-id", default="root_demo", help="Идентификатор Root Node")
    parser.add_argument("--firmware", default="2.0.0", help="Версия прошивки (для discovery)")
    parser.add_argument("--hardware", default="ESP32", help="Аппаратная платформа (для discovery)")
    parser.add_argument("--mac", default="AA:BB:CC:DD:EE:FF", help="MAC-адрес узла (для discovery)")
    parser.add_argument("--uptime", type=int, default=123456, help="Uptime в секундах (для heartbeat)")
    parser.add_argument("--command-name", default="request_status", help="Имя команды (для command режима)")
    parser.add_argument(
        "--command-payload",
        default='{"scope": "status"}',
        help="Параметры команды в формате JSON (для command режима)",
    )
    parser.add_argument("--topic", "-t", default=None, help="Произвольный топик (для custom режима)")
    parser.add_argument("--payload", default=None, help="Произвольный JSON payload (для custom режима)")

    args = parser.parse_args()
    test_mqtt(args)

