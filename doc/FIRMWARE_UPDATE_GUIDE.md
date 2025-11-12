# Руководство по обновлению прошивок для мультизонной системы

## Обзор изменений

Все прошивки обновлены для поддержки мультизонной архитектуры. Legacy код полностью удалён.

---

## Выполненные изменения

### 1. Common компоненты ✅

#### `common/mesh_protocol/`
- **Новые файлы:**
  - `zone_config.h` / `zone_config.c` — модуль управления zone конфигурацией
  - `CMakeLists.txt` — обновлён для включения zone_config
  
- **Обновлено:**
  - `mesh_protocol.h` — добавлена функция `mesh_topic_format()`
  - `mesh_protocol.c` — реализация `mesh_topic_format()`

**API для использования в нодах:**
```c
// Инициализация (вызвать при старте)
zone_config_init();

// Загрузка zone из NVS
char mesh_id[32], root_id[32];
zone_config_load(mesh_id, sizeof(mesh_id), root_id, sizeof(root_id));

// Сохранение zone в NVS (при конфигурации)
zone_config_save("zone_greenhouse_1", "root_setup");

// Получение текущих значений (кэшированные)
const char *mesh = zone_config_get_mesh_id();
const char *root = zone_config_get_root_id();

// Проверка конфигурации
if (!zone_config_is_configured()) {
    // Узел не сконфигурирован
}

// Формирование MQTT топика
char topic[192];
mesh_topic_format(topic, sizeof(topic), NULL, "heartbeat", "climate_001");
// Результат: "hydro/{current_mesh_id}/heartbeat/climate_001"
```

### 2. Root Node ✅

#### `root_node/components/mqtt_client/mqtt_client_manager.c`

**Изменения:**
1. Удалена переменная `s_topic_prefix`
2. Обновлена `refresh_zone_context()` — использует `zone_config_load()`
3. MQTT подписки изменены:
   - Старо: `{prefix}command/#` и `{prefix}config/#`
   - Новое: `hydro/{mesh_id}/command/#` и `hydro/{mesh_id}/config/#`
4. `mqtt_client_manager_send_discovery()` — использует `mesh_topic_format()`
5. `mqtt_client_manager_send_heartbeat()` — использует `mesh_topic_format()`

#### `root_node/components/data_router/data_router.c`

**Изменения:**
1. Удалены функции `refresh_topic_prefix()` и `ensure_topic_prefix()`
2. Удалена переменная `s_topic_prefix`
3. Все публикации в MQTT используют `mesh_topic_format()`:
   - Telemetry: `hydro/{mesh_id}/telemetry/{node_id}`
   - Event: `hydro/{mesh_id}/event/{node_id}`
   - Heartbeat: `hydro/{mesh_id}/heartbeat/{node_id}`
   - Config Response: `hydro/{mesh_id}/config_response/{node_id}`
   - Discovery: `hydro/{mesh_id}/discovery/{node_id}`
4. `data_router_handle_mqtt_data()` парсит новый формат топиков:
   - Формат: `hydro/{mesh_id}/{command|config}/{node_id}`
   - Проверка соответствия mesh_id с текущей зоной
   - Игнорирование команд для других зон

---

## Изменения для Leaf Nodes

Все leaf ноды (climate, ph, ph_ec, water, display, relay) должны быть обновлены одинаково:

### Требуемые изменения

#### 1. Глобальные переменные

**Добавить в `app_main.c`:**
```c
static char s_mesh_network_id[32] = {0};
static char s_root_node_id[32] = {0};
```

#### 2. Функция `run_normal_mode()` или `app_main()`

**После инициализации node_config, добавить:**
```c
// Инициализация zone_config
zone_config_init();

// Загрузка zone из NVS
if (zone_config_load(s_mesh_network_id, sizeof(s_mesh_network_id),
                     s_root_node_id, sizeof(s_root_node_id)) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to load zone config! Node is UNCONFIGURED");
    strcpy(s_mesh_network_id, "UNCONFIGURED");
    strcpy(s_root_node_id, "UNCONFIGURED");
}

ESP_LOGI(TAG, "Zone: mesh_id=%s, root_id=%s", s_mesh_network_id, s_root_node_id);

// Проверка конфигурации
if (!zone_config_is_configured()) {
    ESP_LOGW(TAG, "Node needs configuration, entering setup mode");
    // Можно перейти в setup mode или продолжить с UNCONFIGURED
}
```

#### 3. Обновление всех вызовов `mesh_protocol_create_*()`

**Старый код:**
```c
mesh_protocol_create_heartbeat(
    g_config.base.node_id,
    "climate",
    uptime,
    heap_free,
    buffer,
    sizeof(buffer)
);
```

**Новый код:**
```c
mesh_protocol_create_heartbeat(
    g_config.base.node_id,
    s_root_node_id,           // ← ДОБАВИТЬ
    s_mesh_network_id,        // ← ДОБАВИТЬ
    "climate",
    uptime,
    heap_free,
    buffer,
    sizeof(buffer)
);
```

**Применить к:**
- `mesh_protocol_create_heartbeat()`
- `mesh_protocol_create_telemetry()`
- `mesh_protocol_create_event()`
- `mesh_protocol_create_request()` (для display)

#### 4. Setup Mode (если используется)

**Функция `send_setup_message()` — добавить поля:**
```c
cJSON_AddStringToObject(root, "mesh_network_id", 
    s_mesh_network_id[0] ? s_mesh_network_id : "UNCONFIGURED");
cJSON_AddStringToObject(root, "root_node_id", 
    s_root_node_id[0] ? s_root_node_id : "UNCONFIGURED");
```

