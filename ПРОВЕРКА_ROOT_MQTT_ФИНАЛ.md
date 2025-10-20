# 🔧 ФИНАЛЬНАЯ ПРОВЕРКА ROOT MQTT

## ✅ Что исправлено

1. **BROADCAST_DRIVER** изменен с `pusher` на `log` 
2. **Discovery обработка** теперь работает корректно
3. **Тестовое сообщение** успешно создает узел в БД

## ⚠️ Текущая проблема

**ROOT узел НЕ подключается к Mosquitto физически!**

### Симптомы:
- ✅ ROOT узел говорит: `I mqtt_manager: MQTT connected to broker`
- ✅ ROOT узел говорит: `I mqtt_manager: Published discovery message (msg_id=65240, len=304)`
- ❌ **Mosquitto НЕ видит подключений от 192.168.1.191**
- ❌ **Discovery сообщения НЕ доходят до брокера**

## 🔍 Диагностика

### 1. Проверить логи Mosquitto

```powershell
# Смотрим последние 50 строк
docker logs hydro_mosquitto --tail 50

# Ищем подключения от ESP32 (192.168.x.x)
docker logs hydro_mosquitto | Select-String "192.168"
```

**Ожидаемый результат:** Должны быть строки вида:
```
1760978xxx: New connection from 192.168.1.191:xxxxx on port 1883.
1760978xxx: New client connected from 192.168.1.191:xxxxx as ESP32_CLIENT_ID
```

**Текущий результат:** ❌ Таких строк НЕТ!

### 2. Проверить доступность порта с хоста

```powershell
Test-NetConnection -ComputerName 192.168.1.100 -Port 1883
```

**Ожидаемый результат:** `TcpTestSucceeded : True`

### 3. Проверить IP адрес хост-машины

```powershell
ipconfig | Select-String "192.168"
```

Убедись, что IP адрес хоста **действительно** `192.168.1.100`.

### 4. Проверить настройки ROOT узла

**Файл:** `common/mesh_config/mesh_config.h`

```c
#define MQTT_BROKER_HOST        "192.168.1.100"  // <-- Должен совпадать с IP хоста!
#define MQTT_BROKER_URI         "mqtt://192.168.1.100:1883"
```

## 🛠️ Возможные решения

### Решение 1: Проверить IP адрес

1. Узнать реальный IP хоста:
   ```powershell
   ipconfig
   ```

2. Обновить `common/mesh_config/mesh_config.h`:
   ```c
   #define MQTT_BROKER_HOST        "192.168.1.XXX"  // <-- ТВОЙ IP
   #define MQTT_BROKER_URI         "mqtt://192.168.1.XXX:1883"
   ```

3. Пересобрать и прошить ROOT узел:
   ```powershell
   cd root_node
   idf.py build
   idf.py -p COM7 flash monitor
   ```

### Решение 2: Использовать имя хоста

Если IP меняется, можно использовать имя хоста:

```c
#define MQTT_BROKER_HOST        "DESKTOP-XXXXX.local"  // Имя твоего ПК
#define MQTT_BROKER_URI         "mqtt://DESKTOP-XXXXX.local:1883"
```

### Решение 3: Проверить Windows Firewall

Добавить правило для MQTT (если не было):

```powershell
# От имени администратора
netsh advfirewall firewall add rule name="MQTT Inbound 1883" dir=in action=allow protocol=TCP localport=1883
netsh advfirewall firewall add rule name="MQTT Outbound 1883" dir=out action=allow protocol=TCP localport=1883
```

### Решение 4: Проверить антивирус

Некоторые антивирусы блокируют MQTT трафик. Временно отключи антивирус и проверь.

## ✅ Как проверить что заработало

### 1. Мониторинг ROOT узла

```powershell
cd root_node
idf.py -p COM7 monitor
```

**Должно быть в логах:**
```
I mqtt_manager: MQTT connected to broker
I mqtt_manager: Published discovery message (msg_id=XXXXX, len=304)
```

### 2. Мониторинг Mosquitto (в другом терминале)

```powershell
docker logs -f hydro_mosquitto
```

**Должно появиться:**
```
1760978xxx: New connection from 192.168.1.191:xxxxx on port 1883.
1760978xxx: New client connected from 192.168.1.191:xxxxx as ESP32_...
1760978xxx: Received PUBLISH from ESP32_... (d0, q0, r0, m0, 'hydro/discovery', ... (304 bytes))
```

### 3. Проверка MQTT Listener

```powershell
docker logs -f hydro_mqtt_listener
```

**Должно появиться:**
```
🔍 [DISCOVERY] hydro/discovery
```

### 4. Проверка базы данных

```powershell
docker exec hydro_postgres psql -U hydro -d hydro_system `
  -c "SELECT node_id, node_type, online, mac_address FROM nodes WHERE node_type='root';"
```

**Должно появиться:**
```
      node_id       | node_type | online |    mac_address    
--------------------+-----------+--------+-------------------
 root_98a316f5fde8  | root      | t      | 98:A3:16:F5:FD:E8
```

## 🎯 Быстрый тест

Самый простой способ проверить - отправить MQTT сообщение **С ХОСТА**:

```powershell
# Установить mosquitto-clients если нет:
# https://mosquitto.org/download/

# Отправить тестовое сообщение
mosquitto_pub -h localhost -p 1883 -t "test/topic" -m "Hello from host"

# Подписаться на топик
mosquitto_sub -h localhost -p 1883 -t "test/#" -v
```

Если это работает, значит Mosquitto доступен с хоста. Проблема в ESP32.

Если НЕ работает, значит проблема в Mosquitto/Docker/Firewall.

## 📝 Итоговый чеклист

- [ ] Проверил IP адрес хоста (`ipconfig`)
- [ ] Обновил `mesh_config.h` с правильным IP
- [ ] Пересобрал и прошил ROOT узел
- [ ] Проверил Windows Firewall (добавил правила)
- [ ] Mosquitto видит подключение от ESP32 (192.168.1.191)
- [ ] MQTT Listener получает discovery сообщения
- [ ] ROOT узел появился в базе данных
- [ ] ROOT узел виден в веб-интерфейсе

---

**Следующий шаг:** После решения этой проблемы, ROOT узел должен автоматически регистрироваться и быть виден в системе! 🎉

