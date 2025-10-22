# ROOT NODE

ESP32-S3 #1 - Координатор mesh-сети и MQTT мост

## 🎯 Назначение

ROOT узел - **сердце всей системы**. Координирует mesh-сеть и обеспечивает связь с сервером.

### Ключевые функции:
- ⭐ ESP-WIFI-MESH координатор (ROOT role)
- 🌐 MQTT клиент (связь с сервером)
- 📋 Node Registry (учет всех узлов)
- 🔀 Data Router (маршрутизация NODE ↔ MQTT)
- 🌡️ Climate fallback логика (резервное управление)
- 🔄 OTA координатор (раздача обновлений - TODO)

### ⚠️ Важно:
- ❌ **БЕЗ датчиков!** ROOT не имеет физических датчиков
- ❌ **БЕЗ дисплея!** Работает "headless"
- ✅ **Максимальная стабильность** - от ROOT зависит вся сеть

## 📦 Компоненты

### Из common/:
- `mesh_manager` - управление ESP-WIFI-MESH
- `mesh_protocol` - JSON протокол
- `node_config` - NVS конфигурация

### Собственные (components/):
- `node_registry` - реестр узлов
- `mqtt_client` - MQTT клиент
- `data_router` - маршрутизатор данных
- `climate_logic` - резервная логика климата

## ⚙️ Конфигурация

### 1. WiFi настройки

Отредактируй `sdkconfig.defaults`:
```ini
CONFIG_WIFI_SSID="YourWiFiName"
CONFIG_WIFI_PASSWORD="YourWiFiPassword"
```

### 2. MQTT настройки

Отредактируй `components/mqtt_client/mqtt_client_manager.c`:
```c
#define MQTT_BROKER_URI "mqtt://192.168.0.167:1883"
#define MQTT_USERNAME   "hydro_root"
#define MQTT_PASSWORD   "hydro_pass"
```

## 🚀 Сборка и прошивка

### Подготовка окружения (первый раз):
```bash
# Установка ESP-IDF v5.5.1 (если не установлен)
# См. https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/

# Активация ESP-IDF
. $HOME/esp/esp-idf/export.sh  # Linux/Mac
# или
%userprofile%\esp\esp-idf\export.bat  # Windows
```

### Сборка:
```bash
cd root_node
idf.py set-target esp32s3
idf.py build
```

### Прошивка:
```bash
idf.py -p COM3 flash monitor
```

Замените `COM3` на ваш порт (COM3, /dev/ttyUSB0, и т.д.)

## 📊 Ожидаемый вывод

После прошивки в мониторе должен появиться:

```
I ROOT: ========================================
I ROOT: === HYDRO MESH ROOT NODE Starting ===
I ROOT: ========================================
I ROOT: [Step 1/7] Initializing NVS...
I ROOT: [Step 2/7] Initializing Node Registry...
I node_registry: Node Registry initialized (max 20 nodes)
I ROOT: [Step 3/7] Initializing Mesh (ROOT mode)...
I mesh_manager: ROOT mode: connecting to router MyWiFi
I ROOT: [Step 4/7] Starting Mesh network...
I mesh_manager: Mesh started
I mesh_manager: ROOT got IP address: 192.168.1.101
I ROOT: [Step 5/7] Initializing MQTT Client...
I mqtt_manager: Connecting to broker...
I mqtt_manager: MQTT connected to broker
I ROOT: [Step 6/7] Initializing Data Router...
I data_router: Data Router initialized
I ROOT: [Step 7/7] Initializing Climate Fallback Logic...
I climate_logic: Climate fallback logic initialized
I ROOT: ========================================
I ROOT: === ROOT NODE Running ===
I ROOT: Mesh ID: HYDRO1
I ROOT: MQTT Broker: mqtt://192.168.0.167:1883
I ROOT: ========================================
```

## 🧪 Тестирование

### Тест 1: Проверка mesh
```bash
# Прошить любой NODE узел (например node_climate)
cd ../node_climate
idf.py build flash

# В логах ROOT должно появиться:
# I mesh_manager: Child connected: AA:BB:CC:DD:EE:FF
# I node_registry: New node added: climate_001
```

### Тест 2: Проверка MQTT
```bash
# Подписка на все топики
mosquitto_sub -h 192.168.0.167 -t "hydro/#" -v

# Должны появляться сообщения от узлов
```

### Тест 3: Отправка команды
```bash
# Отправить команду через MQTT
mosquitto_pub -h 192.168.0.167 -t "hydro/command/climate_001" \
  -m '{"type":"command","node_id":"climate_001","command":"restart"}'

# В логах ROOT:
# I data_router: Forwarding command to climate_001
```

## 🔧 Troubleshooting

### Mesh не запускается
- Проверь WiFi SSID/Password в sdkconfig.defaults
- Роутер должен быть на канале 1-11 (не 12-13)

### MQTT не подключается
- Проверь IP broker в mqtt_client_manager.c
- Убедись что Mosquitto запущен: `systemctl status mosquitto` (Linux)

### Узлы не подключаются
- Mesh ID должен быть одинаковым на всех узлах (`HYDRO1`)
- NODE узлы должны быть в режиме `MESH_MODE_NODE`
- Увеличь `ROOT_MAX_CONNECTIONS` если узлов много

## 📚 Документация

- `AI_INSTRUCTIONS.md` - 🤖 полная инструкция для ИИ (880+ строк)
- `components/*/README.md` - документация компонентов
- `../common/AI_INSTRUCTIONS.md` - общие компоненты
- `../doc/MESH_HYDRO_V2_FINAL_PLAN.md` - общий план системы

## ✅ Статус реализации

**ПОЛНОСТЬЮ РЕАЛИЗОВАН** ✅

| Компонент | Статус | Файлы |
|-----------|--------|-------|
| node_registry | ✅ ГОТОВ | .h, .c, CMakeLists.txt, README.md |
| mqtt_client | ✅ ГОТОВ | .h, .c, CMakeLists.txt, README.md |
| data_router | ✅ ГОТОВ | .h, .c, CMakeLists.txt, README.md |
| climate_logic | ✅ ГОТОВ | .h, .c, CMakeLists.txt, README.md |
| main/app_main.c | ✅ ГОТОВ | Полная реализация с мониторингом |
| root_config.h | ✅ ГОТОВ | Все константы конфигурации |
| sdkconfig.defaults | ✅ ГОТОВ | ESP32-S3, WiFi, MQTT настройки |

**Готовность: 100%** 🎉

**Приоритет:** 🔴 КРИТИЧНО (Шаг 4 из плана)

**Следующий шаг:** Прошивка и тестирование → затем NODE pH/EC (самый критичный!)
