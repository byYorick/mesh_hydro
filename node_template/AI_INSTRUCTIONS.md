# 🤖 AI INSTRUCTIONS - NODE Template

## 🎯 Назначение

**NODE Template** - шаблон для создания новых узлов mesh-сети

### Как использовать:

1. **Скопируй папку:**
   ```bash
   cp -r node_template node_my_new_node
   ```

2. **Переименуй в CMakeLists.txt:**
   ```cmake
   project(node_my_new_node)
   ```

3. **Реализуй свою логику в app_main.c:**
   - Инициализация mesh (NODE режим)
   - Чтение датчиков / управление исполнителями
   - Отправка телеметрии на ROOT
   - Обработка команд от ROOT

4. **Добавь компоненты:**
   Создай `components/` папку со своими компонентами

---

## 📋 Минимальный код

```c
#include "mesh_manager.h"
#include "mesh_protocol.h"

void app_main(void) {
    // 1. NVS
    nvs_flash_init();
    
    // 2. Mesh (NODE режим)
    mesh_manager_config_t config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .mesh_password = "hydro_mesh_pass",
        .channel = 1
    };
    mesh_manager_init(&config);
    mesh_manager_start();
    
    // 3. Твоя логика
    while (1) {
        // Чтение данных
        float value = read_my_sensor();
        
        // Отправка телеметрии
        send_telemetry(value);
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
```

---

## 🔧 Подключение компонентов

### В CMakeLists.txt добавь нужные common компоненты:

```cmake
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_SOURCE_DIR}/../common/mesh_manager"
    "${CMAKE_SOURCE_DIR}/../common/mesh_protocol"
    "${CMAKE_SOURCE_DIR}/../common/node_config"
    "${CMAKE_SOURCE_DIR}/../common/sensor_base"      # Если нужны датчики
    "${CMAKE_SOURCE_DIR}/../common/actuator_base"    # Если нужны исполнители
)
```

---

## 📚 Примеры

Смотри готовые узлы:
- `node_ph_ec/` - сложный узел с автономией
- `node_climate/` - простой узел с датчиками
- `node_relay/` - узел с исполнителями
- `node_water/` - узел с насосами

---

## 🎯 Чек-лист

- [ ] Скопировал template
- [ ] Переименовал проект
- [ ] Настроил CMakeLists.txt
- [ ] Реализовал логику датчиков/исполнителей
- [ ] Добавил отправку телеметрии
- [ ] Добавил обработку команд
- [ ] Протестировал с ROOT узлом
- [ ] Обновил README.md

---

## 📖 Документация

- `common/USAGE.md` - Как использовать common компоненты
- `doc/NODE_CREATION_GUIDE.md` - Полное руководство (TODO)
- `doc/MESH_PROTOCOL.md` - Протокол обмена (TODO)

**Удачи в создании своего узла!** 🚀

