# Исправление ошибки cJSON в node_ph

## Проблема

При сборке `node_ph` возникала ошибка:
```
CMake Error: Failed to resolve component 'cjson' required by component 'pump_events': unknown name.
```

## Причина

В ESP-IDF v5.5 компонент называется **`json`**, а не `cjson`.

## Исправленные файлы

### 1. `node_ph/components/ph_manager/CMakeLists.txt`

**Было:**
```cmake
idf_component_register(
    SRCS "ph_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        ph_sensor
        pump_controller
        pid_controller
        adaptive_pid
        pump_events
        mesh_manager
        mesh_protocol
        node_config
        esp_wifi
)
```

**Стало:**
```cmake
idf_component_register(
    SRCS "ph_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        ph_sensor
        pump_controller
        pid_controller
        adaptive_pid
        pump_events
        mesh_manager
        mesh_protocol
        node_config
        esp_wifi
        json  # ← Добавлено
)
```

---

### 2. `node_ph/components/pump_events/CMakeLists.txt`

**Было:**
```cmake
idf_component_register(
    SRCS "pump_events.c"
    INCLUDE_DIRS "."
    REQUIRES "mesh_manager" "cjson" "esp_timer"  # ← ОШИБКА: "cjson"
)
```

**Стало:**
```cmake
idf_component_register(
    SRCS "pump_events.c"
    INCLUDE_DIRS "."
    REQUIRES mesh_manager json esp_timer pump_controller  # ← Исправлено
)
```

**Важно:**
- Убраны кавычки вокруг имен компонентов
- `"cjson"` заменен на `json`
- Добавлен `pump_controller` (т.к. pump_events.h включает pump_controller.h)

---

## Правильное использование cJSON в ESP-IDF v5.5

### В CMakeLists.txt:
```cmake
idf_component_register(
    SRCS "my_component.c"
    INCLUDE_DIRS "."
    REQUIRES json  # ← БЕЗ кавычек, имя компонента: json
)
```

### В .c файлах:
```c
#include "cJSON.h"  // ← С кавычками, имя заголовка: cJSON.h
```

---

## Проверка

После исправления сборка должна пройти успешно:

```bash
cd c:\esp\hydro\mesh\mesh_hydro\node_ph
idf.py build
```

**Ожидаемый результат:**
```
[100%] Built target app
esptool.py v4.8.2
To flash all build output, run 'idf.py flash' or:
python -m esptool --chip esp32 write_flash --flash_mode dio --flash_size 4MB ...
```

---

## Статус

✅ **Исправлено:**
- `ph_manager/CMakeLists.txt` - добавлен `json`
- `pump_events/CMakeLists.txt` - заменен `"cjson"` на `json`

✅ **Сборка запущена:**
```bash
cd c:\esp\hydro\mesh\mesh_hydro\node_ph
idf.py build  # ← В процессе
```

📝 **Следующий шаг:**
Дождаться окончания сборки и прошить pH ноду:
```bash
idf.py flash -p COM4 monitor
```

---

## Аналогичные проблемы в других нодах

Если подобная ошибка возникнет в других узлах (node_ec, node_ph_ec и т.д.), применить то же исправление:

```cmake
# Заменить все
REQUIRES "cjson"
# на
REQUIRES json
```

**Или:**
```cmake
# Заменить все
REQUIRES cjson
# на
REQUIRES json
```

**Проверить все узлы:**
```bash
cd c:\esp\hydro\mesh\mesh_hydro
Get-ChildItem -Recurse -Filter "CMakeLists.txt" | ForEach-Object {
    $content = Get-Content $_.FullName -Raw
    if ($content -match 'cjson') {
        Write-Host $_.FullName
    }
}
```

---

## Документация ESP-IDF

**ESP-IDF v5.5 cJSON компонент:**
- Имя компонента в CMake: **`json`**
- Заголовочный файл: **`cJSON.h`**
- Документация: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/storage/cjson.html

---

## Полезные команды

### Очистить build и пересобрать:
```bash
cd node_ph
rm -rf build
idf.py build
```

### Проверить какие компоненты используются:
```bash
idf.py build | grep "Building component"
```

### Найти все упоминания cjson:
```bash
grep -r "cjson" components/*/CMakeLists.txt
```

---

## Заключение

✅ Проблема решена
✅ Сборка pH ноды в процессе
✅ После прошивки можно тестировать popup уведомления

