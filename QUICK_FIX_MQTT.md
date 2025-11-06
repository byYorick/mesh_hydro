# 🚀 Быстрое исправление MQTT подключения

## Проблема
ESP32 не может подключиться к MQTT брокеру:
```
E (xxxxx) esp-tls: [sock=54] select() timeout
E (xxxxx) transport_base: Failed to open a new connection: 32774
E (xxxxx) mqtt_client: Error transport connect
W (xxxxx) mqtt_manager: MQTT disconnected from broker
```

## ✅ Решение за 2 минуты

### Шаг 1: Добавить правило Firewall

**Откройте PowerShell от имени администратора** (Win+X → Windows PowerShell (Администратор))

Выполните команду:
```powershell
New-NetFirewallRule -DisplayName "MQTT Broker (Port 1883)" -Direction Inbound -LocalPort 1883 -Protocol TCP -Action Allow
```

Или запустите скрипт `fix_mqtt_firewall.ps1`:
```powershell
.\fix_mqtt_firewall.ps1
```

### Шаг 2: Проверка

После добавления правила проверьте:
```powershell
Test-NetConnection -ComputerName 192.168.1.100 -Port 1883
```

Должно показать: `TcpTestSucceeded : True`

### Шаг 3: Перезагрузить ESP32

После добавления правила перезагрузите ESP32 узлы.

### Шаг 4: Проверка подключения

В логах mosquitto должно появиться:
```bash
docker logs -f hydro_mosquitto
```

Должно появиться:
```
New connection from 192.168.1.XXX:xxxxx on port 1883
New client connected from 192.168.1.XXX:xxxxx as ...
```

## 🔍 Если не помогло

### Проверка 1: Правильный IP адрес

Убедитесь, что IP `192.168.1.100` правильный:
```powershell
ipconfig
```

Найдите IP адрес интерфейса, который подключен к той же сети, что и ESP32.

### Проверка 2: Обновление конфигурации ESP32

Если IP другой, обновите в `common/mesh_config/mesh_config.h`:
```c
#define MQTT_BROKER_HOST        "192.168.1.XXX"  // Ваш реальный IP
#define MQTT_BROKER_URI         "mqtt://192.168.1.XXX:1883"
```

Затем пересоберите и прошейте ESP32:
```bash
cd root_node
idf.py build flash
```

### Проверка 3: Сетевая доступность

Убедитесь, что ESP32 и хост в одной сети:
- ESP32 подключен к WiFi роутеру "Yorick"
- Хост подключен к той же сети
- Оба устройства в сети `192.168.1.0/24`

