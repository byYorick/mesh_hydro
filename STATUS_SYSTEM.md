# Система статусов онлайн/офлайн

## 📋 Обзор

Единая централизованная система определения статуса узлов с синхронизированными таймаутами во всех компонентах системы.

## ⚙️ Конфигурация

### Backend (Laravel)

**Файл:** `server/backend/config/hydro.php`

```php
'heartbeat_interval' => 10,        // Интервал heartbeat от ESP32 (сек)
'node_offline_timeout' => 30,      // Таймаут офлайн (сек) = 3x heartbeat
```

**Формула:** `timeout >= heartbeat_interval * 3` для надежности

### ESP32 Nodes

**Файл:** `common/mesh_config/mesh_config.h`

```c
#define HEARTBEAT_INTERVAL_MS   10000  // 10 секунд (стандарт для всех узлов)
#define MESH_NODE_TIMEOUT_MS    30000  // 30 секунд (3x heartbeat интервал)
```

### Frontend

**Файл:** `server/frontend/src/services/NodeStatusManager.ts` и `server/frontend/src/composables/useNodeStatusV2.ts`

- Конфигурация загружается с бэкенда через API `/api/status/thresholds`
- Heartbeat интервал: **10 секунд** (из конфигурации)
- Таймаут офлайн: **30 секунд** (из конфигурации)
- Пороги статуса:
  - `< 10 сек` (heartbeat interval) = **excellent**
  - `< 21 сек` (70% timeout) = **good**
  - `< 30 сек` = **warning** (poor)
  - `> 30 сек` = **offline**

## 🔄 Интервалы и таймауты

### Heartbeat

- **Интервал:** 10 секунд (все ESP32 узлы)
- **Используется в:** `node_ph`, `node_ec`, `node_ph_ec`, `node_climate`, `node_display`
- **Источник:** `HEARTBEAT_INTERVAL_MS` из `common/mesh_config/mesh_config.h`

### Таймаут офлайн

- **Backend:** 30 секунд (из `config/hydro.php`)
- **ESP32 root_node:** 30 секунд (`NODE_TIMEOUT_MS` в `node_registry.h`)
- **ESP32 mesh_config:** 30 секунд (`MESH_NODE_TIMEOUT_MS`)
- **Frontend:** 30 секунд (загружается динамически из API в `NodeStatusManager`)

### Scheduler

- **Интервал проверки:** 30 секунд (совпадает с offline timeout)
- **Файл:** `server/backend/routes/console.php`
- **Команда:** `nodes:check-status`
- **Особенности:** Запускается в фоне (`runInBackground()`) для неблокирующего выполнения

## 📊 Логика определения статуса

### Онлайн
- Узел считается **онлайн** если `last_seen_at < 30 секунд назад`
- Проверка: `now() - last_seen_at < timeout`

### Офлайн
- Узел считается **офлайн** если `last_seen_at >= 30 секунд назад` или `last_seen_at = NULL`
- Проверка: `now() - last_seen_at >= timeout` или `last_seen_at IS NULL`

### Цвет статуса (Backend)

**Метод:** `Node::getStatusColorAttribute()`

- `< 10 сек` (heartbeat interval) = **success** (зеленый)
- `< 21 сек` (70% timeout) = **success** (зеленый)
- `< 30 сек` (70-100% timeout) = **warning** (оранжевый)
- `>= 30 сек` = **error** (красный)

## 🔧 Изменения в коде

### Backend

1. **`config/hydro.php`**
   - Добавлен `heartbeat_interval = 10`
   - Изменен `node_offline_timeout = 30` (было 45)

2. **`app/Models/Node.php`**
   - `isOnline()`: использует `config('hydro.node_offline_timeout', 30)`
   - `getStatusColorAttribute()`: использует оба параметра (heartbeat_interval и timeout)
   - `scopeOnline()` и `scopeOffline()`: используют единый таймаут 30 сек

3. **`app/Console/Commands/CheckNodesStatusCommand.php`**
   - Использует `config('hydro.node_offline_timeout', 30)`

4. **`routes/console.php`**
   - Интервал scheduler: 30 секунд (совпадает с offline timeout)
   - Добавлен `runInBackground()` для неблокирующего выполнения

### ESP32 Nodes

1. **`common/mesh_config/mesh_config.h`**
   - `HEARTBEAT_INTERVAL_MS = 10000` (было 5000)
   - `MESH_NODE_TIMEOUT_MS = 30000` (было 20000)

2. **`root_node/components/node_registry/node_registry.h`**
   - `NODE_TIMEOUT_MS = 30000` (было 40000)

