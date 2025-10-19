# Mesh Config - Централизованная конфигурация

**Единый файл конфигурации для всей Mesh Hydro системы**

---

## 🎯 Назначение

Этот компонент содержит **ВСЕ общие настройки** для всех узлов системы:
- WiFi credentials (SSID и пароль роутера)
- Mesh network ID и пароль
- MQTT broker настройки
- Timeouts и интервалы

**Преимущество:** Нужно менять настройки только в ОДНОМ месте!

---

## 📁 Структура

```
common/mesh_config/
├── mesh_config.h      # Все общие настройки
├── CMakeLists.txt     # ESP-IDF конфигурация
└── README.md          # Этот файл
```

---

## 🔧 Как изменить настройки

### 1️⃣ Изменение WiFi credentials

**Файл:** `common/mesh_config/mesh_config.h`

**Найди строки:**
```c
#define MESH_ROUTER_SSID        "Yorick"
#define MESH_ROUTER_PASSWORD    "pro100parol"
```

**Измени на свои:**
```c
#define MESH_ROUTER_SSID        "YourWiFi"
#define MESH_ROUTER_PASSWORD    "YourPassword"
```

### 2️⃣ Изменение Mesh network ID

**Файл:** `common/mesh_config/mesh_config.h`

**Найди строки:**
```c
#define MESH_NETWORK_ID         "HYDRO1"
#define MESH_NETWORK_PASSWORD   "hydro_mesh_2025"
```

**Измени если нужна другая mesh сеть:**
```c
#define MESH_NETWORK_ID         "GREENHOUSE2"
#define MESH_NETWORK_PASSWORD   "secure_password_2025"
```

### 3️⃣ Изменение MQTT broker адреса

**Файл:** `common/mesh_config/mesh_config.h`

**Найди строку:**
```c
#define MQTT_BROKER_HOST        "192.168.1.100"
```

**Измени на IP твоего компьютера:**
```c
#define MQTT_BROKER_HOST        "192.168.1.150"
```

**И обнови URI:**
```c
#define MQTT_BROKER_URI         "mqtt://192.168.1.150:1883"
```

**Как узнать свой IP:**
```bash
# Windows:
ipconfig

# Linux/Mac:
ifconfig
```

---

## 🔄 Что делать после изменения

### ⚠️ КРИТИЧНО: Пересобрать ВСЕ узлы!

Любое изменение в `mesh_config.h` требует пересборки **ВСЕХ** узлов:

```bash
# 1. ROOT Node
cd root_node
idf.py build flash

# 2. NODE Climate
cd node_climate
idf.py build flash

# 3. NODE pH/EC
cd node_ph_ec
idf.py build flash

# И т.д. для всех узлов
```

**Почему:** Все узлы используют этот header, настройки компилируются в прошивку.

---

## 📊 Какие узлы используют mesh_config

| Узел | Файл | Что использует |
|------|------|----------------|
| **ROOT** | root_node/main/root_config.h | WiFi, Mesh, MQTT |
| **Climate** | node_climate/main/app_main.c | WiFi, Mesh |
| **pH/EC** | node_ph_ec/main/app_main.c | WiFi, Mesh |
| **Relay** | node_relay/main/app_main.c | WiFi, Mesh |
| **Water** | node_water/main/app_main.c | WiFi, Mesh |

**Итого:** ВСЕ узлы используют одну конфигурацию!

---

## ✅ Преимущества

### До (hardcode в каждом узле):

```
root_node/main/root_config.h:     "Yorick", "pro100parol"
root_node/sdkconfig.defaults:     "Yorick", "pro100password"  ← Опечатка!
node_climate/main/app_main.c:     "Yorick", "pro100password"  ← Другой пароль!
node_ph_ec/main/app_main.c:       "Yorick", "pro100parol"
...
```

**Проблемы:**
- ❌ Легко ошибиться (разные пароли)
- ❌ Сложно менять (6+ мест)
- ❌ Легко забыть обновить где-то

### После (централизованно):

```
common/mesh_config/mesh_config.h:  "Yorick", "pro100parol"
```

