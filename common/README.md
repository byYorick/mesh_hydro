# Common Components

Общие компоненты, используемые всеми узлами системы.

## 📚 Документация

**→ [AI_INSTRUCTIONS.md](AI_INSTRUCTIONS.md) - 🤖 Полная инструкция для ИИ-ассистентов**

**→ [USAGE.md](USAGE.md) - Руководство по использованию (если есть)**

---

## ⚙️ mesh_config - Централизованная конфигурация

**📁 `mesh_config/`** - **ЕДИНСТВЕННОЕ место** для изменения общих настроек!

### Что настраивается:
- ✅ WiFi Router (SSID и пароль)
- ✅ Mesh Network (ID, пароль, канал)
- ✅ MQTT Broker (host, port)
- ✅ Timeouts и интервалы

### Как изменить WiFi:

**Редактируй:** `common/mesh_config/mesh_config.h`
```c
#define MESH_ROUTER_SSID        "YourWiFi"
#define MESH_ROUTER_PASSWORD    "YourPassword"
```

**Пересобери:** ВСЕ узлы (root_node, node_climate, node_ph_ec и т.д.)

**Подробнее:** `mesh_config/README.md`

---

## 📦 Компоненты

### ✅ mesh_manager (ГОТОВ)
Управление ESP-WIFI-MESH сетью (ROOT и NODE режимы)
- Инициализация mesh в ROOT/NODE режимах
- Отправка/прием данных
- Broadcast всем узлам
- [Документация](mesh_manager/README.md)

### ✅ mesh_protocol (ГОТОВ)
JSON протокол обмена данными между узлами
- 7 типов сообщений (telemetry, command, config, event, heartbeat, request, response)
- Парсинг и создание JSON
- Проверка размера < 1KB
- [Документация](mesh_protocol/README.md)

### ✅ node_config (ГОТОВ)
NVS хранение конфигураций для всех типов узлов
- Сохранение/загрузка из NVS
- Парсинг JSON от ROOT
- Экспорт в JSON (включая PID параметры)
- Поддержка 4 типов узлов (ph_ec, climate, relay, water)
- [Документация](node_config/README.md) **(340+ строк примеров!)**

### 🔄 ota_manager (В РАЗРАБОТКЕ)
OTA обновления прошивок

### 🔄 sensor_base (В РАЗРАБОТКЕ)
Базовые функции для работы с датчиками (retry, validation, caching)

### 🔄 actuator_base (В РАЗРАБОТКЕ)
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

---

## ✅ Статус компонентов

| Компонент | Статус | Функционал |
|-----------|--------|------------|
| **mesh_manager** | ✅ ГОТОВ | ROOT/NODE режимы, broadcast, callbacks |
| **mesh_protocol** | ✅ ГОТОВ | 7 типов сообщений, парсинг/создание JSON |
| **node_config** | ✅ ГОТОВ | NVS storage, JSON ↔ структуры, 4 типа узлов |
| **oled_display** | ✅ НОВЫЙ | SSD1306 128x64, шаблонный вывод, heartbeat `<3` |
| **ota_manager** | 🔄 TODO | OTA обновления |
| **sensor_base** | 🔄 TODO | Базовый API для датчиков |
| **actuator_base** | 🔄 TODO | Базовый API для исполнителей |

**Готовность:** 3 из 6 компонентов (50%) ✅

**Следующий шаг:** ROOT узел (использует готовые компоненты)

---

## 🤖 Для ИИ-ассистентов

**Начни здесь:** [AI_INSTRUCTIONS.md](AI_INSTRUCTIONS.md)

Полная инструкция с примерами интеграции, правилами и чеклистом для новых узлов.

