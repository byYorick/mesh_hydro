# Как использовать common компоненты

## 🎯 Три способа подключения

### Способ 1: EXTRA_COMPONENT_DIRS (рекомендуется, уже настроено)

**Преимущества:**
- ✅ Уже работает "из коробки"
- ✅ Не требует администратора
- ✅ Кросс-платформенно (Windows/Linux/Mac)

**Использование:**

В вашем `node_xxx/CMakeLists.txt` уже есть:

```cmake
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_SOURCE_DIR}/../common/mesh_manager"
    "${CMAKE_SOURCE_DIR}/../common/mesh_protocol"
    "${CMAKE_SOURCE_DIR}/../common/node_config"
)
```

В коде просто используй:

```c
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
```

**Проверка:**
```bash
cd node_ph_ec
idf.py reconfigure
# Должны увидеть: "Component: mesh_manager"
```

---

### Способ 2: Символические ссылки (symlinks)

**Преимущества:**
- Видимость в файловом менеджере
- IDE может лучше находить файлы

**Использование:**

**Windows (требуется администратор):**
```batch
cd tools
create_symlinks.bat
```

**Linux/Mac:**
```bash
cd tools
chmod +x create_symlinks.sh
./create_symlinks.sh
```

После этого в каждом `node_*` появится папка `common` (симлинк).

В `CMakeLists.txt` можно упростить:

```cmake
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_SOURCE_DIR}/common/mesh_manager"
    "${CMAKE_SOURCE_DIR}/common/mesh_protocol"
)
```

---

### Способ 3: Глобальные компоненты ESP-IDF

**Для продвинутых:**

Можно добавить common в глобальную переменную:

```bash
# В ~/.bashrc или ~/.zshrc (Linux/Mac)
export IDF_EXTRA_COMPONENT_DIRS="/путь/к/mesh_hydro/common"

# Или в каждом проекте
idf.py -D EXTRA_COMPONENT_DIRS=/путь/к/common build
```

---

## 📦 Какие компоненты доступны

### mesh_manager
ESP-WIFI-MESH управление (ROOT и NODE режимы)

```c
#include "mesh_manager.h"

mesh_manager_config_t config = {
    .mode = MESH_MODE_NODE,
    .mesh_id = "HYDRO1",
    .mesh_password = "pass",
    .channel = 1
};

mesh_manager_init(&config);
mesh_manager_start();
```

### mesh_protocol
JSON протокол обмена

```c
#include "mesh_protocol.h"

cJSON *data = cJSON_CreateObject();
cJSON_AddNumberToObject(data, "ph", 6.5);

char json_buf[1024];
mesh_protocol_create_telemetry("ph_ec_001", data, json_buf, sizeof(json_buf));

mesh_manager_send_to_root((uint8_t*)json_buf, strlen(json_buf));
```

### node_config
NVS конфигурация

```c
#include "node_config.h"

ph_ec_node_config_t config;

// Загрузка из NVS
node_config_load(&config, sizeof(config), "ph_ec_ns");

// Сохранение в NVS
node_config_save(&config, sizeof(config), "ph_ec_ns");
```

### sensor_base
Базовые функции датчиков

```c
#include "sensor_base.h"

float value;
sensor_read_with_retry(my_sensor_read, &value, 3);

if (sensor_validate_range(value, 0.0, 14.0)) {
    sensor_cache_value("ph", value);
}
```

### actuator_base
Базовые функции исполнителей

```c
#include "actuator_base.h"

if (actuator_safety_check("pump_ph_up", 5000, 10000)) {
    // Включить насос на 5 сек
    actuator_log_usage("pump_ph_up", 5000);
}
```

### ota_manager
OTA обновления

```c
#include "ota_manager.h"

ota_manager_init();
ota_manager_start_update("http://192.168.1.100/firmware.bin");
```

---

## 🔍 Проверка подключения

### Способ 1: Через reconfigure

```bash
cd node_ph_ec
idf.py reconfigure
```

Ищи в выводе:
```
-- Component: mesh_manager
-- Component: mesh_protocol
-- Component: node_config
```

### Способ 2: Через build

```bash
cd node_ph_ec
idf.py build
```

Если компоненты не найдены, увидишь ошибку типа:
```
mesh_manager.h: No such file or directory
```

---

## ⚠️ Возможные проблемы

### Проблема 1: "mesh_manager.h not found"

**Решение:**
1. Проверь `CMakeLists.txt` - есть ли `EXTRA_COMPONENT_DIRS`
2. Проверь путь: `${CMAKE_SOURCE_DIR}/../common/mesh_manager`
3. Запусти `idf.py fullclean && idf.py reconfigure`

### Проблема 2: Symlink не создается (Windows)

**Решение:**
- Запусти `create_symlinks.bat` от администратора
- Или включи Developer Mode в Windows 10/11

### Проблема 3: Компонент найден, но линковка не работает

**Решение:**
Проверь `CMakeLists.txt` компонента:

```cmake
idf_component_register(
    SRCS "mesh_manager.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash
)
```

---

## 📝 Рекомендации

1. **Используй Способ 1 (EXTRA_COMPONENT_DIRS)** - уже настроен, работает везде
2. Symlinks создавай только если нужна видимость в IDE
3. В каждом узле подключай только нужные компоненты:
   - ROOT: mesh_manager, mesh_protocol, node_config
   - pH/EC: все компоненты
   - Climate: mesh_manager, mesh_protocol, sensor_base
   - Relay: mesh_manager, mesh_protocol, actuator_base

---

## 🚀 Быстрый старт

```bash
# 1. Перейди в узел
cd node_ph_ec

# 2. Настрой проект
idf.py set-target esp32s3

# 3. Проверь компоненты
idf.py reconfigure

# 4. Собери
idf.py build

# 5. Прошей
idf.py -p COM5 flash monitor
```

Компоненты из `common/` будут автоматически включены!

