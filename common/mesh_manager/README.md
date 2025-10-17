# Mesh Manager

Компонент для управления ESP-WIFI-MESH сетью.

## Возможности

- Поддержка ROOT и NODE режимов
- Единый API для отправки/приема данных
- Автоматическое переподключение
- Callback для обработки входящих данных
- Проверка статуса подключения

## Использование

### ROOT узел

```c
#include "mesh_manager.h"

mesh_manager_config_t config = {
    .mode = MESH_MODE_ROOT,
    .mesh_id = "HYDRO1",
    .mesh_password = "mesh_pass",
    .channel = 1,
    .max_connection = 10,
    .router_ssid = "MyWiFi",
    .router_password = "wifi_pass"
};

mesh_manager_init(&config);
mesh_manager_register_recv_cb(on_mesh_data_received);
mesh_manager_start();
```

### NODE узел

```c
mesh_manager_config_t config = {
    .mode = MESH_MODE_NODE,
    .mesh_id = "HYDRO1",
    .mesh_password = "mesh_pass",
    .channel = 1
};

mesh_manager_init(&config);
mesh_manager_register_recv_cb(on_mesh_data_received);
mesh_manager_start();

// Отправка данных на ROOT
const char *data = "{\"type\":\"telemetry\"}";
mesh_manager_send_to_root((uint8_t*)data, strlen(data));
```

## API

См. `mesh_manager.h`

