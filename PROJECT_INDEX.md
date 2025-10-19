# 📚 MESH HYDRO V2.0 - ИНДЕКС ПРОЕКТА

**Навигация по всей документации и компонентам**

---

## 🚀 НАЧАЛО РАБОТЫ

| Документ | Назначение | Для кого |
|----------|------------|----------|
| **[START_HERE.md](START_HERE.md)** | 🔥 НАЧНИ ОТСЮДА! Пошаговая инструкция запуска | Все |
| **[README.md](README.md)** | Краткое описание проекта | Все |
| **[ИТОГОВЫЙ_СТАТУС_ПРОЕКТА.md](ИТОГОВЫЙ_СТАТУС_ПРОЕКТА.md)** | Текущий статус системы | Разработчики |
| **[CHANGELOG_19_10_2025.md](CHANGELOG_19_10_2025.md)** | Все изменения за 19.10.2025 | Разработчики |

---

## 🏗️ АРХИТЕКТУРА

| Документ | Содержание |
|----------|------------|
| **[ARCHITECTURE.md](ARCHITECTURE.md)** | Общая архитектура системы |
| **[TECH_STACK.md](TECH_STACK.md)** | Используемые технологии |
| **[PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)** | Структура файлов и папок |

---

## 🤖 AI ИНСТРУКЦИИ (ДЛЯ КАЖДОГО УЗЛА)

### ROOT NODE:
📄 **[root_node/AI_INSTRUCTIONS.md](root_node/AI_INSTRUCTIONS.md)**
- Назначение: Координатор mesh + MQTT мост
- Режим: ROOT (layer 1)
- Функции: MQTT bridge, data router, node registry

### NODE Climate:
📄 **[node_climate/AI_INSTRUCTIONS.md](node_climate/AI_INSTRUCTIONS.md)**
- Датчики: SHT3x, CCS811, Lux sensor
- Режим: NODE (только мониторинг)
- Функции: Telemetry каждые 5 сек

### NODE pH/EC:
📄 **[node_ph_ec/AI_INSTRUCTIONS.md](node_ph_ec/AI_INSTRUCTIONS.md)**
- Датчики: Trema pH, Trema EC
- Исполнители: 5 перистальтических насосов
- Режим: NODE с автономией (PID контроллер)

### NODE Relay:
📄 **[node_relay/AI_INSTRUCTIONS.md](node_relay/AI_INSTRUCTIONS.md)**
- Исполнители: Актуаторы форточек, вентилятор, LED свет
- Режим: NODE (только команды от ROOT)
- Функции: Управление климатом

### NODE Water:
📄 **[node_water/AI_INSTRUCTIONS.md](node_water/AI_INSTRUCTIONS.md)**
- Датчики: HC-SR04, Float switches
- Исполнители: Насос, 2 клапана
- Режим: NODE (только команды от ROOT)
- Функции: Управление водоснабжением

### NODE Display:
📄 **[node_display/AI_INSTRUCTIONS.md](node_display/AI_INSTRUCTIONS.md)**
- Оборудование: TFT 320x240 (ILI9341), энкодер
- Режим: NODE (запрос данных у ROOT)
- Функции: Локальный мониторинг с LVGL UI

---

## 🧩 ОБЩИЕ КОМПОНЕНТЫ

### Mesh Configuration:
📄 **[common/mesh_config/README.md](common/mesh_config/README.md)**
- **ГЛАВНАЯ КОНФИГУРАЦИЯ!**
- Wi-Fi роутер (SSID/пароль)
- Mesh сеть (ID/пароль/канал)
- MQTT broker URI

### Mesh Manager:
📄 **[common/mesh_manager/README.md](common/mesh_manager/README.md)**
- Управление ESP-WIFI-MESH
- Режимы: ROOT и NODE
- API wrapper для esp_mesh_*

