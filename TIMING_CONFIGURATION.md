# Конфигурация таймингов системы Mesh Hydro

## 📊 Текущие настройки

### pH Node (`node_ph`)

| Параметр | Интервал | Файл | Комментарий |
|----------|----------|------|-------------|
| **Чтение датчика** | 10 сек | `ph_manager.c:175` | Чтение pH и управление |
| **Отправка телеметрии** | 30 сек | `ph_manager.c:187` | ✅ Основной источник жизни |
| **Heartbeat** | 15 сек | `ph_manager.c:201` | ✅ Исправлено с 60→15 сек |
| **Main loop delay** | 1 сек | `ph_manager.c:192` | CPU не загружен |

### Root Node

| Параметр | Интервал | Файл | Комментарий |
|----------|----------|------|-------------|
| **Node timeout** | 40 сек | `node_registry.h:22` | ✅ Больше телеметрии |
| **Check interval** | ~5 сек | `node_registry.c` | Проверка offline узлов |

### Backend (`server/backend`)

| Параметр | Интервал | Файл | Комментарий |
|----------|----------|------|-------------|
| **Node offline timeout** | 45 сек | `config/hydro.php` | ✅ Больше root timeout |

---

## 🔄 Цепочка обновлений

```
pH Node         Root Node       Backend
   |               |               |
   |-- Telemetry --|               |
   |    (30 сек)   |               |
   |               |               |
   |-- Heartbeat --|               |
   |    (15 сек)   |               |
   |               |               |
   |               |-- MQTT ------->|
   |               |    (instant)  |
   |               |               |
   |               |-- Check ------|
   |               | (если >40 сек)|
   |               | → OFFLINE     |
   |               |               |
   |               |               |-- Check ---|
   |               |               | (если >45 сек)
   |               |               | → OFFLINE
```

---

## ⏱️ Расчет таймингов

### Минимальные интервалы:

**Чтобы узел не уходил в offline, нужно:**

```
heartbeat_interval < node_timeout < backend_timeout
```

### Текущие значения (ИСПРАВЛЕНО):

```
15 сек < 40 сек < 45 сек  ✅
```

**Margin of safety:**
- Root node: **40 - 15 = 25 секунд** запаса
- Backend: **45 - 40 = 5 секунд** запаса

### Предыдущие значения (ПРОБЛЕМА):

```
60 сек > 40 сек  ❌ TIMEOUT!
```

**Проблема:** Heartbeat отправлялся раз в **60 секунд**, а Root ждал только **40 секунд**.

---

## 🚨 Диагностика TIMEOUT

### Если узел уходит в OFFLINE:

**1. Проверить логи pH ноды:**
```
I (XXX) ph_manager: ✅ Telemetry sent: pH=7.00, autonomous=1
I (XXX) ph_manager: 💓 Heartbeat sent
```

**2. Проверить логи Root ноды:**
```
I (XXX) data_router: 📊 Telemetry from ph_3f0c00 → MQTT
I (XXX) node_registry: Node ph_3f0c00 is now ONLINE
```

**3. Если Root показывает TIMEOUT:**
```
W (XXX) node_registry: Node ph_3f0c00 TIMEOUT -> OFFLINE (elapsed: 23667 ms)
```

**Причины:**
- ❌ pH нода не подключена к mesh
- ❌ pH нода не отправляет heartbeat/telemetry
- ❌ Root не получает сообщения (проблема mesh)
- ❌ Интервал heartbeat > node_timeout

---

## 🔧 Рекомендуемые настройки

### Оптимальные интервалы:

| Узел | Heartbeat | Telemetry | Timeout |
|------|-----------|-----------|---------|
| **pH node** | **15 сек** | **30 сек** | - |
| **EC node** | **15 сек** | **30 сек** | - |
| **Climate node** | **15 сек** | **30 сек** | - |
| **Water node** | **15 сек** | **30 сек** | - |
| **Root node** | - | - | **40 сек** |
| **Backend** | - | - | **45 сек** |

### Правило безопасности:

