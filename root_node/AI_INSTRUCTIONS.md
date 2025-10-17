# 🤖 AI INSTRUCTIONS - ROOT NODE

## 🎯 Назначение узла

**ROOT NODE (ESP32-S3 #1)** - координатор mesh-сети и MQTT мост

### Основные функции:
- ⭐ Координатор ESP-WIFI-MESH (главный узел)
- 🌐 MQTT клиент (связь с сервером)
- 📋 Node Registry (учет всех подключенных узлов)
- 🔀 Data Router (маршрутизация NODE ↔ MQTT)
- 🌡️ Резервная логика климата (если Climate node offline)
- 🔄 OTA координатор (раздача прошивок другим узлам)

### ⚠️ КРИТИЧНО:
- ❌ **НЕТ ДАТЧИКОВ!** ROOT не имеет физических датчиков
- ❌ **НЕТ ДИСПЛЕЯ!** ROOT работает "headless"
- ❌ **НЕТ ИСПОЛНИТЕЛЕЙ!** ROOT только координирует
- ✅ Должен быть **МАКСИМАЛЬНО СТАБИЛЬНЫМ** - от него зависит вся сеть

---

## 🏗️ Архитектура

```
ROOT NODE (ESP32-S3 #1)
│
├── Mesh Manager (ROOT режим)
│   ├── WiFi STA → Router (для MQTT)
│   ├── Mesh AP → Узлы подключаются сюда
│   └── Роутинг между узлами
│
├── MQTT Client
│   ├── Публикация: telemetry, events
│   ├── Подписка: commands, config
│   └── Keepalive
│
├── Node Registry
│   ├── Список всех узлов (MAC, ID, тип)
│   ├── Статус (online/offline)
│   ├── Последние данные
│   └── Проверка таймаутов
│
├── Data Router
│   ├── NODE → ROOT → MQTT → Server
│   ├── Server → MQTT → ROOT → NODE
│   └── Display запросы
│
└── Climate Logic (резервная)
    ├── Если Climate node offline > 30 сек
    ├── Fallback таймеры (форточки, вент)
    └── Отправка команд на Relay node
```

---

## 📦 Компоненты

### Обязательные (common):
- `mesh_manager` - ESP-WIFI-MESH управление
- `mesh_protocol` - JSON протокол
- `node_config` - NVS конфигурация

### Специфичные (components):
- `node_registry` - Реестр узлов
- `mqtt_client` - MQTT клиент
- `data_router` - Маршрутизация данных
- `climate_logic` - Резервная логика
- `ota_coordinator` - OTA координатор (TODO)

---

## 🔌 Распиновка

### GPIO:
**НЕТ GPIO ПОДКЛЮЧЕНИЙ!**

ROOT узел работает только с mesh и WiFi, без физических датчиков.

### Индикация (опционально):
Можно добавить:
- GPIO 15 - LED статус (опционально)
- GPIO 16 - LED mesh activity (опционально)

---

## ⚙️ Конфигурация

### sdkconfig.defaults

```ini
# WiFi Router
CONFIG_WIFI_SSID="MyWiFi"
CONFIG_WIFI_PASSWORD="MyWiFiPassword"

# MQTT Broker
CONFIG_MQTT_BROKER_URI="mqtt://192.168.1.100:1883"

# Mesh
CONFIG_MESH_ENABLE=y
CONFIG_MESH_ROOT_ENABLE=y
```

### root_config.h

```c
// Mesh настройки
#define ROOT_MESH_ID "HYDRO1"
#define ROOT_MESH_PASSWORD "hydro_mesh_pass"
#define ROOT_MESH_CHANNEL 1
#define ROOT_MAX_CONNECTIONS 10

// MQTT топики
#define MQTT_TOPIC_TELEMETRY "hydro/telemetry"
#define MQTT_TOPIC_COMMAND "hydro/command"
#define MQTT_TOPIC_CONFIG "hydro/config"
#define MQTT_TOPIC_EVENT "hydro/event"

// Timeouts
#define HEARTBEAT_INTERVAL_MS 10000
#define NODE_TIMEOUT_MS 30000
```

---

## 💻 Примеры кода

### Инициализация

```c
void app_main(void) {
    ESP_LOGI(TAG, "=== ROOT NODE Starting ===");
    
    // 1. NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // 2. Node Registry
    node_registry_init();
    
    // 3. Mesh (ROOT режим)
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_ROOT,
        .mesh_id = ROOT_MESH_ID,
        .mesh_password = ROOT_MESH_PASSWORD,
        .channel = ROOT_MESH_CHANNEL,
        .max_connection = ROOT_MAX_CONNECTIONS,
        .router_ssid = CONFIG_WIFI_SSID,
        .router_password = CONFIG_WIFI_PASSWORD
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    
    // 4. MQTT Client
    mqtt_client_manager_init();
    
    // 5. Data Router
    data_router_init();
    
    // 6. Climate Logic
    climate_logic_init();
    
    // 7. Запуск mesh
    ESP_ERROR_CHECK(mesh_manager_start());
    
    ESP_LOGI(TAG, "=== ROOT NODE Running ===");
}
```

### Обработка данных от NODE

```c
void data_router_handle_mesh_data(const uint8_t *src_addr, const uint8_t *data, size_t len) {
    // Парсинг JSON
    mesh_message_t msg;
    if (!mesh_protocol_parse((char*)data, &msg)) {
        return;
    }
    
    // Обновление реестра
    node_registry_update_last_seen(msg.node_id);
    
    // Маршрутизация
    switch (msg.type) {
        case MESH_MSG_TELEMETRY:
            // NODE → MQTT
            mqtt_client_manager_publish(MQTT_TOPIC_TELEMETRY, (char*)data);
            break;
            
        case MESH_MSG_EVENT:
            // NODE → MQTT (события)
            mqtt_client_manager_publish(MQTT_TOPIC_EVENT, (char*)data);
            
            // Если критичное - особая обработка
            if (is_critical_event(&msg)) {
                handle_critical_event(&msg);
            }
            break;
            
        case MESH_MSG_HEARTBEAT:
            // Только обновить реестр, не отправлять в MQTT
            break;
    }
    
    mesh_protocol_free_message(&msg);
}
```

### Обработка команд от MQTT

```c
void data_router_handle_mqtt_data(const char *topic, const char *data, int data_len) {
    // Парсинг топика: hydro/command/{node_id}
    char node_id[32];
    if (sscanf(topic, "hydro/command/%s", node_id) == 1) {
        // Найти узел в реестре
        node_info_t *node = node_registry_get(node_id);
        if (node && node->online) {
            // Отправить команду на NODE
            mesh_manager_send(node->mac_addr, (uint8_t*)data, data_len);
        } else {
            ESP_LOGW(TAG, "Node %s offline, command dropped", node_id);
        }
    }
}
```

### Проверка таймаутов узлов

```c
void node_registry_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));  // Каждые 5 сек
        
        node_registry_check_timeouts();
        
        // Проверка Climate node
        node_info_t *climate = node_registry_get("climate_001");
        if (!climate || !climate->online) {
            ESP_LOGW(TAG, "Climate node offline, using fallback logic");
            climate_logic_enable_fallback();
        }
    }
}
```

---

## 🚫 Что НЕЛЬЗЯ делать

1. ❌ **Не добавляй датчики на ROOT**
   - ROOT должен быть простым и стабильным
   - Все датчики - на NODE узлах

2. ❌ **Не делай тяжелых вычислений**
   - PID контроль - на NODE pH/EC
   - Аналитика - на сервере
   - ROOT только маршрутизирует

3. ❌ **Не блокируй главный поток**
   - Mesh обработка должна быть быстрой
   - Долгие операции - в отдельные задачи

4. ❌ **Не игнорируй ошибки mesh**
   - Логировать все проблемы
   - Переподключаться при необходимости

---

## ✅ Что НУЖНО делать

1. ✅ **Мониторинг всех узлов**
   - Heartbeat каждые 10 сек
   - Таймаут 30 сек → offline
   - Логирование изменений статуса

2. ✅ **Быстрая маршрутизация**
   - JSON парсинг < 10 мс
   - Отправка в MQTT без задержек
   - Буферизация если MQTT offline

3. ✅ **Резервная логика**
   - Если Climate offline → fallback
   - Периодические команды Relay node
   - Логирование fallback режима

4. ✅ **Watchdog**
   - Периодически сбрасывать watchdog
   - Перезагрузка при зависании

---

## 🧪 Тестирование

### Базовый тест mesh

```bash
cd root_node
idf.py set-target esp32s3
idf.py build
idf.py -p COM3 flash monitor
```

**Ожидаемый вывод:**
```
I ROOT: === ROOT NODE Starting ===
I mesh_manager: ROOT mode: connecting to router MyWiFi
I mesh_manager: ROOT got IP address
I mqtt_manager: MQTT connected
I ROOT: === ROOT NODE Running ===
I ROOT: Mesh nodes: 0, Registry: 0
```

### Тест с NODE

Прошей любой NODE (например node_climate):
```bash
cd ../node_climate
idf.py build flash
```

**В логах ROOT должно появиться:**
```
I mesh_manager: Child connected: XX:XX:XX:XX:XX:XX
I data_router: Mesh data received (256 bytes)
I node_registry: Node added: climate_001 (type: climate)
I mqtt_manager: Published to hydro/telemetry
```

### Тест MQTT

```bash
# В другом терминале
python tools/mqtt_tester.py -b 192.168.1.100

# Должны увидеть telemetry от узлов
```

### Тест команд

```bash
# Отправить команду через MQTT
mosquitto_pub -h 192.168.1.100 -t "hydro/command/climate_001" -m '{"type":"command","command":"restart"}'

# В логах ROOT:
I data_router: MQTT data received: hydro/command/climate_001
I data_router: Forwarding command to node climate_001
```

---

## 📊 Мониторинг

### Важные метрики

1. **Количество узлов:**
   ```c
   int total = mesh_manager_get_total_nodes();
   int online = node_registry_get_count();
   ESP_LOGI(TAG, "Mesh: %d nodes, Registry: %d online", total, online);
   ```

2. **MQTT статус:**
   ```c
   if (!mqtt_client_manager_is_connected()) {
       ESP_LOGW(TAG, "MQTT disconnected!");
   }
   ```

3. **Heap memory:**
   ```c
   uint32_t free_heap = esp_get_free_heap_size();
   ESP_LOGI(TAG, "Free heap: %d bytes", free_heap);
   ```

### Логирование

```c
// Уровни логов
ESP_LOGE(TAG, "Error: ...");      // Ошибки
ESP_LOGW(TAG, "Warning: ...");    // Предупреждения
ESP_LOGI(TAG, "Info: ...");       // Информация
ESP_LOGD(TAG, "Debug: ...");      // Отладка
```

---

## 🔧 Troubleshooting

### Проблема: Mesh не запускается

**Решение:**
1. Проверь WiFi SSID/Password в `sdkconfig`
2. Проверь, что роутер на канале 1-11 (не 12-13)
3. `idf.py menuconfig` → Component config → ESP32-specific → WiFi

### Проблема: MQTT не подключается

**Решение:**
1. Проверь IP broker в `sdkconfig`
2. Проверь, что Mosquitto запущен: `systemctl status mosquitto`
3. Тест: `mosquitto_pub -h IP -t test -m hello`

### Проблема: Узлы не подключаются

**Решение:**
1. Проверь, что Mesh ID одинаковый на всех узлах
2. Проверь, что узлы в режиме NODE (не ROOT)
3. Увеличь `ROOT_MAX_CONNECTIONS` если узлов много

### Проблема: Высокое потребление памяти

**Решение:**
1. Уменьши размер буферов
2. Освобождай cJSON после использования: `cJSON_Delete(obj)`
3. Мониторь: `esp_get_free_heap_size()`

---

## 📚 Полезные ссылки

- [ESP-WIFI-MESH документация](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/mesh.html)
- [MQTT esp-mqtt](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html)
- `common/USAGE.md` - Использование common компонентов
- `doc/MESH_HYDRO_V2_FINAL_PLAN.md` - Общий план системы

---

## 🎯 Чек-лист разработки

- [ ] Mesh инициализация (ROOT режим)
- [ ] MQTT подключение и keepalive
- [ ] Node Registry (add, update, timeout)
- [ ] Data Router (NODE → MQTT, MQTT → NODE)
- [ ] Climate fallback логика
- [ ] Heartbeat обработка
- [ ] Display запросы (request/response)
- [ ] OTA coordinator (TODO в Фазе 10)
- [ ] Watchdog timer
- [ ] Тестирование с реальными NODE узлами
- [ ] Стресс-тест (10+ узлов, 24 часа)

**ROOT узел - сердце системы. Делай его максимально надежным!** 🏆

