# MQTT Подключение ESP32 - Устранение неполадок

## 🔍 Проблема

ESP32 не может подключиться к MQTT брокеру:
```
E (493251) esp-tls: [sock=54] select() timeout
E (493252) transport_base: Failed to open a new connection: 32774
E (493252) mqtt_client: Error transport connect
W (493266) mqtt_manager: MQTT disconnected from broker
```

## ✅ Проверка состояния

1. **MQTT брокер работает** ✓
   - Docker контейнер `hydro_mosquitto_dev` запущен
   - Порт 1883 слушает на `0.0.0.0:1883`

2. **Порт проброшен** ✓
   - Docker порт mapping: `1883:1883`

3. **IP адрес настроен** ✓
   - В конфигурации ESP32: `192.168.1.100:1883`
   - IP адрес есть на хосте (Ethernet интерфейс)

4. **Проблема: Подключение не работает** ✗
   - TCP подключение к `192.168.1.100:1883` не устанавливается
   - Ping на `192.168.1.100` не проходит

## 🔧 Решения

### Вариант 1: Проверить Windows Firewall

**Проблема:** Windows Firewall может блокировать входящие подключения на порт 1883.

**Решение:**

1. Откройте **Windows Defender Firewall с дополнительными параметрами**
2. Нажмите **Правила для входящих подключений** → **Создать правило**
3. Выберите:
   - Тип правила: **Порт**
   - Протокол: **TCP**
   - Порты: **1883**
   - Действие: **Разрешить подключение**
   - Профили: Все (Доменная, Частная, Публичная)
   - Имя: **MQTT Broker (Port 1883)**

4. Сохраните правило

**Проверка:**
```powershell
# Проверка правила
Get-NetFirewallRule | Where-Object {$_.DisplayName -like "*MQTT*"} | Select-Object DisplayName, Enabled
```

### Вариант 2: Проверить правильность IP адреса

**Проблема:** ESP32 может быть в другой сети, чем хост.

**Решение:**

1. Узнайте IP адрес вашего компьютера в той же сети, что и ESP32:
   ```powershell
   ipconfig
   ```

2. Если ESP32 подключен к WiFi роутеру "Yorick":
   - Проверьте IP адрес интерфейса, который подключен к той же сети
   - Возможно, это не `192.168.1.100`, а другой IP (например, `192.168.1.XXX`)

3. Обновите конфигурацию ESP32 в `common/mesh_config/mesh_config.h`:
   ```c
   #define MQTT_BROKER_HOST        "192.168.1.XXX"  // Ваш реальный IP
   #define MQTT_BROKER_URI         "mqtt://192.168.1.XXX:1883"
   ```

4. **Пересоберите и прошейте ESP32:**
   ```bash
   cd root_node
   idf.py build flash
   ```

### Вариант 3: Проверить сетевую доступность

**Проблема:** Интерфейс с IP `192.168.1.100` может быть неактивен или отключен.

**Решение:**

1. Проверьте активные сетевые интерфейсы:
   ```powershell
   Get-NetAdapter | Where-Object {$_.Status -eq "Up"}
   ```

2. Проверьте IP адреса активных интерфейсов:
   ```powershell
   Get-NetIPAddress -AddressFamily IPv4 | Where-Object {$_.InterfaceAlias -match "Ethernet|Wi-Fi"}
   ```

3. Если интерфейс с `192.168.1.100` неактивен:
   - Включите интерфейс в настройках Windows
   - Или используйте IP адрес активного интерфейса

### Вариант 4: Использовать hostname вместо IP

**Если ваш компьютер имеет статический hostname в сети:**

1. Узнайте hostname:
   ```powershell
   hostname
   ```

2. Обновите конфигурацию ESP32 (если DNS работает в вашей сети):
   ```c
   #define MQTT_BROKER_HOST        "YOUR-HOSTNAME"  // Например, "DESKTOP-ABC123"
   #define MQTT_BROKER_URI         "mqtt://YOUR-HOSTNAME:1883"
   ```

3. **Пересоберите и прошейте ESP32**

## 📝 Быстрая проверка

### Шаг 1: Проверка порта из другой сети

Если у вас есть другой компьютер/телефон в той же сети:
```bash
# Linux/Mac
telnet 192.168.1.100 1883

# Или
nc -zv 192.168.1.100 1883
```

Если подключение не устанавливается - проблема в firewall или сетевых настройках.

### Шаг 2: Проверка из самого хоста

```powershell
# Тест подключения на localhost
$client = New-Object System.Net.Sockets.TcpClient
try {
    $client.Connect("127.0.0.1", 1883)
    Write-Host "✓ Localhost connection works"
    $client.Close()
} catch {
    Write-Host "✗ Localhost connection failed"
}
```

Если localhost работает, но внешний IP нет - проблема в firewall.

### Шаг 3: Проверка логов MQTT

```powershell
# Смотрим логи mosquitto
docker logs --tail 50 hydro_mosquitto_dev
```

Если вы видите подключения только с `127.0.0.1` - внешние подключения не доходят до брокера.

## ✅ После исправления

1. Пересоберите ESP32:
   ```bash
   cd root_node
   idf.py build flash
   ```

2. Проверьте подключение:
   - В логах ESP32 должно появиться: `I (xxxxx) mqtt_manager: MQTT connected`
   - В логах mosquitto должно появиться: `New connection from 192.168.1.XXX:xxxxx`

3. Проверьте получение heartbeat:
   - В логах mqtt_listener должно появиться: `💓 [HEARTBEAT] hydro/heartbeat/root_node_001`

## 🆘 Если ничего не помогает

1. Проверьте, что ESP32 и хост в одной сети (подключены к одному роутеру)
2. Проверьте, что роутер не блокирует подключения между устройствами
3. Попробуйте использовать другой IP адрес хоста
4. Проверьте, что порт 1883 не используется другим приложением