### Mesh Protocol:
📄 **[common/mesh_protocol/README.md](common/mesh_protocol/README.md)**
- JSON протокол обмена
- Типы сообщений: TELEMETRY, COMMAND, CONFIG, EVENT, HEARTBEAT
- Создание и парсинг JSON

### Node Config:
📄 **[common/node_config/README.md](common/node_config/README.md)**
- NVS конфигурация
- Загрузка/сохранение настроек
- Структуры данных узлов

### Usage Guide:
📄 **[common/USAGE.md](common/USAGE.md)**
- Как использовать общие компоненты
- Примеры кода
- Best practices

---

## 🐳 BACKEND & SERVER

### Backend:
📄 **[server/README.md](server/README.md)**
- Laravel API документация
- Database schema
- MqttService

📄 **[server/backend/QUICK_START.md](server/backend/QUICK_START.md)**
- Быстрый старт backend

📄 **[server/backend/STRUCTURE.md](server/backend/STRUCTURE.md)**
- Структура Laravel приложения

### Frontend:
📄 **[server/frontend/README.md](server/frontend/README.md)**
- Vue.js Dashboard
- Компоненты UI
- WebSocket integration

### Docker:
📄 **[server/DOCKER_QUICK_START.md](server/DOCKER_QUICK_START.md)**
- Запуск через Docker
- docker-compose.yml

📄 **[server/DOCKER_TROUBLESHOOTING.md](server/DOCKER_TROUBLESHOOTING.md)**
- Решение проблем с Docker

---

## 🛠️ УТИЛИТЫ

📄 **[tools/README.md](tools/README.md)** - Описание всех утилит

### Windows Scripts:
| Скрипт | Назначение |
|--------|------------|
| `flash_root.bat` | Прошивка ROOT (COM7) |
| `flash_climate.bat` | Прошивка Climate (COM10) |
| `flash_ph_ec.bat` | Прошивка pH/EC (COM9) |
| `rebuild_all.bat` | Пересборка всех узлов |
| `server_start.bat` | Запуск Docker |
| `server_stop.bat` | Остановка Docker |
| `server_logs.bat` | Просмотр логов |

### Python Scripts:
| Скрипт | Назначение |
|--------|------------|
| `monitor_mesh.py` | Мониторинг mesh через MQTT |
| `mqtt_tester.py` | Тестирование команд |

### Shell Scripts:
| Скрипт | Назначение |
|--------|------------|
| `backup_restore.sh` | Резервное копирование |
| `flash_all.sh` | Прошивка всех узлов (Linux/Mac) |

---

## 📖 ДОКУМЕНТАЦИЯ ПО УЗЛАМ

### ROOT NODE:
- 📄 [root_node/README.md](root_node/README.md) - Описание
- 📄 [root_node/AI_INSTRUCTIONS.md](root_node/AI_INSTRUCTIONS.md) - Полная инструкция
- 📄 [root_node/components/mqtt_client/README.md](root_node/components/mqtt_client/README.md) - MQTT клиент
- 📄 [root_node/components/data_router/README.md](root_node/components/data_router/README.md) - Маршрутизатор данных
- 📄 [root_node/components/node_registry/README.md](root_node/components/node_registry/README.md) - Реестр узлов
- 📄 [root_node/components/climate_logic/README.md](root_node/components/climate_logic/README.md) - Fallback логика

### NODE Climate:
- 📄 [node_climate/README.md](node_climate/README.md) - Описание
- 📄 [node_climate/AI_INSTRUCTIONS.md](node_climate/AI_INSTRUCTIONS.md) - **Полная инструкция (761 строк)**

### NODE pH/EC:
- 📄 [node_ph_ec/README.md](node_ph_ec/README.md) - Описание
- 📄 [node_ph_ec/AI_INSTRUCTIONS.md](node_ph_ec/AI_INSTRUCTIONS.md) - **Полная инструкция (846 строк)**

