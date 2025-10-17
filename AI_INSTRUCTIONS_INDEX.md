# 🤖 AI INSTRUCTIONS - Индекс

Специфичные AI инструкции для каждого узла системы

## 📋 Список AI инструкций

### 🌐 Общие инструкции

**[AI_INSTRUCTIONS.md](AI_INSTRUCTIONS.md)** - Общие правила для всей системы
- Критичные правила (NODE pH/EC святое, JSON < 1KB)
- Структура проекта
- Протокол mesh обмена
- Приоритеты разработки
- Полезные ссылки

---

### 📦 Специфичные инструкции по узлам

#### ⭐ [root_node/AI_INSTRUCTIONS.md](root_node/AI_INSTRUCTIONS.md)
**ROOT NODE (ESP32-S3 #1)** - Координатор mesh + MQTT мост

**Что внутри:**
- Назначение: координатор, MQTT, реестр узлов
- Архитектура ROOT узла
- Компоненты (node_registry, mqtt_client, data_router)
- Примеры кода (обработка mesh, MQTT)
- ❌ НЕТ датчиков/дисплея/исполнителей
- ✅ Максимальная стабильность
- Тестирование mesh-сети

**Размер:** ~800 строк

---

#### ⚠️ [node_ph_ec/AI_INSTRUCTIONS.md](node_ph_ec/AI_INSTRUCTIONS.md)
**NODE pH/EC (ESP32-S3 #3+)** - КРИТИЧНЫЙ узел с автономией

**Что внутри:**
- 🚨 **САМЫЙ ВАЖНЫЙ** узел системы
- Автономная работа при потере связи
- OLED дисплей локальный (SSD1306)
- Adaptive PID контроллер
- Ring buffer 1000 записей
- Аварийные ситуации (pH < 5.0)
- Кнопка MODE (авто/ручной режим)
- ❌ НИКОГДА не останавливай PID!
- ✅ ВСЕГДА буферизуй данные offline

**Размер:** ~1000 строк (самая подробная инструкция)

---

#### 📺 [node_display/AI_INSTRUCTIONS.md](node_display/AI_INSTRUCTIONS.md)
**NODE Display (ESP32-S3 #2)** - TFT дисплей + LVGL

**Что внутри:**
- TFT 240x320 через SPI
- LVGL 8.x UI framework
- Ротационный энкодер
- Запросы данных от ROOT
- Экраны: Dashboard, Node Detail, List, Menu
- ❌ НЕТ датчиков/исполнителей
- ✅ Требует PSRAM

**Размер:** ~300 строк

---

#### 🌡️ [node_climate/AI_INSTRUCTIONS.md](node_climate/AI_INSTRUCTIONS.md)
**NODE Climate (ESP32)** - Датчики климата

**Что внутри:**
- SHT3x (температура, влажность)
- CCS811 (CO2)
- Trema Lux (освещенность)
- I2C датчики
- ❌ БЕЗ исполнителей
- ✅ Логика на ROOT (не здесь!)

**Размер:** ~200 строк

---

#### 🔌 [node_relay/AI_INSTRUCTIONS.md](node_relay/AI_INSTRUCTIONS.md)
**NODE Relay (ESP32)** - Управление климатом

**Что внутри:**
- Линейные актуаторы форточек (2 шт)
- Вентилятор (реле)
- 💡 LED свет PWM диммирование (0-100%)
- Обработка команд от ROOT
- ❌ БЕЗ датчиков
- ✅ PWM 5kHz, 8-bit

**Размер:** ~250 строк

---

#### 💧 [node_water/AI_INSTRUCTIONS.md](node_water/AI_INSTRUCTIONS.md)
**NODE Water (ESP32-C3)** - Управление водой

**Что внутри:**
- Насосы подачи/слива
- 3 соленоидных клапана (зоны)
- Датчик уровня воды
- Safety: макс 60 сек
- Логирование операций

**Размер:** ~200 строк

---

#### 📋 [node_template/AI_INSTRUCTIONS.md](node_template/AI_INSTRUCTIONS.md)
**NODE Template** - Шаблон для новых узлов

**Что внутри:**
- Как создать свой узел
- Минимальный код (mesh + телеметрия)
- Подключение common компонентов
- Чек-лист разработки
- Примеры из готовых узлов

**Размер:** ~150 строк

---

## 📊 Статистика

| Узел | AI Instructions | Размер | Сложность |
|------|-----------------|--------|-----------|
| **ROOT** | root_node/AI_INSTRUCTIONS.md | ~800 строк | ⭐⭐⭐ |
| **pH/EC** | node_ph_ec/AI_INSTRUCTIONS.md | ~1000 строк | ⭐⭐⭐⭐ |
| **Display** | node_display/AI_INSTRUCTIONS.md | ~300 строк | ⭐⭐⭐ |
| **Climate** | node_climate/AI_INSTRUCTIONS.md | ~200 строк | ⭐⭐ |
| **Relay** | node_relay/AI_INSTRUCTIONS.md | ~250 строк | ⭐⭐ |
| **Water** | node_water/AI_INSTRUCTIONS.md | ~200 строк | ⭐ |
| **Template** | node_template/AI_INSTRUCTIONS.md | ~150 строк | ⭐ |

**Всего:** ~2900 строк специфичных инструкций

---

## 🎯 Как использовать

### При работе над конкретным узлом:

1. **Открой специфичную инструкцию**
   ```bash
   # Работаешь с ROOT?
   cat root_node/AI_INSTRUCTIONS.md
   
   # Работаешь с pH/EC?
   cat node_ph_ec/AI_INSTRUCTIONS.md
   ```

2. **Следуй критичным правилам**
   - В начале каждой инструкции есть раздел "Критичные правила"
   - Это самое важное!

3. **Используй примеры кода**
   - Каждая инструкция содержит рабочие примеры
   - Адаптируй их под свои нужды

4. **Проверяй чек-листы**
   - В конце каждой инструкции есть чек-лист
   - Используй для контроля прогресса

---

## 🔍 Что где искать

### Хочу создать новый узел?
→ **[node_template/AI_INSTRUCTIONS.md](node_template/AI_INSTRUCTIONS.md)**

### Не понимаю mesh протокол?
→ **[AI_INSTRUCTIONS.md](AI_INSTRUCTIONS.md)** (общий файл)

### Хочу понять ROOT узел?
→ **[root_node/AI_INSTRUCTIONS.md](root_node/AI_INSTRUCTIONS.md)**

### Нужна автономная работа?
→ **[node_ph_ec/AI_INSTRUCTIONS.md](node_ph_ec/AI_INSTRUCTIONS.md)** (есть полная реализация)

### PWM диммирование света?
→ **[node_relay/AI_INSTRUCTIONS.md](node_relay/AI_INSTRUCTIONS.md)**

### LVGL UI?
→ **[node_display/AI_INSTRUCTIONS.md](node_display/AI_INSTRUCTIONS.md)**

---

## 📚 Дополнительная документация

### Общие:
- `common/USAGE.md` - Использование common компонентов
- `doc/MESH_HYDRO_V2_FINAL_PLAN.md` - Полный план системы
- `ARCHITECTURE.md` - Архитектура
- `SETUP_GUIDE.md` - Установка и настройка

### По узлам:
- `root_node/README.md`
- `node_ph_ec/README.md`
- `node_display/README.md`
- И т.д.

---

## 🎓 Порядок изучения (для новичков)

1. **Начни с общих правил:**
   - `AI_INSTRUCTIONS.md` (общий)
   - `ARCHITECTURE.md`

2. **Изучи common компоненты:**
   - `common/USAGE.md`
   - `common/mesh_manager/README.md`
   - `common/mesh_protocol/README.md`

3. **Пройди по узлам от простого к сложному:**
   1. `node_template/` - шаблон
   2. `node_climate/` - простой узел с датчиками
   3. `node_relay/` - узел с исполнителями
   4. `root_node/` - координатор
   5. `node_ph_ec/` - критичный узел (самый сложный)

4. **Практика:**
   - Прошей ROOT + любой простой NODE
   - Проверь mesh связь
   - Постепенно добавляй узлы

---

## ⚡ Быстрые ссылки

### 🔥 Критичные правила:
- `node_ph_ec/` → **ВСЕГДА автономная работа**
- `root_node/` → **Максимальная стабильность**
- `node_climate/` → **БЕЗ исполнителей** (только датчики)
- `node_relay/` → **БЕЗ датчиков** (только исполнители)

### 📖 Примеры кода:
- Mesh инициализация → `node_template/AI_INSTRUCTIONS.md`
- Автономная работа → `node_ph_ec/AI_INSTRUCTIONS.md`
- PWM диммирование → `node_relay/AI_INSTRUCTIONS.md`
- OLED дисплей → `node_ph_ec/AI_INSTRUCTIONS.md`
- LVGL UI → `node_display/AI_INSTRUCTIONS.md`

### 🧪 Тестирование:
Каждая AI инструкция содержит раздел "Тестирование" с конкретными шагами.

---

## 🤖 Для AI Assistant

При работе с проектом:

1. **Всегда читай специфичную инструкцию** для узла, над которым работаешь
2. **Следуй критичным правилам** в начале каждой инструкции
3. **Используй примеры кода** из инструкций
4. **Проверяй чек-листы** перед завершением
5. **Не нарушай архитектуру** (датчики где надо, исполнители где надо)

---

**Все AI инструкции созданы! Готовы к использованию!** 🚀

