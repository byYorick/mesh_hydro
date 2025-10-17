# 🌐 Mesh Hydro System V2

**Распределенная mesh-система гидропонного мониторинга с AI инструкциями для каждого узла**

## 🎯 Описание

Система включает:
- **1× ROOT узел** (ESP32-S3) - координатор mesh + MQTT
- **1× NODE Display** (ESP32-S3) - TFT экран + LVGL
- **N× NODE pH/EC** (ESP32-S3) - критичные узлы с автономией ⚠️
- **N× NODE Climate** (ESP32) - датчики климата
- **1× NODE Relay** (ESP32) - управление форточками и вентиляцией
- **N× NODE Water** (ESP32-C3) - управление водой
- **1× Веб-сервер** (Laravel + Vue.js + PostgreSQL + Mosquitto)

## 📁 Структура проекта

```
mesh_hydro/
├── common/              # Общие компоненты для всех узлов
├── root_node/           # ROOT координатор + AI_INSTRUCTIONS.md
├── node_display/        # Узел с TFT дисплеем + AI_INSTRUCTIONS.md
├── node_ph_ec/          # Узел pH/EC (критичный) + AI_INSTRUCTIONS.md
├── node_climate/        # Узел климата + AI_INSTRUCTIONS.md
├── node_relay/          # Узел управления + AI_INSTRUCTIONS.md
├── node_water/          # Узел воды + AI_INSTRUCTIONS.md
├── node_template/       # Шаблон + AI_INSTRUCTIONS.md
├── server/              # Веб-сервер (Laravel + Vue.js)
├── doc/                 # Документация
└── tools/               # Утилиты
```

**📖 Подробнее:** [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)

## 🤖 AI Инструкции

**✨ Каждый узел имеет свой AI_INSTRUCTIONS.md файл!**

### 📚 Индекс AI инструкций:

- **[AI_INSTRUCTIONS.md](AI_INSTRUCTIONS.md)** - Общие правила для всей системы
- **[AI_INSTRUCTIONS_INDEX.md](AI_INSTRUCTIONS_INDEX.md)** - Индекс всех AI инструкций

### По узлам:

| Узел | AI Instructions | Назначение |
|------|-----------------|------------|
| **ROOT** | [root_node/AI_INSTRUCTIONS.md](root_node/AI_INSTRUCTIONS.md) | Координатор mesh + MQTT (800 строк) |
| **pH/EC** | [node_ph_ec/AI_INSTRUCTIONS.md](node_ph_ec/AI_INSTRUCTIONS.md) | **КРИТИЧНЫЙ** узел с автономией (1000 строк) ⚠️ |
| **Display** | [node_display/AI_INSTRUCTIONS.md](node_display/AI_INSTRUCTIONS.md) | TFT + LVGL (300 строк) |
| **Climate** | [node_climate/AI_INSTRUCTIONS.md](node_climate/AI_INSTRUCTIONS.md) | Датчики климата (200 строк) |
| **Relay** | [node_relay/AI_INSTRUCTIONS.md](node_relay/AI_INSTRUCTIONS.md) | Форточки + вентиляция + PWM свет (250 строк) |
| **Water** | [node_water/AI_INSTRUCTIONS.md](node_water/AI_INSTRUCTIONS.md) | Насосы + клапаны (200 строк) |
| **Template** | [node_template/AI_INSTRUCTIONS.md](node_template/AI_INSTRUCTIONS.md) | Шаблон для новых узлов (150 строк) |

**Всего:** ~2900 строк специфичных AI инструкций!

## 🚀 Быстрый старт

### 1. Подключение common компонентов

Компоненты уже настроены через `EXTRA_COMPONENT_DIRS`:

```c
// В любом node просто используй:
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
```

**📖 Подробнее:** [common/USAGE.md](common/USAGE.md)

### 2. Прошивка узлов

