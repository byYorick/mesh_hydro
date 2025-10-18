# 🎨 Mesh Hydro System - Frontend

Vue.js 3 + Vuetify 3 Dashboard для централизованного мониторинга гидропонической mesh-системы.

## 📋 Содержание

- [Требования](#требования)
- [Установка](#установка)
- [Запуск](#запуск)
- [Структура проекта](#структура-проекта)
- [Компоненты](#компоненты)
- [API](#api)
- [Stores](#stores)
- [Конфигурация](#конфигурация)

---

## 🔧 Требования (Windows)

- **Node.js 18+** (https://nodejs.org/)
- **npm 9+** (входит в Node.js)

---

## 📦 Установка

```cmd
cd frontend
npm install
```

---

## 🚀 Запуск

### Development режим

```cmd
npm run dev
```

Приложение доступно на http://localhost:3000

### Production build

```cmd
npm run build
```

Build будет в папке `dist/`

### Preview production build

```cmd
npm run preview
```

---

## 📁 Структура проекта

```
frontend/
├── public/                 # Статические файлы
├── src/
│   ├── assets/             # CSS, изображения
│   │   └── main.css
│   │
│   ├── components/         # Переиспользуемые компоненты
│   │   ├── NodeCard.vue           # 🔌 Карточка узла
│   │   ├── TelemetryChart.vue     # 📊 График телеметрии
│   │   └── EventLog.vue           # 🔔 Лог событий
│   │
│   ├── views/              # Страницы приложения
│   │   ├── Dashboard.vue          # 📈 Главный dashboard
│   │   ├── Nodes.vue              # 🖥️ Список узлов
│   │   ├── NodeDetail.vue         # 🔍 Детали узла
│   │   ├── Telemetry.vue          # 📊 Телеметрия
│   │   ├── Events.vue             # 🔔 События
│   │   └── Settings.vue           # ⚙️ Настройки
│   │
│   ├── stores/             # Pinia stores (состояние)
│   │   ├── app.js                 # Глобальное состояние
│   │   ├── nodes.js               # Узлы mesh-сети
│   │   ├── telemetry.js           # Телеметрия
│   │   └── events.js              # События
│   │
│   ├── services/           # API и сервисы
│   │   └── api.js                 # Axios API клиент
│   │
│   ├── router/             # Vue Router
│   │   └── index.js
│   │
│   ├── plugins/            # Плагины
│   │   └── vuetify.js             # Vuetify конфигурация
│   │
│   ├── utils/              # Утилиты
│   │   └── time.js                # Форматирование времени
│   │
│   ├── App.vue             # Главный компонент
│   └── main.js             # Точка входа
│
├── index.html              # HTML шаблон
├── vite.config.js          # Vite конфигурация
└── package.json            # Зависимости
```

---

## 🎨 Компоненты

### NodeCard.vue

Карточка узла с отображением текущих данных.

**Пропсы:**
- `node` (Object) - данные узла

**События:**
- `@command` - отправка команды узлу

**Особенности:**
- Автоматическое определение типа узла (pH/EC, Climate, Water, etc)
- Отображение статуса (Online/Offline) с цветовыми индикаторами
- Последние данные телеметрии
- Быстрые действия (зависят от типа узла)

### TelemetryChart.vue

График телеметрии на основе Chart.js.

**Пропсы:**
- `title` (String) - заголовок графика
- `data` (Array) - массив данных телеметрии
- `fields` (Array) - поля для отображения на графике
- `yAxisLabel` (String) - подпись оси Y

**События:**
- `@range-changed` - изменение временного диапазона

**Особенности:**
- Временная шкала с адаптивными единицами
- Выбор периода: 1ч, 6ч, 24ч, 7д
- Интерактивные подсказки
- Плавная анимация

### EventLog.vue

Журнал событий с фильтрацией.

**Пропсы:**
- `events` (Array) - массив событий
- `limit` (Number) - максимальное количество событий

**События:**
- `@resolve` - пометить событие как решенное
- `@resolve-all` - решить все активные события
- `@delete` - удалить событие

**Особенности:**
- Фильтры: Все, Активные, Критичные
- Цветовое кодирование по уровню (info, warning, critical, emergency)
- Массовые действия

---

## 📊 Views (Страницы)

### Dashboard.vue

Главная страница с обзором системы.

**Отображает:**
- Карточки статистики (узлы online/offline, события)
- Сетка карточек всех узлов
- Последние события
- Последняя телеметрия

### Nodes.vue

Список всех узлов с фильтрацией.

**Возможности:**
- Поиск по node_id, типу, зоне
- Фильтр по типу узла
- Фильтр по статусу (online/offline)
- Отправка команд узлам

### NodeDetail.vue

Детальная информация об узле.

**Отображает:**
- Информация об узле (тип, зона, MAC, последняя связь)
- График телеметрии с выбором периода
- Таблица событий узла
- Кнопка "Назад"

### Telemetry.vue

Страница анализа телеметрии.

**Возможности:**
- Выбор узла
- Выбор поля данных
- График с выбором периода
- Таблица с сырыми данными
- Экспорт в CSV

### Events.vue

Управление событиями.

**Отображает:**
- Статистика событий (всего, активных, критичных, решенных)
- Таблица всех событий
- Фильтры: Все, Активные, Критичные, Решенные
- Массовое резолвение

### Settings.vue

Настройки системы.

**Отображает:**
- Статус системы (БД, MQTT, Telegram)
- Системная информация (PHP, Laravel версии)
- Переключатель темной темы
- Информация о приложении

---

## 🔌 API (services/api.js)

Axios клиент для взаимодействия с Laravel backend.

**Методы:**

```javascript
import api from '@/services/api'

// Health check
await api.health()

// System status
await api.getStatus()

// Dashboard
await api.getDashboardSummary()

// Nodes
await api.getNodes({ type: 'ph_ec', status: 'online' })
await api.getNode('ph_ec_001')
await api.sendCommand('ph_ec_001', 'calibrate', { ph: 7.0 })
await api.updateNodeConfig('ph_ec_001', { interval: 30 })
await api.getNodeStatistics('ph_ec_001', 24)

// Telemetry
await api.getTelemetry({ node_id: 'ph_ec_001', hours: 24 })
await api.getLatestTelemetry()
await api.getAggregateTelemetry('ph_ec_001', 'ph', 24, '1hour')
await api.exportTelemetry({ node_id: 'ph_ec_001' })

// Events
await api.getEvents({ status: 'active', level: 'critical' })
await api.resolveEvent(123)
await api.resolveBulkEvents([123, 456, 789])
await api.getEventStatistics(24)
```

**Автоматически:**
- Добавляет auth token из localStorage
- Обрабатывает ошибки 401 (redirect на login)
- Распаковывает response.data

---

## 📦 Stores (Pinia)

### useAppStore

Глобальное состояние приложения.

**Состояние:**
- `theme` - текущая тема (light/dark)
- `drawer` - состояние навигационного меню
- `systemStatus` - статус системы
- `dashboardSummary` - данные dashboard
- `snackbar` - уведомления
- `connected` - статус подключения к backend

**Действия:**
- `toggleTheme()` - переключить тему
- `showSnackbar(message, color)` - показать уведомление
- `fetchSystemStatus()` - загрузить статус системы
- `fetchDashboardSummary()` - загрузить данные dashboard

### useNodesStore

Управление узлами.

**Состояние:**
- `nodes` - массив всех узлов
- `selectedNode` - выбранный узел
- `loading` - загрузка
- `error` - ошибки

**Геттеры:**
- `onlineNodes` - только online узлы
- `offlineNodes` - только offline узлы
- `nodesByType(type)` - узлы по типу
- `getNodeById(nodeId)` - узел по ID

**Действия:**
- `fetchNodes()` - загрузить все узлы
- `fetchNode(nodeId)` - загрузить узел
- `sendCommand(nodeId, command, params)` - отправить команду
- `updateConfig(nodeId, config)` - обновить конфигурацию
- `updateNodeRealtime(data)` - обновить в реальном времени (WebSocket)

### useTelemetryStore

Управление телеметрией.

**Состояние:**
- `telemetry` - массив данных телеметрии
- `latestTelemetry` - последние данные от каждого узла
- `aggregatedData` - агрегированные данные

**Действия:**
- `fetchTelemetry(params)` - загрузить телеметрию
- `fetchLatestTelemetry()` - последняя телеметрия
- `fetchAggregatedData(nodeId, field, hours)` - агрегированные данные
- `exportTelemetry(params)` - экспорт в CSV

### useEventsStore

Управление событиями.

**Состояние:**
- `events` - массив событий
- `statistics` - статистика событий

**Геттеры:**
- `activeEvents` - активные события
- `resolvedEvents` - решенные события
- `criticalEvents` - критичные события
- `criticalCount` - количество критичных

**Действия:**
- `fetchEvents(params)` - загрузить события
- `resolveEvent(eventId)` - решить событие
- `resolveBulk(eventIds)` - массовое резолвение
- `deleteEvent(eventId)` - удалить событие

---

## ⚙️ Конфигурация

### vite.config.js

```javascript
export default defineConfig({
  server: {
    port: 3000,
    proxy: {
      '/api': {
        target: 'http://localhost:8000',
        changeOrigin: true
      }
    }
  }
})
```

### Environment Variables

Создайте файл `.env`:

```env
VITE_API_URL=http://localhost:8000/api
VITE_WS_URL=ws://localhost:6001
```

---

## 🎨 Темы Vuetify

Приложение поддерживает светлую и темную темы.

Переключение через:
- Кнопка в AppBar
- Store: `appStore.toggleTheme()`
- Автоматически сохраняется в localStorage

---

## 🔧 Полезные команды

```cmd
# Установка зависимостей
npm install

# Запуск dev сервера
npm run dev

# Build для production
npm run build

# Preview production build
npm run preview

# Lint
npm run lint

# Format code
npm run format
```

---

## 📱 Адаптивность

Приложение полностью адаптивно и работает на:
- Desktop (1920x1080+)
- Laptop (1366x768+)
- Tablet (768x1024)
- Mobile (375x667+)

Breakpoints (Vuetify):
- xs: 0-600px
- sm: 600-960px
- md: 960-1280px
- lg: 1280-1920px
- xl: 1920px+

---

## 🧪 Тестирование

```cmd
# Запустить тесты
npm test

# E2E тесты
npm run test:e2e

# Coverage
npm run test:coverage
```

---

## 📚 Технологии

- **Vue.js 3** - Progressive JavaScript Framework (Composition API)
- **Vuetify 3** - Material Design Component Framework
- **Vite** - Next Generation Frontend Tooling
- **Pinia** - Intuitive State Management
- **Vue Router** - Official Router
- **Axios** - Promise based HTTP client
- **Chart.js** - Simple yet flexible JavaScript charting
- **Material Design Icons** - Icon library

---

## 🔗 Связанные проекты

- Backend API: `../backend/`
- ROOT Node (ESP32): `../../root_node/`
- Документация: `../docs/`

---

## 🐛 Troubleshooting

### Ошибка подключения к API

**Проблема:** `Network Error` или `CORS error`

**Решение:**
1. Убедитесь что backend запущен на порту 8000
2. Проверьте `.env`: `VITE_API_URL=http://localhost:8000/api`
3. Перезапустите dev сервер: `npm run dev`

### Не обновляются данные

**Проблема:** Данные не обновляются автоматически

**Решение:**
1. Проверьте что backend MQTT listener запущен
2. Проверьте консоль браузера на ошибки
3. Принудительное обновление: кнопка Refresh в AppBar

### Медленная загрузка графиков

**Проблема:** Графики Chart.js медленно загружаются

**Решение:**
1. Уменьшите период отображения (1ч вместо 7д)
2. Используйте агрегированные данные
3. Включите lazy loading для графиков

---

**Версия:** 2.0.0  
**Vue.js:** 3.3+  
**Vuetify:** 3.3+  
**Node.js:** 18+

