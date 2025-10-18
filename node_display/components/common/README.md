# Common Components

Общие компоненты, используемые всеми узлами системы.

## 📚 Документация

**→ [USAGE.md](USAGE.md) - Полное руководство по использованию common компонентов**

## 📦 Компоненты

### mesh_manager
Управление ESP-WIFI-MESH сетью (ROOT и NODE режимы)

### mesh_protocol
JSON протокол обмена данными между узлами

### node_config
NVS хранение конфигураций для всех типов узлов

### ota_manager
OTA обновления прошивок

### sensor_base
Базовые функции для работы с датчиками (retry, validation, caching)

### actuator_base
Базовые функции для работы с исполнительными устройствами (safety checks, stats)

## 🚀 Быстрый старт

### Подключение компонентов

Компоненты уже подключены через `EXTRA_COMPONENT_DIRS` в каждом узле!

```cmake
# В node_xxx/CMakeLists.txt уже есть:
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_SOURCE_DIR}/../common/mesh_manager"
    "${CMAKE_SOURCE_DIR}/../common/mesh_protocol"
    "${CMAKE_SOURCE_DIR}/../common/node_config"
)
```

### Использование в коде

```c
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

void app_main(void) {
    // Mesh manager
    mesh_manager_config_t config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .mesh_password = "pass",
        .channel = 1
    };
    mesh_manager_init(&config);
    mesh_manager_start();
    
    // Protocol
    char json_buf[1024];
    mesh_protocol_create_telemetry("node_001", data, json_buf, sizeof(json_buf));
    
    // Config
    ph_ec_node_config_t node_cfg;
    node_config_load(&node_cfg, sizeof(node_cfg), "ph_ec_ns");
}
```

## 🔗 Альтернативные способы подключения

### 1. EXTRA_COMPONENT_DIRS (✅ уже настроено)
Рекомендуется, работает "из коробки"

### 2. Символические ссылки
Для удобства в IDE:
```bash
cd tools
./create_symlinks.sh  # Linux/Mac
create_symlinks.bat   # Windows (от администратора)
```

**Подробнее:** [USAGE.md](USAGE.md)

## 📖 Документация компонентов

См. README в каждой папке компонента для деталей API.