3. **Все узлы (`node_ph`, `node_climate`, `node_ec`, `node_ph_ec`)**
   - Добавлен `#include "mesh_config.h"`
   - `heartbeat_task()` использует `HEARTBEAT_INTERVAL_MS` вместо hardcoded значений
   - Стандартизирован интервал на 10 секунд для всех узлов

### Frontend (новая архитектура)

1. **`services/NodeStatusManager.ts`** (новый)
   - Singleton класс для централизованного управления статусами
   - Загружает конфигурацию с бэкенда через `/api/status/thresholds`
   - Вычисляет статусы узлов на основе конфигурации
   - Реактивная система подписок для обновлений статусов

2. **`composables/useNodeStatusV2.ts`** (новый)
   - Упрощенный и мощный composable
   - Использует NodeStatusManager для расчета статусов
   - Автоматически подписывается на обновления через менеджер
   - Полная типизация TypeScript

3. **`stores/nodes.ts`**
   - Интегрирован с NodeStatusManager
   - Обновляет статусы через менеджер при fetchNodes и updateNodeRealtime
   - Централизованная статистика через менеджер

4. **Компоненты мигрированы:**
   - `NodeCard.vue` → использует `useNodeStatusV2`
   - `NodeStatusIndicator.vue` → использует `useNodeStatusV2`
   - `NodeDetail.vue` → использует `useNodeStatusV2`

5. **`App.vue`**
   - Инициализирует NodeStatusManager при запуске
   - Загружает конфигурацию перед остальной инициализацией

6. **Удален старый код:**
   - `composables/useNodeStatus.ts` (старая версия)
   - `composables/useNodeStatus.js` (дубликат)
   - `stores/nodes.js` (дубликат)

## 🏗️ Frontend Architecture

### NodeStatusManager API

**Основные методы:**
- `loadConfig()`: Загружает конфигурацию с бэкенда
- `calculateStatus(node)`: Вычисляет статус узла на основе конфигурации
- `subscribe(nodeId, callback)`: Подписывается на обновления статуса узла
- `updateNodeStatus(nodeId, node)`: Обновляет статус узла и уведомляет подписчиков

**Пример использования:**
```typescript
import { nodeStatusManager } from '@/services/NodeStatusManager'
import { useNodeStatusV2 } from '@/composables/useNodeStatusV2'

// В компоненте
const nodeRef = computed(() => props.node)
const { isOnline, statusColor, statusText } = useNodeStatusV2(nodeRef)
```

### Миграция с useNodeStatus на useNodeStatusV2

**Было:**
```typescript
import { useNodeStatus } from '@/composables/useNodeStatus'
const { isOnline, statusColor } = useNodeStatus({ value: () => props.node })
```

**Стало:**
```typescript
import { useNodeStatusV2 } from '@/composables/useNodeStatusV2'
const nodeRef = computed(() => props.node)
const { isOnline, statusColor } = useNodeStatusV2(nodeRef)
```

### Диаграмма потока данных

```
Backend API (/api/status/thresholds)
         ↓
NodeStatusManager.loadConfig()
         ↓
NodeStatusManager (конфигурация в памяти)
         ↓
useNodeStatusV2 → NodeStatusManager.calculateStatus()
         ↓
Компоненты (NodeCard, NodeStatusIndicator, NodeDetail)
         ↓
WebSocket → nodesStore.updateNodeRealtime()
         ↓
NodeStatusManager.updateNodeStatus()
         ↓
Подписчики уведомляются автоматически
```

## ✅ Преимущества

1. **Единый источник истины** - все таймауты в одном месте (`mesh_config.h` и `config/hydro.php`)
2. **Надежность** - timeout = 3x heartbeat интервал предотвращает ложные срабатывания
3. **Согласованность** - все компоненты используют одинаковые значения
4. **Гибкость** - легко изменить через конфигурацию без изменения кода
5. **Централизация фронтенда** - NodeStatusManager управляет всеми статусами
6. **Реактивность** - автоматические обновления через подписки
7. **Типобезопасность** - полная поддержка TypeScript

## 🚀 После изменений

1. **Пересобрать все ESP32 узлы** с новыми значениями:
   ```bash
   cd node_ph && idf.py build flash
   cd node_climate && idf.py build flash
   cd node_ec && idf.py build flash
   cd node_ph_ec && idf.py build flash
   cd root_node && idf.py build flash
   ```

2. **Перезапустить backend** для применения новых конфигураций

3. **Проверить работу** - узлы не должны показывать офлайн когда они онлайн

## 📝 Примечания

- Таймаут 30 секунд позволяет пропустить до 2 heartbeat сообщений без ложного офлайн
- Scheduler проверяет каждые 30 секунд (совпадает с offline timeout)
- Все узлы отправляют heartbeat с одинаковым интервалом 10 секунд
- Планировщик запускается в фоне для неблокирующего выполнения

