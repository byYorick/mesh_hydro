# 🏭 Руководство по настройке зон

## Обзор

В системе Mesh Hydro каждая **зона** представляет собой независимую гидропонную установку со своей mesh-сетью ESP32 устройств. Каждая зона управляется **Root Node**, который служит шлюзом между mesh-сетью и сервером через MQTT.

## Архитектура зоны

```
┌─────────────────────────────────────────────────────────┐
│                    Зона "NFT-1"                         │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Root Node (root_nft_001)                              │
│  ├── Mesh Network ID: HYDRO_MESH_NFT_001               │
│  ├── MQTT Topic: hydro/nft1/                           │
│  └── Gateway: WiFi → MQTT → Server                     │
│                                                         │
│  Child Nodes:                                          │
│  ├── pH/EC Sensor (ph_ec_nft_001)                     │
│  ├── Climate Sensor (climate_nft_001)                 │
│  ├── Water Level (water_nft_001)                      │
│  ├── Relay Controller (relay_nft_001)                 │
│  └── Display (display_nft_001)                        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## 1. Настройка Root Node

### 1.1 Конфигурация через menuconfig

```bash
cd root_node
idf.py menuconfig
```

Настройте параметры:

#### Root Node Configuration
```
[*] Root Node Configuration
    Root Node ID (root_nft_001)
    Mesh Network ID (HYDRO_MESH_NFT_001)
    MQTT Topic Prefix (hydro/nft1/)
    Zone Name (NFT Zone 1)
```

#### WiFi Configuration
```
[*] WiFi Configuration
    WiFi SSID (YourSSID)
    WiFi Password (YourPassword)
```

#### MQTT Configuration
```
[*] MQTT Configuration
    MQTT Broker URI (mqtt://192.168.1.100:1883)
    MQTT Username (hydro_user)
    MQTT Password (secure_password)
```

### 1.2 Конфигурация через NVS

Альтернативно, можно настроить через команды NVS:

```c
// Установка Root Node ID
nvs_set_str(handle, "root_node_id", "root_nft_001");

// Установка Mesh Network ID
nvs_set_str(handle, "mesh_net_id", "HYDRO_MESH_NFT_001");

// Установка MQTT Topic Prefix
nvs_set_str(handle, "mqtt_prefix", "hydro/nft1/");
```

### 1.3 Прошивка Root Node

```bash
cd root_node
idf.py build
idf.py flash monitor
```

### 1.4 Проверка подключения

После прошивки проверьте логи:

```
I (5000) ROOT_NODE: Root Node ID: root_nft_001
I (5010) ROOT_NODE: Mesh Network ID: HYDRO_MESH_NFT_001
I (5020) MQTT: Connected to broker
I (5030) MQTT: Subscribed to: hydro/nft1/commands/#
```

## 2. Настройка Child Nodes

### 2.1 Конфигурация pH/EC узла

```bash
cd node_ph_ec
idf.py menuconfig
```

#### Mesh Configuration
```
[*] Mesh Configuration
    Root Node ID (root_nft_001)              # Должен совпадать с Root Node
    Mesh Network ID (HYDRO_MESH_NFT_001)     # Должен совпадать с Root Node
    Node ID (ph_ec_nft_001)                  # Уникальный ID узла
    Node Type (ph_ec)
```

#### Sensor Configuration
```
[*] pH/EC Sensor Configuration
    pH Calibration Point 1 (4.0)
    pH Calibration Point 2 (7.0)
    EC Calibration Factor (1.0)
```

### 2.2 Прошивка Child Node

```bash
idf.py build
idf.py flash monitor
```

### 2.3 Проверка mesh-подключения

```
I (3000) NODE_PH_EC: Node ID: ph_ec_nft_001
I (3010) NODE_PH_EC: Root Node ID: root_nft_001
I (3020) MESH: Connected to mesh network: HYDRO_MESH_NFT_001
I (3030) MESH: Parent found, layer: 2
```

## 3. Регистрация зоны на сервере

### 3.1 Через Web интерфейс

1. Откройте Dashboard → Zones
2. Нажмите "Добавить зону"
3. Заполните форму:
   - **Название**: "NFT Zone 1"
   - **Root Node ID**: root_nft_001
   - **Mesh Network ID**: HYDRO_MESH_NFT_001
   - **MQTT Topic Prefix**: hydro/nft1/
   - **Тип зоны**: NFT
   - **Объем резервуара**: 100 л
   - **Площадь выращивания**: 2 м²
4. Сохраните

### 3.2 Через API

```bash
curl -X POST http://localhost:8000/api/zones \
  -H "Content-Type: application/json" \
  -d '{
    "name": "NFT Zone 1",
    "root_node_id": "root_nft_001",
    "mesh_network_id": "HYDRO_MESH_NFT_001",
    "mqtt_topic_prefix": "hydro/nft1/",
    "zone_type": "nft",
    "reservoir_volume_liters": 100,
    "growing_area_m2": 2,
    "plant_capacity": 30
  }'
```

### 3.3 Через Seeder

```php
php artisan db:seed --class=ZoneSeeder
```

## 4. Назначение узлов зоне

### 4.1 Автоматическое назначение

Узлы автоматически назначаются зоне при первом подключении, если их `root_node_id` совпадает с зоной.

### 4.2 Ручное назначение через API

```bash
curl -X POST http://localhost:8000/api/zones/1/assign-node \
  -H "Content-Type: application/json" \
  -d '{
    "node_id": "ph_ec_nft_001",
    "role": "ph_sensor"
  }'
