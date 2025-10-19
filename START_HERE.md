# 🚀 MESH HYDRO V2.0 - БЫСТРЫЙ СТАРТ

**Пошаговая инструкция по запуску проекта с нуля**

---

## 📋 СОДЕРЖАНИЕ

1. [Требования](#требования)
2. [Подготовка окружения](#подготовка-окружения)
3. [Запуск Backend](#запуск-backend)
4. [Прошивка узлов](#прошивка-узлов)
5. [Проверка работы](#проверка-работы)
6. [Troubleshooting](#troubleshooting)

---

## ✅ ТРЕБОВАНИЯ

### Железо:
- **ROOT NODE:** ESP32-S3 (подключён к COM7)
- **NODE Climate:** ESP32 (подключён к COM10)
- **NODE pH/EC:** ESP32-S3 (подключён к COM9) - опционально
- **Wi-Fi роутер:** SSID "Yorick", пароль в `common/mesh_config/mesh_config.h`

### Софт:
- **Windows 10/11**
- **ESP-IDF v5.5** (через Espressif installer)
- **Docker Desktop** (для backend)
- **Git**

### Сеть:
- **Хост машина IP:** 192.168.1.100 (или измени в `common/mesh_config/mesh_config.h`)
- **MQTT Broker:** Mosquitto на 192.168.1.100:1883

---

## 🔧 ПОДГОТОВКА ОКРУЖЕНИЯ

### 1. Установка ESP-IDF (если не установлен):

1. Скачай: https://dl.espressif.com/dl/esp-idf/
2. Запусти installer
3. Выбери **ESP-IDF v5.5**
4. Установи в `C:\Espressif\`

### 2. Установка Docker Desktop:

1. Скачай: https://www.docker.com/products/docker-desktop/
2. Установи и запусти Docker Desktop
3. Проверь: `docker --version`

### 3. Клонирование проекта (если ещё не сделано):

```bash
git clone <repository_url>
cd mesh_hydro
```

---

## 🐳 ЗАПУСК BACKEND (Server + MQTT + Database)

### Шаг 1: Запуск Docker контейнеров

```batch
tools\server_start.bat
```

**Что произойдёт:**
- ✅ Запустится Mosquitto MQTT broker (192.168.1.100:1883)
- ✅ Запустится Laravel backend (http://localhost:8000)
- ✅ Запустится MQTT Listener (обработка сообщений от узлов)
- ✅ Запустится Frontend (http://localhost:3000)
- ✅ Создастся SQLite база данных

**Ожидаемый вывод:**
```
✅ Connected successfully!
📡 Subscribing to: hydro/telemetry/#
📡 Subscribing to: hydro/heartbeat/#
📡 Subscribing to: hydro/discovery
```

### Шаг 2: Проверка

Открой в браузере:
- **Dashboard:** http://localhost:3000
- **API:** http://localhost:8000/api/nodes

---

## ⚡ ПРОШИВКА УЗЛОВ (ESP32)

### ⚠️ ВАЖНО: Порядок прошивки имеет значение!

**1️⃣ СНАЧАЛА ROOT, 2️⃣ ПОТОМ NODE!**

---

### Шаг 1: Прошивка ROOT NODE (COM7)

```batch
tools\flash_root.bat
```

**Ожидаемый лог:**
```
I (2524) ROOT: ✓ IP address obtained: 192.168.1.191
I (2578) mqtt_manager: MQTT connected to broker
I (2586) mqtt_manager: Subscribed to hydro/command/#
I (2599) mqtt_manager: Subscribed to hydro/config/#
I (2611) mqtt_manager: Published discovery message
I (5184) mesh_manager: ✓ MESH Parent connected!
I (5187) mesh_manager: Layer: 1
```

**Что проверить:**
- ✅ ROOT получил IP адрес (192.168.1.191 или подобный)
- ✅ MQTT подключился к broker
- ✅ Discovery отправлен
- ✅ Mesh started (Layer: 1)

**НЕ должно быть:**
- ❌ "Host is unreachable"
- ❌ "Still waiting for IP..."
- ❌ "DHCP failed"

---

### Шаг 2: Прошивка NODE CLIMATE (COM10)

**⏰ ПОДОЖДИ 30 секунд после прошивки ROOT!**

```batch
tools\flash_climate.bat
```

**Ожидаемый лог:**
```
I (1060) mesh_manager: Mesh manager initialized (mode: NODE)
I (1063) mesh_manager: NODE mode: will connect to mesh AP 'HYDRO1'
I (5184) mesh_manager: ✓ MESH Parent connected!
I (5187) mesh_manager: Layer: 2
I (5191) mesh_manager: Parent BSSID: e4:fa:c4:4a:fe:19
I (10000) climate_ctrl: 📊 Telemetry: 24.5°C, 65%, 820ppm, 450lux, RSSI=-42
I (10000) climate_ctrl: 💓 Heartbeat sent
```

**Что проверить:**
- ✅ NODE подключился к ROOT (Layer: 2)
- ✅ Parent BSSID показывает MAC ROOT
- ✅ Telemetry отправляется каждые 5 сек
- ✅ Heartbeat отправляется каждые 5 сек

**НЕ должно быть:**
- ❌ "Node is currently root (voting)"
- ❌ "Connected to Yorick" (роутер)
- ❌ "disable self-organizing, search parent<stop reconnect>"

---

### Шаг 3 (опционально): Прошивка NODE pH/EC (COM9)

```batch
tools\flash_ph_ec.bat
```

---

## 🔍 ПРОВЕРКА РАБОТЫ

### 1. Проверка Backend MQTT Listener:

```batch
tools\server_logs.bat
```

**Должны видеть:**
```
💓 [HEARTBEAT] hydro/heartbeat/climate_001
💓 [HEARTBEAT] hydro/heartbeat/climate_001
🔍 [DISCOVERY] hydro/discovery
```

**Нажми `Ctrl+C` для выхода.**

---

### 2. Проверка Dashboard:

Открой: **http://localhost:3000**

**Должны видеть:**
- ✅ `root_98a316f5fde8` - **ONLINE** (зелёный)
- ✅ `climate_001` - **ONLINE** (зелёный)

**Если узлы показывают OFFLINE:**
- Подожди 10-15 секунд
- Обнови страницу (F5)

---

### 3. Проверка API:

Открой: **http://localhost:8000/api/nodes**

**Должен вернуть JSON:**
```json
[
  {
    "node_id": "root_98a316f5fde8",
    "node_type": "root",
    "online": true,
    "last_seen_at": "2025-10-19T12:18:00Z"
  },
  {
    "node_id": "climate_001",
    "node_type": "climate",
    "online": true,
    "last_seen_at": "2025-10-19T12:18:05Z"
  }
]
```

---

## 🛠️ TROUBLESHOOTING

### Проблема: ROOT не получает IP адрес

**Симптомы:**
```
W (5000) ROOT: Still waiting for IP... (10/60)
W (10000) ROOT: Still waiting for IP... (20/60)
```

**Решение:**
1. Проверь Wi-Fi роутер (SSID/пароль в `common/mesh_config/mesh_config.h`)
2. Проверь DHCP на роутере (должен быть включён)
3. Перезагрузи роутер

---

### Проблема: NODE подключается к роутеру вместо ROOT

**Симптомы:**
```
I (5000) wifi: Connected to Yorick
I (5100) mesh: Node is currently root (voting)
```

**Решение:**
1. ✅ Проверь что ROOT прошит и работает (Layer: 1)
2. ✅ Проверь канал Wi-Fi роутера (должен быть 7)
3. ✅ В `mesh_config.h` установи `MESH_NETWORK_CHANNEL 7`
4. ✅ Пересобери ОБА узла: `tools\rebuild_all.bat`

---

### Проблема: Backend не получает сообщения

**Симптомы:**
```
💓 [HEARTBEAT] hydro/heartbeat/climate_001  ← В логах есть
НО: узлы OFFLINE на dashboard
```

**Решение:**
1. Проверь что `MQTT_HOST=192.168.1.100` в `server/docker-compose.yml`
2. Перезапусти backend:
   ```batch
   tools\server_stop.bat
   tools\server_start.bat
   ```
3. Проверь симлинк БД:
   ```batch
   docker exec hydro_backend ls -la /var/www/html/database/
   ```
   Должен быть: `database.sqlite -> hydro_system.sqlite`

---

### Проблема: climate_001 не появляется на dashboard

**Симптомы:**
```
Backend получает heartbeat, но узел не создаётся в БД
```

**Решение:**
1. Проверь логи Laravel:
   ```batch
   tools\server_logs.bat
   ```
2. Проверь БД:
   ```batch
   docker exec hydro_backend php artisan tinker --execute="echo App\Models\Node::count();"
   ```
3. Очисти кэш:
   ```batch
   docker exec hydro_backend php artisan config:clear
   docker restart hydro_backend hydro_mqtt_listener
   ```

---

### Проблема: Ошибка компиляции ESP32

**Симптомы:**
```
ninja: build stopped: subcommand failed
```

**Решение:**
```batch
cd root_node  (или node_climate)
C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005
idf.py fullclean
idf.py build
```

---

## 📁 СТРУКТУРА ПРОЕКТА

```
mesh_hydro/
├── tools/                      # ← Все .bat скрипты здесь!
│   ├── flash_root.bat         # Прошивка ROOT (COM7)
│   ├── flash_climate.bat      # Прошивка Climate (COM10)
│   ├── flash_ph_ec.bat        # Прошивка pH/EC (COM9)
│   ├── rebuild_all.bat        # Пересборка всех узлов
│   ├── server_start.bat       # Запуск Docker
│   ├── server_stop.bat        # Остановка Docker
│   └── server_logs.bat        # Просмотр логов
│
├── common/                     # Общие компоненты
│   ├── mesh_config/           # ← ГЛАВНАЯ КОНФИГУРАЦИЯ!
│   ├── mesh_manager/          # Управление mesh
│   ├── mesh_protocol/         # JSON протокол
│   └── node_config/           # NVS конфигурация
│
├── root_node/                  # ROOT узел (ESP32-S3)
│   ├── components/            # Компоненты ROOT
│   ├── main/                  # app_main.c
│   └── AI_INSTRUCTIONS.md     # Инструкция для ИИ
│
├── node_climate/               # Climate узел (ESP32)
│   ├── components/            # Датчики
│   ├── main/                  # app_main.c
│   └── AI_INSTRUCTIONS_UPDATED.md
│
├── node_ph_ec/                 # pH/EC узел (ESP32-S3)
│   ├── components/            # Датчики + PID
│   ├── main/                  # app_main.c
│   └── AI_INSTRUCTIONS_COMPLETE.md
│
├── node_relay/                 # Relay узел (ESP32)
│   └── AI_INSTRUCTIONS_COMPLETE.md
│
├── node_water/                 # Water узел (ESP32-C3)
│   └── AI_INSTRUCTIONS_COMPLETE.md
│
├── node_display/               # Display узел (ESP32-S3)
│   └── AI_INSTRUCTIONS_COMPLETE.md
│
├── server/                     # Backend + MQTT + Frontend
│   ├── backend/               # Laravel API
│   ├── frontend/              # Vue.js Dashboard
│   ├── mqtt_bridge/           # MQTT Listener
│   └── docker-compose.yml     # Docker конфигурация
│
├── doc/                        # Документация
├── README.md                   # Краткое описание проекта
└── START_HERE.md               # ← ЭТА ИНСТРУКЦИЯ
```

---

## ⚙️ КОНФИГУРАЦИЯ

### Главный файл: `common/mesh_config/mesh_config.h`

```c
// Wi-Fi роутер (для ROOT)
#define MESH_ROUTER_SSID        "Yorick"
#define MESH_ROUTER_PASSWORD    "pro100parol"

// Mesh сеть
#define MESH_NETWORK_ID         "HYDRO1"
#define MESH_NETWORK_PASSWORD   "hydro_mesh_2025"
#define MESH_NETWORK_CHANNEL    7  // ← ФИКСИРОВАННЫЙ канал роутера!

// MQTT Broker
#define MQTT_BROKER_URI         "mqtt://192.168.1.100:1883"
```

### Если твой роутер на другом канале:

1. Узнай канал роутера (через настройки роутера или Wi-Fi анализатор)
2. Измени `MESH_NETWORK_CHANNEL` в `mesh_config.h`
3. Пересобери ВСЕ узлы: `tools\rebuild_all.bat`

---

## 🎯 ПОЛНЫЙ ПРОЦЕСС ЗАПУСКА (20 МИНУТ)

### 1. Запуск Backend (5 минут):

```batch
tools\server_start.bat
```

Жди сообщение:
```
✅ Connected successfully!
📡 Subscribing to: hydro/heartbeat/#
```

---

### 2. Прошивка ROOT (5 минут):

```batch
tools\flash_root.bat
```

Жди:
```
I (2524) ROOT: ✓ IP address obtained: 192.168.1.191
I (2578) mqtt_manager: MQTT connected to broker
I (5184) mesh_manager: ✓ MESH Parent connected!
```

**✅ ROOT готов!** (не закрывай окно монитора)

---

### 3. Прошивка Climate NODE (5 минут):

**⏰ ПОДОЖДИ 30 секунд после ROOT!**

```batch
tools\flash_climate.bat
```

Жди:
```
I (5184) mesh_manager: ✓ MESH Parent connected!
I (5187) mesh_manager: Layer: 2
I (10000) climate_ctrl: 📊 Telemetry: 24.5°C, 65%, 820ppm, 450lux
```

**✅ Climate готов!** (не закрывай окно монитора)

---

### 4. Проверка Dashboard (1 минута):

Открой: **http://localhost:3000**

**Должны появиться:**
- ✅ `root_98a316f5fde8` (root) - **ONLINE**
- ✅ `climate_001` (climate) - **ONLINE**

**Обновляй раз в 5 секунд:**
- Heartbeat обновляется
- Last seen меняется
- Телеметрия отображается

---

## 📊 ТИПЫ СООБЩЕНИЙ (Mesh Protocol)

### NODE → ROOT → MQTT → Server:

| Тип | Описание | Частота | Топик MQTT |
|-----|----------|---------|------------|
| **HEARTBEAT** | Проверка связи | 5 сек | `hydro/heartbeat/{node_id}` |
| **TELEMETRY** | Данные с датчиков | 5-30 сек | `hydro/telemetry/{node_id}` |
| **EVENT** | Критичные события | По событию | `hydro/event/{node_id}` |
| **DISCOVERY** | Регистрация узла | При старте + каждые 30 сек | `hydro/discovery` |

### Server → MQTT → ROOT → NODE:

| Тип | Описание | Топик MQTT |
|-----|----------|------------|
| **COMMAND** | Команда узлу | `hydro/command/{node_id}` |
| **CONFIG** | Конфигурация | `hydro/config/{node_id}` |

### Display → ROOT → Display:

| Тип | Описание |
|-----|----------|
| **REQUEST** | Запрос данных всех узлов |
| **RESPONSE** | Ответ с данными |

---

## 🔁 ПРОТОКОЛ РАБОТЫ СИСТЕМЫ

### Пример: Мониторинг температуры

```
1. NODE climate_001 читает датчик SHT3x:
   ├─ Температура: 24.5°C
   └─ Влажность: 65%

2. NODE создаёт JSON:
   {"type":"telemetry","node_id":"climate_001","data":{"temperature":24.5,"humidity":65}}

3. NODE отправляет через mesh → ROOT:
   mesh_manager_send_to_root(json, len)

4. ROOT получает в data_router:
   ├─ Парсит JSON
   ├─ Обновляет node_registry
   └─ Публикует в MQTT: hydro/telemetry/climate_001

5. MQTT Broker получает и рассылает подписчикам:
   ├─ Backend MqttService (автоматическая регистрация узла)
   └─ MQTT Listener (логирование)

6. Backend сохраняет в БД:
   ├─ Создаёт/обновляет NODE в таблице `nodes`
   ├─ Сохраняет телеметрию в `telemetry`
   └─ Обновляет `last_seen_at` и `online=true`

7. Frontend получает через WebSocket:
   └─ Dashboard обновляется в реальном времени
```

---

## 🎛️ КОМАНДЫ УПРАВЛЕНИЯ

### Пример: Изменить интервал чтения Climate

**Через MQTT:**
```bash
mosquitto_pub -h 192.168.1.100 -t hydro/command/climate_001 -m '{
  "type": "command",
  "node_id": "climate_001",
  "command": "set_read_interval",
  "params": {"interval_ms": 30000}
}'
```

**Через Dashboard:**
1. Открой узел `climate_001`
2. Settings → Read Interval
3. Измени на 30 секунд
4. Сохрани

**Что произойдёт:**
```
Server → MQTT hydro/command/climate_001
       ↓
ROOT data_router получает команду
       ↓
ROOT отправляет через mesh → NODE climate_001
       ↓
NODE обрабатывает в on_mesh_data_received()
       ↓
climate_controller_handle_command("set_read_interval", ...)
       ↓
Сохранение в NVS + применение
       ↓
NODE отправляет новую телеметрию с новым интервалом
```

---

## 🔧 ДОПОЛНИТЕЛЬНЫЕ ИНСТРУМЕНТЫ

### Полная пересборка всех узлов:

```batch
tools\rebuild_all.bat
```

**Когда использовать:**
- После изменения `mesh_config.h`
- После изменения mesh_manager
- После обновления ESP-IDF

---

### Остановка Backend:

```batch
tools\server_stop.bat
```

---

### Мониторинг работающего узла:

```bash
# Подключение к уже прошитому ROOT на COM7:
cd root_node
C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005
idf.py -p COM7 monitor
```

**Выход:** `Ctrl+]`

---

## 📚 ДОПОЛНИТЕЛЬНАЯ ДОКУМЕНТАЦИЯ

| Документ | Назначение |
|----------|------------|
| **README.md** | Краткое описание проекта |
| **ARCHITECTURE.md** | Архитектура системы |
| **TECH_STACK.md** | Технологии |
| **root_node/AI_INSTRUCTIONS.md** | Инструкция для ИИ - ROOT |
| **node_climate/AI_INSTRUCTIONS_UPDATED.md** | Инструкция для ИИ - Climate |
| **node_ph_ec/AI_INSTRUCTIONS_COMPLETE.md** | Инструкция для ИИ - pH/EC |
| **node_relay/AI_INSTRUCTIONS_COMPLETE.md** | Инструкция для ИИ - Relay |
| **node_water/AI_INSTRUCTIONS_COMPLETE.md** | Инструкция для ИИ - Water |
| **node_display/AI_INSTRUCTIONS_COMPLETE.md** | Инструкция для ИИ - Display |
| **common/mesh_config/README.md** | Детали конфигурации Mesh |
| **server/README.md** | Backend документация |

---

## 🎉 СИСТЕМА ГОТОВА!

**Если всё прошло успешно, у тебя есть:**

✅ ROOT узел (координатор mesh + MQTT мост)
✅ Climate узел (мониторинг температуры, влажности, CO2, lux)
✅ Backend API (Laravel + MQTT Listener)
✅ Frontend Dashboard (Vue.js)
✅ MQTT Broker (Mosquitto)
✅ Database (SQLite с auto-discovery)

**Все узлы общаются через mesh, данные идут на сервер в реальном времени!**

---

## 🆘 ПОЛУЧИТЬ ПОМОЩЬ

1. **Проверь логи:**
   - ESP32: в окне монитора
   - Backend: `tools\server_logs.bat`
   - Docker: `docker logs hydro_backend`

2. **Перезапусти всё:**
   ```batch
   tools\server_stop.bat
   tools\server_start.bat
   ```
   Перезагрузи ESP32 (кнопка RESET)

3. **Полная очистка и пересборка:**
   ```batch
   tools\rebuild_all.bat
   ```

---

**НАЧНИ С ЭТОГО ФАЙЛА И СЛЕДУЙ ШАГАМ! УСПЕХОВ!** 🚀