### NODE Relay:
- 📄 [node_relay/README.md](node_relay/README.md) - Описание
- 📄 [node_relay/AI_INSTRUCTIONS.md](node_relay/AI_INSTRUCTIONS.md) - **Полная инструкция (844 строк)**

### NODE Water:
- 📄 [node_water/README.md](node_water/README.md) - Описание
- 📄 [node_water/AI_INSTRUCTIONS.md](node_water/AI_INSTRUCTIONS.md) - **Полная инструкция (843 строк)**

### NODE Display:
- 📄 [node_display/README.md](node_display/README.md) - Описание
- 📄 [node_display/AI_INSTRUCTIONS.md](node_display/AI_INSTRUCTIONS.md) - **Полная инструкция (885 строк)**

---

## 📋 ПЛАНЫ И ROADMAP

### Документы в `doc/`:
- 📄 [doc/MESH_START_HERE.md](doc/MESH_START_HERE.md) - Введение в mesh
- 📄 [doc/MESH_QUICK_SUMMARY.md](doc/MESH_QUICK_SUMMARY.md) - Краткая сводка
- 📄 [doc/MESH_ARCHITECTURE_PLAN.md](doc/MESH_ARCHITECTURE_PLAN.md) - Архитектурный план
- 📄 [doc/MESH_HYDRO_V2_FINAL_PLAN.md](doc/MESH_HYDRO_V2_FINAL_PLAN.md) - Финальный план v2.0
- 📄 [doc/MESH_IMPLEMENTATION_PRIORITY.md](doc/MESH_IMPLEMENTATION_PRIORITY.md) - Приоритеты реализации
- 📄 [doc/MESH_PINOUT_ALL_NODES.md](doc/MESH_PINOUT_ALL_NODES.md) - Распиновка всех узлов

---

## 🔍 КАК НАЙТИ НУЖНОЕ

### Хочу запустить проект с нуля:
👉 **[START_HERE.md](START_HERE.md)**

### Хочу понять архитектуру:
👉 **[ARCHITECTURE.md](ARCHITECTURE.md)**

### Хочу прошить узел:
👉 **[tools/README.md](tools/README.md)** → `flash_*.bat`

### Хочу разработать новый узел:
👉 **[node_template/AI_INSTRUCTIONS.md](node_template/AI_INSTRUCTIONS.md)**

### Хочу понять протокол:
👉 **[common/mesh_protocol/README.md](common/mesh_protocol/README.md)**

### Хочу настроить mesh:
👉 **[common/mesh_config/README.md](common/mesh_config/README.md)**

### Хочу работать с backend:
👉 **[server/README.md](server/README.md)**