#### 5. Обработка команды `write_config`

**Функция `handle_write_config_command()` — сохранение zone:**
```c
// Извлечение из JSON
cJSON *mesh_id_item = cJSON_GetObjectItem(params, "mesh_id");
cJSON *root_node_id_item = cJSON_GetObjectItem(params, "root_node_id");

if (mesh_id_item && cJSON_IsString(mesh_id_item) &&
    root_node_id_item && cJSON_IsString(root_node_id_item)) {
    
    const char *mesh_id = mesh_id_item->valuestring;
    const char *root_id = root_node_id_item->valuestring;
    
    // Сохранить в NVS через zone_config
    esp_err_t err = zone_config_save(mesh_id, root_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save zone config: %s", esp_err_to_name(err));
        return err;
    }
    
    // Обновить глобальные переменные
    strncpy(s_mesh_network_id, mesh_id, sizeof(s_mesh_network_id) - 1);
    s_mesh_network_id[sizeof(s_mesh_network_id) - 1] = '\0';
    
    strncpy(s_root_node_id, root_id, sizeof(s_root_node_id) - 1);
    s_root_node_id[sizeof(s_root_node_id) - 1] = '\0';
    
    ESP_LOGI(TAG, "Zone config saved: mesh_id=%s, root_id=%s", mesh_id, root_id);
}
```

#### 6. CMakeLists.txt

**Добавить зависимость на zone_config:**

Если используется локальный `common/mesh_protocol`:
```cmake
idf_component_register(
    SRCS "app_main.c" # ... other sources
    INCLUDE_DIRS "."
    REQUIRES mesh_protocol mesh_manager node_config  # mesh_protocol уже включает zone_config
)
```

Если `mesh_protocol` не подключен напрямую, добавить в `components/`:
```
components/
  mesh_protocol/  -> симлинк на ../../common/mesh_protocol
```

---

## Пример: node_climate

### Структура изменений

**Файл:** `node_climate/main/app_main.c`

1. ✅ Добавлены глобальные переменные `s_mesh_network_id` и `s_root_node_id`
2. ✅ В `run_normal_mode()` добавлена загрузка zone через `zone_config_load()`
3. ✅ Обновлён вызов `mesh_protocol_create_heartbeat()` в `send_heartbeat()`
4. ✅ Обновлён вызов `mesh_protocol_create_telemetry()` в контроллере
5. ✅ Обновлён `send_setup_message()` для включения zone полей
6. ✅ Обновлён `handle_write_config_command()` для сохранения zone

---

## Список нод для обновления

| Нода | Статус | Примечания |
|------|--------|------------|
| `root_node` | ✅ Готово | |
| `node_climate` | 🔄 В работе | Пример реализации |
| `node_ph` | ⏳ Ожидает | Аналогично climate |
| `node_ph_ec` | ⏳ Ожидает | Аналогично climate |
| `node_water` | ⏳ Ожидает | Аналогично climate |
| `node_relay` | ⏳ Ожидает | Минимальная реализация |
| `node_display` | ⏳ Ожидает | Использует `mesh_protocol_create_request()` |

---

## Сборка и тестирование

### Сборка всех нод

```bash
cd root_node && idf.py build && cd ..
cd node_climate && idf.py build && cd ..
cd node_ph && idf.py build && cd ..
cd node_ph_ec && idf.py build && cd ..
cd node_water && idf.py build && cd ..
cd node_display && idf.py build && cd ..
cd node_relay && idf.py build && cd ..
```

### Прошивка

**Root Node (первым):**
```bash
cd root_node
idf.py -p COM3 flash monitor
```

**Leaf Nodes:**
```bash
cd node_climate
idf.py -p COM4 flash monitor
```

### Проверка

**Ожидаемые логи в Root Node:**
```
[mqtt_manager] Zone context: mesh_id=zone_greenhouse_1, root_node_id=root_setup
[mqtt_manager] Subscribed to hydro/zone_greenhouse_1/command/#
[mqtt_manager] Subscribed to hydro/zone_greenhouse_1/config/#
[mqtt_manager] Published discovery message to hydro/zone_greenhouse_1/discovery/root_setup
```

**Ожидаемые логи в Leaf Node:**
```
[app_main] Zone: mesh_id=zone_greenhouse_1, root_id=root_setup
[climate_controller] Heartbeat sent to Root
[data_router] Heartbeat published to hydro/zone_greenhouse_1/heartbeat/climate_001
```

---

## Известные проблемы и решения

### 1. Ошибка компиляции: `zone_config.h: No such file`

**Решение:**
```bash
# Создать симлинк в components/
cd node_climate/components
ln -s ../../common/mesh_protocol mesh_protocol
```

Или добавить путь в CMakeLists.txt:
```cmake
set(EXTRA_COMPONENT_DIRS "../../common")
```

### 2. Node остаётся UNCONFIGURED после write_config

**Причина:** zone параметры не сохраняются в NVS

**Решение:** Убедиться что `zone_config_save()` вызывается в `handle_write_config_command()`

### 3. MQTT сообщения не публикуются

**Причина:** Root Node не загружает zone из NVS

**Решение:** 
- Убедиться что `zone_config_init()` вызывается в root_node
- Проверить что NVS содержит `mesh_network_id` и `root_node_id`

---

## Следующие шаги

1. ✅ Обновить Root Node
2. 🔄 Обновить node_climate (в процессе)
3. ⏳ Обновить остальные leaf ноды по аналогии
4. ⏳ Обновить Backend для обработки новых топиков
5. ⏳ Обновить Frontend для отображения зон
6. ⏳ Провести интеграционное тестирование


