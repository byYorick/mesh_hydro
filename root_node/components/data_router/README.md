# Data Router

Маршрутизатор данных между mesh-сетью и MQTT.

## Возможности

- Прием данных от NODE узлов через mesh
- Пересылка в MQTT broker
- Прием команд от MQTT
- Пересылка командам узлам через mesh
- Обработка запросов от Display узла

## Потоки данных

### NODE → MQTT (телеметрия):
```
NODE pH/EC → mesh → ROOT → data_router → MQTT → Server
```

### MQTT → NODE (команды):
```
Server → MQTT → ROOT → data_router → mesh → NODE
```

### Display → ROOT → Display (запросы):
```
Display → mesh → ROOT → data_router → node_registry → mesh → Display
```

## API

Компонент автоматически регистрирует callbacks в mesh_manager и mqtt_client_manager.

```c
// Инициализация
data_router_init();

// Callbacks вызываются автоматически:
// - data_router_handle_mesh_data() при приеме от mesh
// - data_router_handle_mqtt_data() при приеме от MQTT
```

## Типы сообщений

### От NODE:
- `telemetry` → MQTT `hydro/telemetry`
- `event` → MQTT `hydro/event`
- `heartbeat` → MQTT `hydro/heartbeat`
- `request` → обработка локально

### От MQTT:
- `hydro/command/{node_id}` → mesh к узлу
- `hydro/config/{node_id}` → mesh к узлу