### Хочу решить проблему:
👉 **[START_HERE.md#troubleshooting](START_HERE.md#troubleshooting)**

---

## 📊 ТИПЫ ДОКУМЕНТОВ

### 🔥 КРИТИЧНО (читай обязательно):
- START_HERE.md
- README.md
- common/mesh_config/README.md
- AI_INSTRUCTIONS.md (для твоего узла)

### 📖 ВАЖНО (для разработки):
- ARCHITECTURE.md
- TECH_STACK.md
- tools/README.md
- common/mesh_protocol/README.md

### 📋 ПОЛЕЗНО (для углублённого изучения):
- doc/* (планы, roadmap)
- server/* (backend детали)
- CHANGELOG_19_10_2025.md

---

## 🎯 БЫСТРЫЕ ССЫЛКИ

### Прошивка:
- [Прошить ROOT](tools/flash_root.bat) (COM7)
- [Прошить Climate](tools/flash_climate.bat) (COM10)
- [Прошить pH/EC](tools/flash_ph_ec.bat) (COM9)
- [Пересобрать все](tools/rebuild_all.bat)

### Backend:
- [Запустить](tools/server_start.bat)
- [Остановить](tools/server_stop.bat)
- [Логи](tools/server_logs.bat)

### Dashboard:
- [Frontend](http://localhost:3000)
- [API](http://localhost:8000/api/nodes)

---

## 📁 СТРУКТУРА ПРОЕКТА (упрощённая)

```
mesh_hydro/
│
├── 🚀 START_HERE.md           # ← НАЧНИ ОТСЮДА!
├── README.md                   # Описание проекта
├── PROJECT_INDEX.md            # ← ТЫ ЗДЕСЬ
│
├── tools/                      # Все утилиты
│   ├── flash_*.bat            # Прошивка
│   ├── server_*.bat           # Backend
│   └── README.md              # Описание утилит
│
├── common/                     # Общие компоненты
│   ├── mesh_config/           # ← ГЛАВНАЯ КОНФИГУРАЦИЯ!
│   ├── mesh_manager/
│   ├── mesh_protocol/
│   └── node_config/
│
├── root_node/                  # ROOT NODE (ESP32-S3 #1)
│   ├── components/
│   ├── main/
│   ├── AI_INSTRUCTIONS.md     # Инструкция для ИИ
│   └── README.md
│
├── node_climate/               # NODE Climate (ESP32)
│   ├── components/
│   ├── main/
│   ├── AI_INSTRUCTIONS.md     # 761 строк
│   └── README.md
│
├── node_ph_ec/                 # NODE pH/EC (ESP32-S3 #3)
│   ├── AI_INSTRUCTIONS.md     # 846 строк
│   └── README.md
│
├── node_relay/                 # NODE Relay (ESP32)
│   ├── AI_INSTRUCTIONS.md     # 844 строк
│   └── README.md
│
├── node_water/                 # NODE Water (ESP32-C3)
│   ├── AI_INSTRUCTIONS.md     # 843 строк
│   └── README.md
│
├── node_display/               # NODE Display (ESP32-S3 #2)
│   ├── AI_INSTRUCTIONS.md     # 885 строк
│   └── README.md
│
├── node_template/              # Шаблон нового узла
│   ├── AI_INSTRUCTIONS.md
│   └── README.md
│
├── server/                     # Backend + MQTT + Frontend
│   ├── backend/               # Laravel
│   ├── frontend/              # Vue.js
│   ├── mqtt_bridge/           # MQTT Listener
│   ├── docker-compose.yml
│   └── README.md
│
└── doc/                        # Планы, roadmap
    ├── MESH_START_HERE.md
    ├── MESH_ARCHITECTURE_PLAN.md
    └── MESH_PINOUT_ALL_NODES.md
```

---

## 🔄 ПРОТОКОЛ СООБЩЕНИЙ

### Mesh Protocol (JSON):

| Тип | Направление | Частота | Топик MQTT |
|-----|-------------|---------|------------|
| **HEARTBEAT** | NODE → ROOT → MQTT | 5 сек | `hydro/heartbeat/{node_id}` |
| **TELEMETRY** | NODE → ROOT → MQTT | 5-30 сек | `hydro/telemetry/{node_id}` |
| **EVENT** | NODE → ROOT → MQTT | По событию | `hydro/event/{node_id}` |
| **DISCOVERY** | ROOT/NODE → MQTT | 30 сек (ROOT), при старте (NODE) | `hydro/discovery` |
| **COMMAND** | MQTT → ROOT → NODE | По запросу | `hydro/command/{node_id}` |
| **CONFIG** | MQTT → ROOT → NODE | По запросу | `hydro/config/{node_id}` |
| **REQUEST** | Display → ROOT | Каждые 5 сек | - (через mesh) |
| **RESPONSE** | ROOT → Display | По запросу | - (через mesh) |

**Подробно:** [common/mesh_protocol/README.md](common/mesh_protocol/README.md)

---

## 🎓 ОБУЧЕНИЕ

### Новичок в проекте?

1. **День 1:** Прочитай [START_HERE.md](START_HERE.md) и запусти систему
2. **День 2:** Изучи [ARCHITECTURE.md](ARCHITECTURE.md) и [TECH_STACK.md](TECH_STACK.md)
3. **День 3:** Прочитай [common/mesh_protocol/README.md](common/mesh_protocol/README.md)
4. **День 4:** Выбери узел для разработки и прочитай его AI_INSTRUCTIONS.md
5. **День 5+:** Начни разработку!

### Хочешь разработать новый узел?

1. Скопируй `node_template/` → `node_new/`
2. Прочитай соответствующий AI_INSTRUCTIONS.md (node_relay/node_water/node_display)
3. Следуй checklist из инструкции
4. Тестируй с ROOT узлом
5. Создай PR

---

## 🐛 TROUBLESHOOTING

### Проблемы с прошивкой:
👉 **[START_HERE.md#troubleshooting](START_HERE.md#troubleshooting)**

### Проблемы с Docker:
👉 **[server/DOCKER_TROUBLESHOOTING.md](server/DOCKER_TROUBLESHOOTING.md)**

### Проблемы с mesh:
👉 **[doc/MESH_START_HERE.md](doc/MESH_START_HERE.md)**

---

## 📞 БЫСТРАЯ ПОМОЩЬ

### Команда не работает?
```batch
# Проверь логи:
tools\server_logs.bat
```

### Узел offline?
```
1. Проверь COM порт (Device Manager)
2. Перезагрузи ESP32 (кнопка RST)
3. Проверь логи в idf.py monitor
```

### Backend не запускается?
```batch
# Перезапуск:
tools\server_stop.bat
tools\server_start.bat
```

---

## 🏆 СТАТУС РЕАЛИЗАЦИИ

| Узел | Разработка | Тестирование | Production |
|------|------------|--------------|------------|
| **ROOT** | ✅ 100% | ✅ 100% | ✅ READY |
| **Climate** | ✅ 100% | ✅ 100% | ✅ READY |
| **pH/EC** | 🟡 60% | 🟡 40% | 🔴 Not ready |
| **Relay** | 🟡 30% | ❌ 0% | 🔴 Not ready |
| **Water** | 🟡 30% | ❌ 0% | 🔴 Not ready |
| **Display** | 🟡 40% | ❌ 0% | 🔴 Not ready |
| **Backend** | ✅ 100% | ✅ 100% | ✅ READY |
| **Frontend** | ✅ 90% | 🟡 70% | 🟡 Almost ready |

---

## 🗺️ НАВИГАЦИЯ ПО ЗАДАЧАМ

### Хочу исправить баг:
1. Найди узел в структуре выше
2. Прочитай его AI_INSTRUCTIONS.md
3. Найди раздел "Troubleshooting"
4. Примени решение

### Хочу добавить функцию:
1. Определи в каком узле (Climate/pH/EC/Relay/Water/Display)
2. Прочитай AI_INSTRUCTIONS.md узла
3. Найди раздел с похожей функцией
4. Реализуй по аналогии
5. Тестируй

### Хочу изменить конфигурацию:
1. Открой **common/mesh_config/mesh_config.h**
2. Измени нужные параметры
3. Пересобери ВСЕ узлы: `tools\rebuild_all.bat`
4. Прошей все узлы (ROOT, затем NODE)

---

## 📚 ВСЕГО В ПРОЕКТЕ

- **Документов:** 30+ файлов
- **Строк документации:** ~10000
- **Утилит:** 12 скриптов
- **Узлов:** 6 (ROOT + 5 NODE)
- **Общих компонентов:** 4
- **Backend компонентов:** 15+

---

## 🎉 СИСТЕМА ГОТОВА К РАБОТЕ!

**Начни с:** [START_HERE.md](START_HERE.md)

**Открой Dashboard:** http://localhost:3000

**Увидишь:**
- ✅ root_98a316f5fde8 (ONLINE)
- ✅ climate_001 (ONLINE)

---

**ВСЯ ДОКУМЕНТАЦИЯ В ОДНОМ МЕСТЕ!** 📚🚀