**Преимущества:**
- ✅ Одно место для изменения
- ✅ Невозможно использовать разные credentials
- ✅ Легко мигрировать на новый роутер
- ✅ Нет опечаток

---

## 🔍 Проверка hardcode

### Поиск оставшегося hardcode:

```bash
# Поиск WiFi SSID
grep -r "Yorick" root_node/main/*.c node_climate/main/*.c node_ph_ec/main/*.c

# Поиск WiFi password
grep -r "pro100" root_node/ node_climate/ node_ph_ec/

# Поиск Mesh ID
grep -r "HYDRO1" root_node/main/*.c node_climate/main/*.c node_ph_ec/main/*.c
```

**Ожидаемый результат:** Нет совпадений (всё использует mesh_config.h)

---

## 📝 Примеры использования

### В root_config.h:

```c
#include "mesh_config.h"

#define ROOT_MESH_ID            MESH_NETWORK_ID
#define ROOT_ROUTER_SSID        MESH_ROUTER_SSID
#define ROOT_ROUTER_PASSWORD    MESH_ROUTER_PASSWORD
```

### В node app_main.c:

```c
#include "mesh_config.h"

mesh_manager_config_t mesh_config = {
    .mode = MESH_MODE_NODE,
    .mesh_id = MESH_NETWORK_ID,
    .mesh_password = MESH_NETWORK_PASSWORD,
    .router_ssid = MESH_ROUTER_SSID,
    .router_password = MESH_ROUTER_PASSWORD,
};
```

---

## 🎯 Что настраивается

### Обязательные настройки:

- `MESH_ROUTER_SSID` - SSID вашего WiFi роутера
- `MESH_ROUTER_PASSWORD` - пароль WiFi роутера
- `MQTT_BROKER_HOST` - IP компьютера с Docker

### Опциональные настройки:

- `MESH_NETWORK_ID` - ID mesh сети (если несколько систем)
- `MESH_NETWORK_PASSWORD` - пароль mesh (для безопасности)
- `MESH_NETWORK_CHANNEL` - WiFi канал (0=auto, 1-13=fixed)
- `ROOT_MAX_MESH_CONNECTIONS` - макс узлов (6-10)
- `TELEMETRY_INTERVAL_MS` - интервал telemetry (по умолчанию 30 сек)
- `HEARTBEAT_INTERVAL_MS` - интервал heartbeat (по умолчанию 60 сек)

---

## 🚀 Быстрый старт

### Шаг 1: Настрой WiFi

```c
// common/mesh_config/mesh_config.h
#define MESH_ROUTER_SSID        "YourWiFi"      // ← Твой SSID
#define MESH_ROUTER_PASSWORD    "YourPassword"  // ← Твой пароль
```

### Шаг 2: Настрой MQTT broker

```c
// common/mesh_config/mesh_config.h
#define MQTT_BROKER_HOST        "192.168.1.XXX"  // ← IP твоего ПК
```

### Шаг 3: Пересобери все узлы

```bash
cd root_node && idf.py build
cd node_climate && idf.py build
cd node_ph_ec && idf.py build
```

### Шаг 4: Прошей узлы

```bash
cd root_node && idf.py flash
cd node_climate && idf.py flash
cd node_ph_ec && idf.py flash
```

**Готово!** Все узлы используют одинаковую конфигурацию.

---

## ⚠️ Важные замечания

### 1. Header-only компонент

`mesh_config` это header-only компонент (только .h файл, нет .c).

**Это значит:**
- Настройки компилируются прямо в код
- Изменения требуют пересборки
- Не занимает дополнительную память

### 2. Автоматическое подключение через EXTRA_COMPONENT_DIRS

Все узлы уже имеют в CMakeLists.txt:

```cmake
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../common")
```

Это значит `mesh_config` автоматически доступен для всех узлов!

### 3. Безопасность

В продакшене:
- ✅ Используй сильные пароли
- ✅ Не коммить mesh_config.h с реальными паролями в public repo
- ✅ Создай mesh_config.h.example с примерами

---

**Централизованная конфигурация - единая точка истины!** ✨

