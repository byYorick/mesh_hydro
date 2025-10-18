# Mesh Protocol

Компонент для работы с JSON протоколом обмена данными в mesh-сети.

## Типы сообщений

- **telemetry** - Телеметрия (NODE → ROOT → Server)
- **command** - Команды (Server → ROOT → NODE)
- **config** - Конфигурация (Server → NODE)
- **event** - События (NODE → Server)
- **heartbeat** - Проверка связи (каждые 10 сек)
- **request** - Запрос данных (Display → ROOT)
- **response** - Ответ на запрос (ROOT → Display)

## Использование

### Создание телеметрии

```c
#include "mesh_protocol.h"

cJSON *data = cJSON_CreateObject();
cJSON_AddNumberToObject(data, "ph", 6.5);
cJSON_AddNumberToObject(data, "ec", 1.8);

char json_buf[1024];
mesh_protocol_create_telemetry("ph_ec_001", data, json_buf, sizeof(json_buf));

// Отправка через mesh
mesh_manager_send_to_root((uint8_t*)json_buf, strlen(json_buf));

cJSON_Delete(data);
```

### Парсинг входящего сообщения

```c
void on_mesh_data_received(const uint8_t *src_addr, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    
    if (mesh_protocol_parse((char*)data, &msg)) {
        switch (msg.type) {
            case MESH_MSG_COMMAND:
                // Обработка команды
                break;
            case MESH_MSG_CONFIG:
                // Обработка конфигурации
                break;
            default:
                break;
        }
        
        mesh_protocol_free_message(&msg);
    }
}
```

## API

См. `mesh_protocol.h`