```
heartbeat_interval = node_timeout / 2.5
```

**Пример:**
- Node timeout = 40 сек
- Heartbeat = 40 / 2.5 = **16 сек** ≈ **15 сек** ✅

**Почему коэффициент 2.5?**
- Гарантирует **минимум 2 попытки** отправки heartbeat до timeout
- Учитывает возможные задержки mesh сети
- Запас на джиттер и потери пакетов

---

## 📈 Оптимизация под нагрузку

### Для production:

**Если mesh стабильна:**
```
Heartbeat: 20 сек
Telemetry: 30 сек
Timeout: 50 сек
Backend: 60 сек
```

**Если mesh нестабильна:**
```
Heartbeat: 10 сек  ← более частые проверки
Telemetry: 20 сек
Timeout: 30 сек
Backend: 35 сек
```

**Компромисс:**
- ↓ Интервал heartbeat = ↑ Надежность, но ↑ Трафик mesh
- ↑ Интервал heartbeat = ↓ Трафик, но ↓ Скорость обнаружения offline

---

## 🧪 Тестирование таймингов

### Тест 1: Нормальная работа

1. Запустите pH ноду
2. Смотрите логи Root:
```
I (XXX) node_registry: Node ph_3f0c00 is now ONLINE
```
3. Ждите 20 секунд
4. Должен прийти heartbeat:
```
I (XXX) data_router: 💓 Heartbeat from ph_3f0c00 → MQTT
```
5. ✅ Узел должен оставаться ONLINE

---

### Тест 2: Потеря связи

1. Запустите pH ноду
2. Отключите её (выдерните USB)
3. Смотрите логи Root:
```
W (XXX) node_registry: Node ph_3f0c00 TIMEOUT -> OFFLINE (elapsed: 40XXX ms)
```
4. ✅ Timeout должен произойти через **~40-45 секунд**

---

### Тест 3: Восстановление связи

1. После Теста 2
2. Подключите pH ноду обратно
3. Смотрите логи Root:
```
I (XXX) node_registry: Node ph_3f0c00 is now ONLINE
```
4. ✅ Узел должен вернуться в ONLINE через **~15-30 секунд**

---

## 📝 История изменений

### 2025-10-28: Исправление heartbeat интервала

**Проблема:**
```
W (196065370) node_registry: Node ph_3f0c00 TIMEOUT -> OFFLINE (elapsed: 23667 ms)
```

**Причина:**
- Heartbeat отправлялся раз в **60 секунд**
- Root timeout был **40 секунд**
- **60 > 40** → TIMEOUT!

**Решение:**
- ✅ Изменил heartbeat с **60 сек → 15 сек**
- ✅ Теперь **15 < 40** → Работает!

**Файлы:**
- `node_ph/components/ph_manager/ph_manager.c:201`

---

## 🎯 Рекомендации

### Для всех нод:

1. **Heartbeat интервал:** **15 секунд** (универсально)
2. **Telemetry интервал:** **30 секунд** (основные данные)
3. **Root node timeout:** **40 секунд** (проверка offline)
4. **Backend timeout:** **45 секунд** (финальная проверка)

### Мониторинг:

Добавить метрики:
- Время между heartbeat'ами
- Количество пропущенных heartbeat'ов
- Средняя задержка mesh сети
- Количество false-positive offline

---

## ✅ Чек-лист перед запуском

- [ ] Heartbeat interval < Node timeout
- [ ] Node timeout < Backend timeout
- [ ] Коэффициент запаса ≥ 2.0
- [ ] Тестирование offline detection
- [ ] Тестирование recovery
- [ ] Логи показывают регулярные heartbeat'ы
- [ ] Узлы не уходят в OFFLINE без причины

---

## 🔗 Связанные файлы

- `node_ph/components/ph_manager/ph_manager.c` - таймеры pH ноды
- `root_node/components/node_registry/node_registry.h` - timeout root
- `server/backend/config/hydro.php` - timeout backend
- `TROUBLESHOOTING_CONFIG.md` - диагностика проблем


