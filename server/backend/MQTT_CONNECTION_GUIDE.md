# 🔌 Руководство по подключению MQTT

## ✅ Текущий статус

### Root Node (ESP32)
- **IP адрес:** 192.168.1.191
- **MQTT брокер:** 192.168.1.100:1883
- **Статус:** ✅ ПОДКЛЮЧЕН
- **Подписки:**
  - `hydro/command/#`
  - `hydro/config/#`

### MQTT Broker (Mosquitto)
- **IP адрес:** 192.168.1.100:1883
- **Статус:** ✅ РАБОТАЕТ
- **Активные соединения:**
  - Root Node (192.168.1.191)
  - Backend (должен подключиться)

### Backend (Laravel)
- **MQTT Host:** 192.168.1.100 (обновлено)
- **MQTT Port:** 1883
- **Статус:** 🔄 Запускается...

## 🚀 Быстрый старт

### Вариант 1: Запуск всей системы одной командой

```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
START_ALL.bat
```

Это запустит:
1. Laravel Backend (http://localhost:8000)
2. MQTT Listener (подключится к 192.168.1.100:1883)

### Вариант 2: Запуск по отдельности

#### Terminal 1 - Backend
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
C:\PHP\php.exe artisan serve
```

#### Terminal 2 - MQTT Listener
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
C:\PHP\php.exe artisan mqtt:listen
```

#### Terminal 3 - Frontend
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\frontend
npm run dev
```

## 🔍 Проверка подключения

### 1. Проверить MQTT брокер
```cmd
simple-mqtt-check.bat
```

Должно показать:
```
[OK] MQTT Broker is RUNNING on port 1883
Active connections found
```

### 2. Проверить Backend MQTT Listener

В окне "MQTT Listener" должны появиться сообщения:
```
[INFO] Connected to MQTT broker: 192.168.1.100:1883
[INFO] Subscribed to: hydro/telemetry/+
[INFO] Subscribed to: hydro/status/+
[INFO] Subscribed to: hydro/events/+
```

### 3. Проверить Dashboard

Откройте: http://localhost:5173

В правом верхнем углу должно показать:
```
🟢 MQTT Broker Status: connected
```

## 📊 Топики MQTT

### Backend слушает:
- `hydro/telemetry/+` - телеметрия от узлов
- `hydro/status/+` - статус узлов
- `hydro/events/+` - события от узлов

### Backend отправляет:
- `hydro/command/{node_id}` - команды узлам
- `hydro/config/{node_id}` - конфигурация узлам

### Root Node слушает:
- `hydro/command/#` - все команды
- `hydro/config/#` - все настройки конфигурации

### Root Node отправляет:
- `hydro/telemetry/{node_id}` - данные телеметрии
- `hydro/status/{node_id}` - статус узлов
- `hydro/events/{node_id}` - события

## 🧪 Тестирование

### Тест 1: Отправить тестовое сообщение

Если установлен Mosquitto с инструментами:
```cmd
mosquitto_pub -h 192.168.1.100 -t hydro/telemetry/test -m "{\"test\":\"data\"}"
```

Backend MQTT Listener должен его получить.

### Тест 2: Прослушать сообщения от Root Node
```cmd
mosquitto_sub -h 192.168.1.100 -t hydro/#
```

Должны появиться сообщения от root_001:
```
hydro/telemetry/root_001 {"uptime": 12345, ...}
hydro/status/root_001 {"online": true}
```

### Тест 3: Отправить команду через веб-интерфейс

1. Откройте http://localhost:5173
2. Перейдите к узлу (например, root_001)
3. Нажмите "Команды" → Выберите команду → Отправить

Backend должен отправить команду в MQTT:
```
Topic: hydro/command/root_001
Payload: {"command": "reboot"}
```

Root Node должен её получить и выполнить.

## 🔧 Устранение неполадок

### Проблема: "MQTT Broker Status: disconnected"

**Решение 1:** Проверьте MQTT Listener
```cmd
# Проверьте, запущен ли процесс
tasklist | findstr php
```

Если не запущен:
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
C:\PHP\php.exe artisan mqtt:listen
```

**Решение 2:** Проверьте .env
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
findstr MQTT .env
```

Должно быть:
```
MQTT_HOST=192.168.1.100
MQTT_PORT=1883
```

Если не так:
```cmd
update-mqtt-host.bat
```

**Решение 3:** Проверьте firewall

Убедитесь, что порт 1883 открыт:
```cmd
netstat -an | findstr :1883
```

Должно показать `LISTENING`.

### Проблема: Root Node подключен, но данные не приходят

**Проверка 1:** Смотрим логи Root Node

Должны быть сообщения:
```
I (xxxx) mqtt_manager: Publishing to hydro/telemetry/root_001
```

**Проверка 2:** Слушаем MQTT напрямую
```cmd
mosquitto_sub -h 192.168.1.100 -t hydro/# -v
```

Если сообщения идут, но Backend не получает - перезапустите MQTT Listener.

**Проверка 3:** Проверьте топики

Backend подписан на `hydro/telemetry/+`, root отправляет в `hydro/telemetry/root_001` - должно совпадать.

### Проблема: Backend не может подключиться к MQTT

**Проверка 1:** Ping MQTT broker
```cmd
ping 192.168.1.100
```

**Проверка 2:** Telnet к порту
```cmd
telnet 192.168.1.100 1883
```

Должно подключиться (Ctrl+C для выхода).

**Проверка 3:** Проверьте PHP MQTT библиотеку
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
C:\PHP\php.exe -r "var_dump(class_exists('PhpMqtt\Client\MqttClient'));"
```

Должно вывести `bool(true)`. Если `false`:
```cmd
C:\PHP\php.exe C:\composer\composer.phar require php-mqtt/client
```

## 📝 Полезные команды

### Перезапуск MQTT Listener
```cmd
# Найти процесс
tasklist | findstr "php.*mqtt"

# Убить процесс
taskkill /F /IM php.exe /FI "WINDOWTITLE eq MQTT Listener*"

# Запустить заново
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
start "MQTT Listener" C:\PHP\php.exe artisan mqtt:listen
```

### Перезапуск Mosquitto
```cmd
net stop mosquitto
net start mosquitto
```

### Просмотр всех MQTT соединений
```cmd
netstat -an | findstr :1883
```

### Очистка кэша Laravel
```cmd
cd C:\esp\hydro\mesh\mesh_hydro\server\backend
C:\PHP\php.exe artisan cache:clear
C:\PHP\php.exe artisan config:clear
```

## 📚 Дополнительная информация

### Структура MQTT сообщений

#### Telemetry
```json
{
  "node_id": "root_001",
  "timestamp": 1697654400,
  "data": {
    "uptime": 86400,
    "heap_free": 192000,
    "wifi_rssi": -45
  }
}
```

#### Status
```json
{
  "node_id": "root_001",
  "online": true,
  "last_seen": "2025-10-18T12:00:00Z"
}
```

#### Command
```json
{
  "command": "reboot",
  "params": {},
  "timestamp": 1697654400
}
```

### QoS уровни

Backend использует QoS 0 (fire and forget) для большинства сообщений.

Для критичных команд можно использовать QoS 1 (at least once) или QoS 2 (exactly once).

### Keep-alive

- Backend: 60 секунд
- Root Node: по умолчанию (обычно 120 секунд)

## ✅ Контрольный список

- [ ] Mosquitto запущен на 192.168.1.100:1883
- [ ] Root Node подключен (192.168.1.191)
- [ ] Backend .env настроен (MQTT_HOST=192.168.1.100)
- [ ] Laravel Backend запущен (http://localhost:8000)
- [ ] MQTT Listener запущен (C:\PHP\php.exe artisan mqtt:listen)
- [ ] Frontend запущен (http://localhost:5173)
- [ ] Dashboard показывает "connected"
- [ ] Узлы отображаются на Dashboard
- [ ] Данные телеметрии обновляются

---

**Версия:** 1.0.0  
**Дата:** 18.10.2025  
**Автор:** AI Assistant

