# 🔧 Исправление MQTT подключения

**Дата:** 22 октября 2025  
**Проблема:** ROOT узел не мог подключиться к MQTT брокеру

---

## ❌ Проблема

### Симптомы:
```
E (12479) esp-tls: [sock=54] select() timeout
E (12480) transport_base: Failed to open a new connection: 32774
E (12480) mqtt_client: Error transport connect
W (12494) mqtt_manager: MQTT disconnected from broker
```

### Причина:
ROOT узел пытался подключиться к **старому IP адресу** MQTT брокера:

| Параметр | Значение |
|----------|----------|
| ❌ Старый адрес | `192.168.1.100:1883` |
| ✅ Текущий адрес | `192.168.0.167:1883` |
| 📡 ROOT узел IP | `192.168.0.104` |

### Логи показали:
```
I (2467) mqtt_manager: Connecting to broker: mqtt://192.168.1.100:1883  ❌
I (2554) ROOT: MQTT Broker: mqtt://192.168.0.167:1883                   ✅
```

**Несоответствие:** В разных местах кода использовались разные IP адреса!

---

## ✅ Решение

### 1. Найден источник проблемы
Файл: `root_node/components/mqtt_client/mqtt_client_manager.c`

**Было:**
```c
// MQTT конфигурация (TODO: переместить в sdkconfig)
#define MQTT_BROKER_URI         "mqtt://192.168.1.100:1883"  // ❌ Захардкожен старый IP
```

### 2. Исправлено
**Стало:**
```c
#include "mesh_config.h"  // ✅ Добавлен include

// MQTT конфигурация берётся из mesh_config.h
// MQTT_BROKER_URI уже определён в mesh_config.h: "mqtt://192.168.0.167:1883"
// (удалён дублирующий define)
```

### 3. Централизованная конфигурация
Файл: `common/mesh_config/mesh_config.h`

```c
/**
 * @brief IP адрес или hostname MQTT брокера
 */
#define MQTT_BROKER_HOST        "192.168.0.167"

/**
 * @brief Порт MQTT брокера
 */
#define MQTT_BROKER_PORT        1883

/**
 * @brief Полный URI MQTT брокера
 */
#define MQTT_BROKER_URI         "mqtt://192.168.0.167:1883"
```

**Преимущества:**
- ✅ Единая точка конфигурации
- ✅ Все узлы используют одинаковый адрес
- ✅ Легко изменить для всех узлов сразу

---

## 📝 Дополнительные исправления

### Обновлена документация:
- `root_node/README.md` - все примеры обновлены на `192.168.0.167`
- Примеры команд mosquitto_sub/mosquitto_pub

### Проверка конфигурации MQTT брокера:
```yaml
# server/mosquitto/config/mosquitto.conf
listener 1883 0.0.0.0        ✅ Слушает все интерфейсы
allow_anonymous true          ✅ Анонимный доступ разрешён
```

### Проверка доступности:
```powershell
Test-NetConnection -ComputerName 192.168.0.167 -Port 1883
# TcpTestSucceeded: True ✅
```

---

## 🚀 Действия после исправления

### 1. Пересборка прошивки
```bash
cd root_node
idf.py build
```

### 2. Прошивка устройства
```bash
idf.py flash monitor
```

### 3. Ожидаемый результат
```
I (2467) mqtt_manager: Connecting to broker: mqtt://192.168.0.167:1883
I (2500) mqtt_manager: ✅ MQTT connected to broker
I (2505) mqtt_manager: ✅ Subscribed to hydro/command/#
I (2510) mqtt_manager: ✅ Subscribed to hydro/config/#
```

---

## 📊 Текущая сетевая конфигурация

| Устройство | IP адрес | Роль |
|------------|----------|------|
| Хост (Docker) | `192.168.0.167` | MQTT Broker, Backend, Frontend |
| ROOT узел | `192.168.0.104` | ESP32-S3 ROOT узел |
| Gateway | `192.168.0.1` | Роутер |
| Subnet | `255.255.255.0` | Локальная сеть |

### Доступные сервисы:
```
MQTT Broker:  mqtt://192.168.0.167:1883  ✅
Backend API:  http://192.168.0.167:8000  ✅
Frontend:     http://192.168.0.167:3000  ✅
WebSocket:    ws://192.168.0.167:8080    ✅
```

---

## ⚠️ Важно на будущее

### При изменении IP адреса хоста:

**1. Обновить только один файл:**
```c
// common/mesh_config/mesh_config.h
#define MQTT_BROKER_HOST        "НОВЫЙ_IP"
#define MQTT_BROKER_URI         "mqtt://НОВЫЙ_IP:1883"
```

**2. Пересобрать ВСЕ узлы:**
```bash
cd root_node && idf.py build
cd node_climate && idf.py build
cd node_ph_ec && idf.py build
# и т.д.
```

**3. Прошить все узлы:**
```bash
idf.py flash
```

### НЕ захардкоживать IP адреса в коде!
❌ **Плохо:**
```c
#define MQTT_BROKER_URI "mqtt://192.168.1.100:1883"  // В каждом файле
```

✅ **Хорошо:**
```c
#include "mesh_config.h"  // Использовать MQTT_BROKER_URI из общего конфига
```

---

## 🔍 Как найти IP адрес хоста

### Windows:
```bash
ipconfig | findstr IPv4
```

### Linux/Mac:
```bash
ifconfig | grep "inet "
hostname -I
```

### В Docker контейнере:
```bash
docker exec hydro_backend hostname -I
```

---

## ✅ Проверка работы после исправления

### 1. Проверить логи ROOT узла:
```bash
idf.py monitor
```

Должно быть:
```
✅ MQTT connected to broker
✅ Subscribed to hydro/command/#
```

### 2. Проверить логи MQTT брокера:
```bash
docker logs hydro_mqtt_listener -f
```

Должно появиться:
```
📡 New message from hydro/discovery
✅ Node registered: root_001
```

### 3. Проверить в веб-интерфейсе:
```
http://192.168.0.167:3000
```

Узел должен появиться в списке и быть **Online** ✅

---

**Статус:** ✅ **ИСПРАВЛЕНО**  
**Прошивка:** 🔄 Пересборка в процессе...

