# ROOT NODE

**ESP32-S3 #1** - Координатор mesh-сети и MQTT мост

## Функции

- Координатор ESP-WIFI-MESH сети
- MQTT клиент (связь с сервером)
- Node Registry (учет всех узлов)
- Data Router (маршрутизация NODE ↔ MQTT)
- Резервная логика климата (если Climate node offline)
- OTA координатор (раздача прошивок)

## Компоненты

- `node_registry/` - Реестр всех узлов в mesh
- `mqtt_client/` - MQTT клиент
- `data_router/` - Маршрутизация данных
- `climate_logic/` - Резервная логика климата
- `ota_coordinator/` - Управление OTA обновлениями

## Сборка

```bash
cd root_node
idf.py set-target esp32s3
idf.py menuconfig  # Настроить WiFi SSID/Password и MQTT
idf.py build
idf.py -p COM3 flash monitor
```

## Конфигурация

В `menuconfig` настроить:
- WiFi SSID/Password роутера
- MQTT broker адрес
- Mesh ID и пароль