```

## 5. Проверка работы зоны

### 5.1 Проверка MQTT топиков

```bash
mosquitto_sub -h localhost -t "hydro/nft1/#" -v
```

Вы должны увидеть:
```
hydro/nft1/telemetry/ph_ec_nft_001 {"ph":6.0,"ec":1.5,...}
hydro/nft1/telemetry/climate_nft_001 {"temp":22.5,"humidity":65,...}
hydro/nft1/heartbeat/root_nft_001 {"uptime":12345,...}
```

### 5.2 Проверка через API

```bash
# Получить информацию о зоне
curl http://localhost:8000/api/zones/1

# Получить узлы зоны
curl http://localhost:8000/api/zones/1/nodes

# Получить телеметрию зоны
curl http://localhost:8000/api/zones/1/telemetry
```

## 6. Настройка нескольких зон

### 6.1 Пример: 3 независимые зоны

**Зона 1 - NFT**
```
Root Node ID: root_nft_001
Mesh Network ID: HYDRO_MESH_NFT_001
MQTT Topic: hydro/nft1/
```

**Зона 2 - DWC**
```
Root Node ID: root_dwc_001
Mesh Network ID: HYDRO_MESH_DWC_001
MQTT Topic: hydro/dwc1/
```

**Зона 3 - Drip**
```
Root Node ID: root_drip_001
Mesh Network ID: HYDRO_MESH_DRIP_001
MQTT Topic: hydro/drip1/
```

### 6.2 Изоляция зон

- ⚠️ **Mesh Network ID должен быть уникальным для каждой зоны**
- ⚠️ **Root Node ID должен быть уникальным для каждой зоны**
- ⚠️ **MQTT Topic Prefix должен быть уникальным для каждой зоны**
- ✅ Узлы разных зон **не видят** друг друга в mesh-сети
- ✅ MQTT сообщения изолированы по топикам

## 7. Troubleshooting

### Узел не подключается к mesh-сети

1. Проверьте Mesh Network ID:
   ```bash
   idf.py menuconfig → Mesh Configuration → Mesh Network ID
   ```
2. Проверьте, что Root Node запущен и в сети
3. Проверьте логи узла:
   ```
   E (5000) MESH: Failed to connect to mesh network
   ```

### Root Node не подключается к MQTT

1. Проверьте WiFi подключение:
   ```
   I (3000) WIFI: Connected to AP, IP: 192.168.1.50
   ```
2. Проверьте MQTT брокер:
   ```bash
   mosquitto -v
   ```
3. Проверьте credentials в menuconfig

### Телеметрия не доходит до сервера

1. Проверьте MQTT топики:
   ```bash
   mosquitto_sub -h localhost -t "#" -v
   ```
2. Проверьте MqttListenerCommand:
   ```bash
   php artisan mqtt:listen
   ```
3. Проверьте логи сервера:
   ```bash
   tail -f storage/logs/laravel.log
   ```

### Конфликт узлов между зонами

Если узел пытается подключиться к двум зонам одновременно:

1. Проверьте `root_node_id` в конфигурации узла
2. Убедитесь, что узел не назначен двум зонам в БД:
   ```sql
   SELECT * FROM zone_node_assignments WHERE node_id = 'ph_ec_001' AND ended_at IS NULL;
   ```
3. Удалите дублирующиеся назначения через API

## 8. Best Practices

### 8.1 Именование

**Root Nodes:**
- Формат: `root_<type>_<number>`
- Примеры: `root_nft_001`, `root_dwc_002`

**Mesh Network IDs:**
- Формат: `HYDRO_MESH_<TYPE>_<NUMBER>`
- Примеры: `HYDRO_MESH_NFT_001`, `HYDRO_MESH_DWC_002`

**MQTT Topics:**
- Формат: `hydro/<zone_slug>/`
- Примеры: `hydro/nft1/`, `hydro/dwc2/`

**Child Nodes:**
- Формат: `<type>_<zone>_<number>`
- Примеры: `ph_ec_nft_001`, `climate_dwc_001`

### 8.2 Масштабирование

- До 10 Root Nodes (зон) на один MQTT брокер
- До 50 Child Nodes на одну mesh-сеть
- Рекомендуется не более 3-5 слоев в mesh-сети

### 8.3 Мониторинг

- Настройте алерты для offline узлов
- Мониторьте качество mesh-соединения (RSSI)
- Отслеживайте uptime Root Nodes
- Проверяйте доступность MQTT брокера

## 9. Backup и восстановление

### 9.1 Backup конфигурации

```bash
# Экспорт зон из БД
php artisan db:export zones

# Backup NVS партиций ESP32
esptool.py read_flash 0x9000 0x6000 nvs_backup.bin
```

### 9.2 Восстановление

```bash
# Импорт зон в БД
php artisan db:import zones

# Восстановление NVS на ESP32
esptool.py write_flash 0x9000 nvs_backup.bin
```

## 10. Дополнительные ресурсы

- [Документация ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/esp-wifi-mesh.html)
- [MQTT Protocol](https://mqtt.org/)
- [Laravel Queue для фоновой обработки](https://laravel.com/docs/11.x/queues)
- [API Documentation](./API_DOCUMENTATION.md)

---

**Версия документа:** 1.0  
**Последнее обновление:** 2024-11-06