**ROOT (ESP32-S3 #1):**
```bash
cd root_node
idf.py set-target esp32s3
idf.py menuconfig  # Настроить WiFi SSID/Password
idf.py build
idf.py -p COM3 flash monitor
```

**NODE pH/EC (ESP32-S3 #3):**
```bash
cd node_ph_ec
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
```

**Прошить все узлы сразу:**
```bash
cd tools
./flash_all.bat   # Windows
./flash_all.sh    # Linux/Mac
```

### 3. Настройка сервера

```bash
# Backend (Laravel)
cd server/backend
composer install
cp .env.example .env
php artisan key:generate
php artisan migrate

# Frontend (Vue.js)
cd server/frontend
npm install
npm run dev

# MQTT Listener
php artisan mqtt:listen

# Telegram Bot
php artisan telegram:bot
```

**📖 Полное руководство:** [SETUP_GUIDE.md](SETUP_GUIDE.md)

## 📖 Документация

### Общие:
- **[README.md](README.md)** - Этот файл (быстрый старт)
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Архитектура системы
- **[SETUP_GUIDE.md](SETUP_GUIDE.md)** - Подробная установка
- **[PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)** - Структура проекта
- **[AI_INSTRUCTIONS.md](AI_INSTRUCTIONS.md)** - Общие AI инструкции
- **[AI_INSTRUCTIONS_INDEX.md](AI_INSTRUCTIONS_INDEX.md)** - Индекс всех AI инструкций

### Специфичные:
- **[common/USAGE.md](common/USAGE.md)** - Использование common компонентов
- **Каждый node/** имеет свой `README.md` и `AI_INSTRUCTIONS.md`

### Из doc/:
- **[doc/MESH_HYDRO_V2_FINAL_PLAN.md](doc/MESH_HYDRO_V2_FINAL_PLAN.md)** - Детальный план (1681 строка)
- **[doc/MESH_START_HERE.md](doc/MESH_START_HERE.md)** - С чего начать

## 🔧 Утилиты (tools/)

- `flash_all.bat/sh` - Прошивка всех узлов
- `create_symlinks.bat/sh` - Создание симлинков на common
- `monitor_mesh.py` - Мониторинг mesh-сети
- `mqtt_tester.py` - Тестирование MQTT
- `backup_restore.sh` - Backup/Restore БД

**📖 Подробнее:** [tools/README.md](tools/README.md)

## ⚙️ Требования

### Для узлов (ESP32):
- ESP-IDF v5.1+
- ESP32-S3 (3 шт) - ROOT, Display, pH/EC
- ESP32 (2 шт) - Climate, Relay
- ESP32-C3 (1 шт) - Water

### Для сервера:
- PHP 8.1+
- Composer
- Node.js 18+
- PostgreSQL 15+
- Mosquitto MQTT Broker

## 🎯 Статус проекта

### ✅ Готово:
- Структура проекта создана (~70+ файлов)
- Common компоненты (6 модулей)
- ROOT узел (4 компонента)
- 6 NODE узлов (базовая структура)
- Сервер (базовая структура)
- AI инструкции (~2900 строк!)
- Утилиты (5 инструментов)
- Документация (README во всех папках)

### ⏳ В разработке (по фазам):
1. ⏳ Базовая mesh-сеть (3-4 дня)
2. ⏳ ROOT узел (3-4 дня)
3. ⏳ Сервер базовый (3-4 дня)
4. ⏳ **NODE pH/EC (4-5 дней)** ⚠️ КРИТИЧНО!
5. ⏳ Vue Dashboard (3-4 дня)
6. ⏳ NODE Display (3-4 дня)
7. ⏳ Climate + Relay (3-4 дня)
8. ⏳ Water (2-3 дня)
9. ⏳ Telegram + SMS (2-3 дня)
10. ⏳ OTA система (3-4 дня)
11. ⏳ Аутентификация (1-2 дня)
12. ⏳ Backup (1-2 дня)
13. ⏳ Документация (2-3 дня)
14. ⏳ Тестирование (3-5 дней)

**Общая оценка:** 36-51 день (1.5-2 месяца)

## 🏆 Ключевые особенности

- ✅ **Автономная работа pH/EC узлов** - продолжают работу при потере связи
- ✅ **Резервная логика климата** - ROOT управляет если Climate offline
- ✅ **OLED на pH/EC узлах** - локальный мониторинг
- ✅ **PWM диммирование света** - плавная регулировка 0-100%
- ✅ **OTA обновления** - прошивка через mesh или HTTP
- ✅ **Telegram + SMS уведомления** - критичные события
- ✅ **Ring buffer 1000 записей** - буферизация при offline
- ✅ **AI инструкции для каждого узла** - детальные гайды разработки

## 📝 Лицензия

MIT License

---

## 🤝 Для разработчиков

### Работаешь над конкретным узлом?
1. Открой его **AI_INSTRUCTIONS.md**
2. Следуй критичным правилам
3. Используй примеры кода
4. Проверяй чек-листы

### Создаешь новый узел?
1. Скопируй **node_template/**
2. Читай **node_template/AI_INSTRUCTIONS.md**
3. Адаптируй под свои нужды

### Нужна помощь?
- Общие вопросы → `AI_INSTRUCTIONS.md`
- Специфичные вопросы → `node_xxx/AI_INSTRUCTIONS.md`
- Архитектура → `ARCHITECTURE.md`
- Установка → `SETUP_GUIDE.md`

---

**Дата создания:** 17 октября 2025  
**Версия:** 2.0  
**Статус:** Структура проекта готова, начинается разработка

**Проект готов к разработке!** 🚀
