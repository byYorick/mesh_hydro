# 🔧 Исправление ошибки компиляции mesh_config.h

## ❌ Проблема
```
fatal error: mesh_config.h: No such file or directory
   12 | #include "mesh_config.h"
```

## 🔍 Причина
Компонент `mqtt_client` не указывал зависимость от `mesh_config` в CMakeLists.txt

## ✅ Решение

### Файл: `root_node/components/mqtt_client/CMakeLists.txt`

**Было:**
```cmake
idf_component_register(
    SRCS "mqtt_client_manager.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt
    PRIV_REQUIRES esp_wifi esp_hw_support
)
```

**Стало:**
```cmake
idf_component_register(
    SRCS "mqtt_client_manager.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt mesh_config    # ✅ Добавлена зависимость
    PRIV_REQUIRES esp_wifi esp_hw_support
)
```

## 🚀 Пересборка

```bash
cd root_node
rm -rf build
idf.py build
```

---

**Статус:** ✅ Исправлено  
**Сборка:** 🔄 В процессе...

