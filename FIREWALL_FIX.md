# 🔥 Исправление Windows Firewall для MQTT

## ❌ Проблема

ESP32 ROOT узел не может подключиться к MQTT брокеру:
```
E (12471) esp-tls: [sock=54] select() timeout
E (12472) transport_base: Failed to open a new connection
```

**Причина:** Windows Firewall блокирует входящие подключения на порт 1883

---

## ✅ Решение

### Вариант 1: PowerShell (от администратора)

Откройте **PowerShell от имени администратора** и выполните:

```powershell
New-NetFirewallRule -DisplayName "MQTT Broker 1883" -Direction Inbound -Protocol TCP -LocalPort 1883 -Action Allow
```

### Вариант 2: Командная строка (от администратора)

Откройте **cmd от имени администратора** и выполните:

```cmd
netsh advfirewall firewall add rule name="MQTT Broker 1883" dir=in action=allow protocol=TCP localport=1883
```

### Вариант 3: Графический интерфейс

1. Откройте **Панель управления** → **Система и безопасность** → **Брандмауэр Windows**
2. Нажмите **Дополнительные параметры** (слева)
3. Выберите **Правила для входящих подключений**
4. Нажмите **Создать правило** (справа)
5. Выберите **Для порта** → **Далее**
6. Выберите **TCP** и укажите порт **1883** → **Далее**
7. Выберите **Разрешить подключение** → **Далее**
8. Оставьте все профили отмеченными → **Далее**
9. Имя: `MQTT Broker 1883` → **Готово**

---

## 🧪 Проверка после добавления правила

### 1. Проверить правило файрволла:
```powershell
Get-NetFirewallRule -DisplayName "MQTT*" | Format-Table DisplayName, Enabled, Direction, Action
```

### 2. Проверить доступность порта:
```powershell
Test-NetConnection -ComputerName 192.168.0.167 -Port 1883
```

Должно быть: `TcpTestSucceeded : True`

### 3. Перезагрузить ESP32:
```bash
# Нажмите кнопку RESET на плате или:
idf.py monitor
# Нажмите Ctrl+]
# Нажмите EN кнопку на ESP32
```

---

## 📊 Ожидаемый результат

После добавления правила в логах ESP32 должно появиться:

```
I (2467) mqtt_manager: Connecting to broker: mqtt://192.168.0.167:1883
I (2500) mqtt_manager: ✅ MQTT connected to broker
I (2505) mqtt_manager: ✅ Subscribed to hydro/command/#
I (2510) mqtt_manager: ✅ Subscribed to hydro/config/#
```

---

## 🔍 Дополнительная диагностика

### Проверить логи MQTT брокера:
```bash
docker logs hydro_mosquitto --tail 20 -f
```

После подключения ESP32 должны появиться строки:
```
New connection from 192.168.0.104:XXXXX on port 1883
New client connected from 192.168.0.104 as root_001
```

### Проверить логи MQTT Listener:
```bash
docker logs hydro_mqtt_listener -f
```

Должно появиться:
```
📡 New message from hydro/discovery
✅ Node registered: root_001
```

---

## ⚠️ Если проблема остается

### 1. Проверить сетевую связность:
На ESP32 добавьте ping теста к хосту (если нужно).

### 2. Проверить Docker маппинг портов:
```bash
docker port hydro_mosquitto
```

Должно быть:
```
1883/tcp -> 0.0.0.0:1883
```

### 3. Временно отключить файрволл (для теста):
```powershell
# ВНИМАНИЕ: Это небезопасно! Только для теста!
Set-NetFirewallProfile -Profile Domain,Public,Private -Enabled False

# После теста ОБЯЗАТЕЛЬНО включить обратно:
Set-NetFirewallProfile -Profile Domain,Public,Private -Enabled True
```

Если после отключения файрволла ESP32 подключается - проблема точно в файрволле.

---

## 📝 Альтернативное решение

Если не хотите открывать порт 1883 полностью, можно разрешить только для локальной сети:

```powershell
New-NetFirewallRule -DisplayName "MQTT Local Network" `
    -Direction Inbound `
    -Protocol TCP `
    -LocalPort 1883 `
    -Action Allow `
    -RemoteAddress 192.168.0.0/24
```

Это разрешит подключения только с устройств из сети `192.168.0.*`

---

## ✅ Итоговый чеклист

- [ ] Добавить правило файрволла для порта 1883
- [ ] Проверить доступность порта с хоста
- [ ] Перезагрузить ESP32 ROOT узел
- [ ] Проверить логи подключения
- [ ] Проверить появление узла в веб-интерфейсе

---

**После выполнения:** ESP32 должен успешно подключиться к MQTT брокеру! 🎉

