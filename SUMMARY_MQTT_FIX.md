# 📋 Сводка исправлений MQTT подключения

**Дата:** 22 октября 2025

---

## 🎯 Выполненные исправления

### 1. ✅ Исправлен IP адрес MQTT брокера

**Проблема:**
```
❌ Старый IP: 192.168.1.100
✅ Новый IP:  192.168.0.167
```

**Исправленные файлы:**
- `root_node/components/mqtt_client/mqtt_client_manager.c`
  - Удалён захардкоженный старый IP
  - Добавлен `#include "mesh_config.h"`
  - Теперь использует централизованный MQTT_BROKER_URI

- `root_node/components/mqtt_client/CMakeLists.txt`
  - Добавлена зависимость `mesh_config` в REQUIRES

- `root_node/README.md`
  - Обновлены все примеры с 192.168.1.100 на 192.168.0.167

### 2. ✅ Централизована конфигурация

**Единая точка настройки:** `common/mesh_config/mesh_config.h`
```c
#define MQTT_BROKER_HOST  "192.168.0.167"
#define MQTT_BROKER_PORT  1883
#define MQTT_BROKER_URI   "mqtt://192.168.0.167:1883"
```

**Преимущества:**
- Все узлы используют одну конфигурацию
- Легко изменить IP для всей системы
- Нет дублирования кода

---

## 📡 Текущая сетевая конфигурация

| Компонент | IP/URL | Порт | Статус |
|-----------|--------|------|--------|
| MQTT Broker | 192.168.0.167 | 1883 | ✅ Работает |
| Backend API | 192.168.0.167 | 8000 | ✅ Работает |
| Frontend | 192.168.0.167 | 3000 | ✅ Работает |
| WebSocket | 192.168.0.167 | 8080 | ✅ Работает |
| ROOT Node | 192.168.0.104 | - | 🔄 Перепрошивка |

**Сеть:**
- Gateway: 192.168.0.1
- Subnet: 255.255.255.0

---

## 🔄 Статус сборки

**Команда:** `idf.py build`  
**Проект:** root_node  
**Статус:** 🔄 **В процессе компиляции**

**Процессы:**
- ✅ ninja - активен
- ✅ xtensa-esp32s3-elf-gcc - компилирует
- ✅ cmake - настроил проект

**Ожидаемый результат:**
```
root_node/build/root_node.bin  (прошивка)
root_node/build/bootloader/bootloader.bin
root_node/build/partition_table/partition-table.bin
```

---

## 🚀 Следующие шаги

### После завершения сборки:

#### 1. Прошить ROOT узел
```bash
cd root_node
idf.py flash monitor
```

#### 2. Проверить подключение
Ожидаемые логи:
```
I (2467) mqtt_manager: Connecting to broker: mqtt://192.168.0.167:1883
I (2500) mqtt_manager: ✅ MQTT connected to broker
I (2505) mqtt_manager: ✅ Subscribed to hydro/command/#
I (2510) mqtt_manager: ✅ Subscribed to hydro/config/#
I (2515) ROOT: MQTT Broker: mqtt://192.168.0.167:1883
```

#### 3. Проверить в веб-интерфейсе
```
http://192.168.0.167:3000
```

Узел **root_001** должен:
- Появиться в списке узлов
- Показывать статус **Online** ✅
- Отправлять телеметрию

#### 4. Проверить MQTT топики
```bash
# На хосте
mosquitto_sub -h 192.168.0.167 -t "hydro/#" -v
```

Должны появляться сообщения:
```
hydro/discovery {...}
hydro/heartbeat/root_001 {...}
hydro/telemetry/root_001 {...}
```

---

## 📝 Документация

Создана подробная документация:

1. **`MQTT_FIX_LOG.md`**
   - Подробное описание проблемы
   - Процесс исправления
   - Проверка работы

2. **`root_node/COMPILE_FIX.md`**
   - Исправление ошибки компиляции
   - Настройка CMakeLists.txt

3. **`server/PRODUCTION_SETUP.md`**
   - Настройка production окружения
   - Команды управления

4. **`server/SETUP_COMPLETE.md`**
   - Финальная инструкция
   - Проверка системы

---

## ⚠️ Важные замечания

### При изменении IP адреса в будущем:

**1. Изменить только в одном месте:**
```c
// common/mesh_config/mesh_config.h
#define MQTT_BROKER_HOST  "НОВЫЙ_IP"
#define MQTT_BROKER_URI   "mqtt://НОВЫЙ_IP:1883"
```

**2. Пересобрать все узлы:**
```bash
cd root_node && idf.py build flash
cd node_climate && idf.py build flash
cd node_ph_ec && idf.py build flash
```

**3. НЕ захардкоживать IP в коде!**
- ✅ Использовать mesh_config.h
- ❌ Не писать IP напрямую в .c файлах

---

## 📊 Проверка после прошивки

### 1. Логи ROOT узла
```bash
idf.py monitor
```

### 2. Логи MQTT Listener
```bash
docker logs hydro_mqtt_listener -f
```

### 3. Логи MQTT брокера
```bash
docker logs hydro_mosquitto --tail 50
```

### 4. API проверка
```bash
curl http://192.168.0.167:8000/api/nodes
```

---

**Статус:** ✅ Исправления применены  
**Сборка:** 🔄 В процессе  
**Следующий шаг:** Прошить ROOT узел после завершения сборки

