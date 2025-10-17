# NODE Template

Шаблон для создания нового узла mesh-сети

## Как использовать

1. Скопируй эту папку
2. Переименуй в `node_ваше_имя`
3. Настрой `CMakeLists.txt`
4. Реализуй свою логику в `app_main.c`
5. Добавь компоненты в `components/`

## Минимальный код

```c
#include "mesh_manager.h"
#include "mesh_protocol.h"

void app_main(void) {
    // 1. Инициализация NVS
    nvs_flash_init();
    
    // 2. Настройка mesh (NODE режим)
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
        // Сбор данных
        // Отправка телеметрии
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
```

## Компоненты

Добавляй компоненты в `components/`, следуя структуре.

См. примеры в других node_* папках.

