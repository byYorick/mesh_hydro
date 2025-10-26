# Документация для AI - Mesh Hydro System

**Версия:** 2.0  
**Дата:** 2025-01-26  
**Статус:** Production Ready  

## 📋 Оглавление

1. [Обзор проекта](#обзор-проекта)
2. [Архитектура](#архитектура)
3. [Технологии](#технологии)
4. [Структура проекта](#структура-проекта)
5. [Backend](#backend)
6. [Frontend](#frontend)
7. [ESP32 узлы](#esp32-узлы)
8. [Системы управления](#системы-управления)
9. [API](#api)
10. [Развертывание](#развертывание)

---

## Обзор проекта

**Mesh Hydro System** - IoT система управления гидропонной установкой с mesh-сетью ESP32 узлов.

### Основные возможности

- 🌿 **Управление гидропоникой** - pH, EC, температура, влажность
- 🌐 **Mesh сеть** - ESP32 Mesh для соединения узлов
- 📊 **Real-time мониторинг** - WebSocket + MQTT
- 🎛️ **PID контроль** - Адаптивный PID для автоматической коррекции
- 📱 **Web Dashboard** - Vue 3 + Vuetify
- 🔄 **Real-time события** - Laravel Echo + Pusher/Reverb
- 🐳 **Docker** - Containerized deployment

### Топология сети

```
┌─────────────┐
│  Root Node  │ (ESP32 - шлюз)
│   (ESP32)   │
└──────┬──────┘
       │ Mesh Network
       │
   ┌───┴────────┬────────┬────────┐
   │            │        │        │
┌──▼──┐    ┌───▼───┐ ┌──▼──┐ ┌──▼──┐
│ pH  │    │  EC   │ │Clim │ │Relay│
│ Node│    │ Node  │ │Node │ │Node │
└─────┘    └───────┘ └─────┘ └─────┘
```

---

## Архитектура

### 3-уровневая архитектура

#### 1. Hardware Level (ESP32)
- **Root Node** - ESP32 mesh шлюз с климатом
- **pH/EC Node** - ESP32 с pH/EC датчиками
- **Climate Node** - ESP32 с датчиками климата
- **Relay Node** - ESP32 с реле
- **Display Node** - ESP32 с дисплеем

#### 2. Backend (Laravel)
- **API** - REST API для frontend
- **MQTT Client** - Подключение к MQTT брокеру
- **Database** - PostgreSQL
- **WebSocket** - Laravel Echo + Reverb
- **Business Logic** - Обработка данных, события, команды

#### 3. Frontend (Vue.js)
- **Dashboard** - Главная страница
- **Nodes** - Список узлов
- **Events** - Лог событий
- **Settings** - Настройки
- **Real-time** - WebSocket обновления

### Поток данных

```
ESP32 Node ──> Mesh ──> Root Node ──> MQTT ──> Laravel ──> PostgreSQL
                                                          └─> WebSocket ──> Vue.js
```

---

## Технологии

### Backend
- **Laravel 11** - PHP framework
- **PostgreSQL** - База данных
- **MQTT** (Mosquitto) - Брокер
- **Laravel Echo** + **Reverb** - WebSocket
- **Docker** - Containerization

### Frontend
- **Vue 3** - Progressive Web Framework
- **Vuetify 3** - Material Design
- **Pinia** - State management
- **Vue Router** - Routing
- **Axios** - HTTP client
- **Laravel Echo** - WebSocket client
- **TypeScript** - Type safety
- **Vite** - Build tool

### Hardware
- **ESP32** - Microcontroller
- **ESP-IDF v5.5** - Framework
- **ESP-MESH** - Mesh networking
- **C/C++** - Firmware

---

## Структура проекта

```
mesh_hydro/
├── server/                 # Backend (Laravel)
│   ├── backend/           # Laravel application
│   │   ├── app/           # Application logic
│   │   ├── config/        # Configuration
│   │   └── routes/        # API routes
│   └── frontend/          # Frontend (Vue.js)
│       ├── src/           # Source code
│       ├── public/        # Public assets
│       └── nginx.conf     # Nginx config
├── root_node/             # Root Node firmware
├── node_ph_ec/            # pH/EC Node firmware
├── node_climate/          # Climate Node firmware
├── node_relay/            # Relay Node firmware
├── node_display/          # Display Node firmware
├── common/                # Shared code
│   ├── mesh_manager/      # Mesh networking
│   ├── mesh_protocol/     # Protocol
│   ├── sensor_base/       # Sensor base
│   └── actuator_base/     # Actuator base
├── doc/                   # Documentation
├── tools/                 # Build tools
└── docker-compose.yml     # Docker compose
```

---

## Backend

### API Endpoints

#### Nodes
- `GET /api/nodes` - Список узлов
- `GET /api/nodes/{id}` - Детали узла
- `POST /api/nodes` - Добавить узел
- `PUT /api/nodes/{id}` - Обновить узел
- `DELETE /api/nodes/{id}` - Удалить узел

#### Telemetry
- `POST /api/telemetry` - Отправить телеметрию
- `GET /api/telemetry/{nodeId}` - Получить телеметрию

#### Commands
- `POST /api/nodes/{id}/commands` - Отправить команду
- `GET /api/commands` - Список команд

#### Events
- `GET /api/events` - Список событий
- `POST /api/events` - Создать событие

### MQTT Topics

```
hydro/discovery/{node_id}     - Discovery
hydro/telemetry/{node_id}     - Telemetry
hydro/status/{node_id}        - Status
hydro/commands/{node_id}      - Commands
hydro/response/{node_id}      - Responses
```

### WebSocket Channels

```javascript
// Global channel
echo.channel('hydro-system')
  .listen('.telemetry.received', handleTelemetry)
  .listen('.node.status.changed', handleNodeStatus)
  .listen('.event.created', handleEvent)

// Node channel
echo.private(`hydro.node.${nodeId}`)
  .listen('.command.response', handleResponse)
```

### Database Schema

#### Nodes
- `id` - Primary key
- `node_id` - Unique ID (e.g., "ph_ec_001")
- `node_type` - Type (ph_ec, climate, relay, etc.)
- `zone` - Zone name
- `online` - Online status
- `last_seen_at` - Last contact
- `mac_address` - MAC address

#### Telemetry
- `id` - Primary key
- `node_id` - Foreign key
- `data` - JSON data
- `received_at` - Timestamp

#### Events
- `id` - Primary key
- `node_id` - Foreign key
- `level` - Level (info, warning, error, critical)
- `message` - Message
- `data` - JSON data
- `resolved_at` - Resolution time

---

## Frontend

### Структура

```
frontend/src/
├── components/         # Vue components
│   ├── ui/            # UI components
│   ├── NodeCard.vue   # Node card
│   └── ...
├── composables/       # Composables
│   ├── useDialog.ts   # Dialog manager
│   └── useNodeStatus.ts # Node status
├── stores/            # Pinia stores
│   ├── nodes.js       # Nodes store
│   └── events.js      # Events store
├── services/          # Services
│   ├── api.js         # API client
│   └── echo.js        # WebSocket
├── views/             # Views
│   ├── Dashboard.vue  # Dashboard
│   ├── Nodes.vue      # Nodes list
│   └── Events.vue     # Events
└── main.js            # Entry point
```

### Ключевые компоненты

#### useDialog.ts
Централизованная система попапов:

```typescript
const dialog = useDialog()

// Confirm
dialog.confirm('Delete node?', 'Confirm', async () => {
  await deleteNode()
})

// Success
dialog.success('Node deleted')

// Error
dialog.error('Failed to delete')
```

#### useNodeStatus.ts
Система статусов узлов:

```typescript
const status = useNodeStatus(nodeRef)

// Quality: 'excellent' | 'good' | 'poor' | 'offline'
status.connectionQuality.value

// Seconds since last contact
status.secondsSinceLastSeen.value

// Time until offline
status.timeUntilOffline.value

// Can perform actions?
status.canPerformActions.value
```

### Системы

#### Централизованные системы

1. **Dialog System** (`useDialog.ts`)
   - Подтверждения
   - Уведомления
   - Ошибки
   - Success сообщения

2. **Node Status System** (`useNodeStatus.ts`)
   - Качество связи (4 уровня)
   - Тайминги
   - Проверки доступности

3. **WebSocket System** (`echo.js`)
   - Real-time обновления
   - Автоматическое переподключение
   - Fallback polling

---

## ESP32 узлы

### Root Node

**Функции:**
- Mesh шлюз
- Климат-контроль (fallback)
- MQTT клиент
- Обнаружение узлов

**Pins:**
- `GPIO_DS18B20` - DS18B20 (temp)
- `GPIO_DHT22` - DHT22 (temp/humidity)

### pH/EC Node

**Функции:**
- pH датчик (pH-4502C)
- EC датчик (TDS)
- 3 насоса (pH Up, pH Down, EC Up)
- PID контроль

**Pins:**
- `GPIO_PH_SENSOR` - pH sensor
- `GPIO_EC_SENSOR` - EC sensor
- `GPIO_PH_UP_PUMP` - pH Up pump
- `GPIO_PH_DOWN_PUMP` - pH Down pump
- `GPIO_EC_UP_PUMP` - EC Up pump

**Команды:**
```json
{
  "command": "run_pump",
  "params": {
    "pump": "ph_up",
    "duration": 5
  }
}
```

### Climate Node

**Функции:**
- Температура (DS18B20, DHT22)
- Влажность (DHT22)
- CO2 (MH-Z19B)
- Освещенность (BH1750)

### Relay Node

**Функции:**
- Управление реле
- Окна/вентиляторы
- Освещение

### Display Node

**Функции:**
- OLED дисплей
- Отображение статуса
- Меню навигации

---

## Системы управления

### 1. Централизованная система диалогов

**Использование:**
```typescript
import { useDialog } from '@/composables/useDialog'

const dialog = useDialog()

// Подтверждение
dialog.confirm('Delete?', 'Confirm', async () => {
  await delete()
})

// Успех
dialog.success('Deleted!')

// Ошибка
dialog.error('Failed!')
```

**Типы:**
- `confirm()` - Подтверждение
- `alert()` - Уведомление
- `success()` - Успех
- `error()` - Ошибка
- `warning()` - Предупреждение

### 2. Система статусов узлов

**Использование:**
```typescript
import { useNodeStatus } from '@/composables/useNodeStatus'

const nodeRef = ref(node)
const status = useNodeStatus(nodeRef)

// Качество связи
status.connectionQuality.value // 'excellent' | 'good' | 'poor' | 'offline'

// Детали
status.secondsSinceLastSeen.value // 5
status.timeUntilOffline.value // 15

// UI
status.statusColor.value // 'success' | 'warning' | 'error'
status.statusText.value // "Онлайн" | "Подключение слабое"
```

**Компонент:**
```vue
<NodeStatusIndicator :node="node" />
```

### 3. WebSocket система

**Использование:**
```javascript
import { initializeEcho } from '@/services/echo'

// Initialize
const echo = initializeEcho()

// Subscribe
echo.channel('hydro-system')
  .listen('.telemetry.received', (data) => {
    // Handle telemetry
  })
```

---

## API

### Nodes

#### GET /api/nodes
Получить список узлов.

**Query params:**
- `type` - Фильтр по типу
- `status` - Фильтр по статусу (online/offline)

**Response:**
```json
[
  {
    "id": 1,
    "node_id": "ph_ec_001",
    "node_type": "ph_ec",
    "zone": "Grow Room 1",
    "online": true,
    "last_seen_at": "2025-01-26T10:00:00Z",
    "status_color": "success",
    "icon": "mdi-flask"
  }
]
```

#### POST /api/nodes/{id}/commands
Отправить команду узлу.

**Request:**
```json
{
  "command": "run_pump",
  "params": {
    "pump": "ph_up",
    "duration": 5
  }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Command sent",
  "command_id": 123
}
```

### Telemetry

#### POST /api/telemetry
Отправить телеметрию.

**Request:**
```json
{
  "node_id": "ph_ec_001",
  "data": {
    "ph": 6.5,
    "ec": 2.4,
    "temperature": 21.5
  }
}
```

---

## Развертывание

### Docker

#### Сборка
```bash
cd server
docker-compose build
```

#### Запуск
```bash
docker-compose up -d
```

#### Остановка
```bash
docker-compose down
```

### Настройки

#### Frontend (.env)
```
VITE_API_URL=http://localhost:8000
VITE_WS_HOST=localhost
VITE_WS_PORT=8080
```

#### Backend (.env)
```
DB_CONNECTION=pgsql
DB_HOST=postgres
DB_DATABASE=hydro
MQTT_HOST=mosquitto
MQTT_PORT=1883
REVERB_HOST=reverb
REVERB_PORT=8080
```

### ESP-IDF окружение

**Windows:**
```bash
C:\Windows\system32\cmd.exe /k "C:\Espressif\idf_cmd_init.bat" esp-idf-1dcc643656a1439837fdf6ab63363005
```

**Target:** ESP32  
**Version:** ESP-IDF v5.5  
**Path:** C:\Espressif\

### Сборка узлов

**Root Node:**
```bash
cd root_node
idf.py build flash
```

**pH/EC Node:**
```bash
cd node_ph_ec
idf.py build flash
```

**Climate Node:**
```bash
cd node_climate
idf.py build flash
```

---

## Ключевые концепции

### 1. Mesh Network
- ESP-MESH для связи между узлами
- Root Node как шлюз
- Автоматическое обнаружение узлов

### 2. Real-time Updates
- WebSocket для real-time обновлений
- MQTT для hardware communication
- Polling fallback при недоступности WebSocket

### 3. PID Control
- Адаптивный PID для pH/EC
- Автоматическая коррекция
- Автонастройка параметров

### 4. Status System
- 4 уровня качества связи
- Автоматическая проверка доступности
- Детальная информация о таймингах

### 5. Dialog System
- Единый менеджер диалогов
- Автоматический loading
- Обработка ошибок

---

## Best Practices

### Frontend
1. Используйте `useDialog()` вместо прямых v-dialog
2. Используйте `useNodeStatus()` для статусов
3. Проверяйте `canPerformActions` перед действиями
4. Обрабатывайте errors в async операциях
5. Используйте TypeScript для типизации

### Backend
1. Используйте Eloquent ORM
2. Валидируйте все inputs
3. Обрабатывайте errors gracefully
4. Логируйте важные события
5. Используйте events для decoupling

### Hardware
1. Проверяйте online статус перед командами
2. Используйте watchdog для stability
3. Логируйте ошибки через ESP_LOG
4. Проверяйте timeouts
5. Используйте магистрали для датчиков

---

## Troubleshooting

### Frontend не загружается
```bash
# Проверка контейнера
docker-compose logs frontend

# Пересборка
docker-compose build frontend --no-cache
docker-compose up -d frontend
```

### WebSocket не подключается
```bash
# Проверка Reverb
docker-compose logs reverb

# Проверка backend
docker-compose logs backend
```

### MQTT не работает
```bash
# Проверка Mosquitto
docker-compose logs mosquitto

# Проверка подключения
mosquitto_sub -h localhost -t hydro/#
```

### ESP32 не подключается
```bash
# Проверка Mesh сети
idf.py monitor

# Сброс ESP32
idf.py flash

# Проверка логов
grep "Mesh" log.txt
```

---

## Важные файлы

### Backend
- `server/backend/app/Models/Node.php` - Node модель
- `server/backend/app/Http/Controllers/NodeController.php` - API
- `server/backend/app/Services/MqttService.php` - MQTT сервис

### Frontend
- `server/frontend/src/main.js` - Entry point
- `server/frontend/src/App.vue` - Главный компонент
- `server/frontend/src/stores/nodes.js` - Nodes store
- `server/frontend/src/composables/useDialog.ts` - Dialog manager
- `server/frontend/src/composables/useNodeStatus.ts` - Status manager
- `server/frontend/src/services/echo.js` - WebSocket client

### Hardware
- `root_node/main/app_main.c` - Root Node main
- `node_ph_ec/main/main.c` - pH/EC Node main
- `common/mesh_manager/mesh_manager.c` - Mesh manager

---

## Тестирование

### API тесты
```bash
cd server/backend
php artisan test
```

### Frontend тесты
```bash
cd server/frontend
npm run test
```

### Hardware тесты
```bash
# Root Node
cd root_node
idf.py build flash monitor

# pH/EC Node
cd node_ph_ec
idf.py build flash monitor
```

---

## Git Workflow

### Основные ветки
- `main` - Production
- `develop` - Development
- `feature/*` - Features
- `bugfix/*` - Bug fixes

### Commit convention
```
feat: add new feature
fix: fix bug
docs: update documentation
refactor: refactor code
test: add tests
```

---

## Monitoring

### Логи
```bash
# Frontend
docker-compose logs -f frontend

# Backend
docker-compose logs -f backend

# MQTT
docker-compose logs -f mosquitto
```

### Metrics
- Node online/offline status
- Response times
- Error rates
- WebSocket connections

---

## Security

### Authentication
- Laravel Sanctum для API
- JWT токены
- Token refresh

### Validation
- Backend validation для всех endpoints
- Frontend validation для форм
- SQL injection protection

### CORS
- Настроен для localhost
- Production requires HTTPS

---

## Performance

### Frontend
- Code splitting
- Lazy loading
- Image optimization
- Caching

### Backend
- Query optimization
- Caching
- Indexing
- Connection pooling

---

## Backup

### Database
```bash
docker exec hydro_postgres pg_dump -U hydro hydro > backup.sql
```

### Restore
```bash
docker exec -i hydro_postgres psql -U hydro hydro < backup.sql
```

---

## Лицензия

MIT License

---

## Контакты

- **Project:** Mesh Hydro System
- **Version:** 2.0
- **Framework:** Laravel 11 + Vue 3 + ESP-IDF v5.5

---

## Последнее обновление

**Дата:** 2025-01-26  
**Изменения:**
- ✅ Добавлена система диалогов
- ✅ Улучшена система статусов
- ✅ Удален VSkeletonLoader
- ✅ Улучшен WebSocket logging
- ✅ Добавлена полная TypeScript типизация

---

**Следующие шаги:**
1. Интегрировать NodeStatusIndicator в компоненты
2. Мигрировать существующие диалоги
3. Добавить исторические данные для uptime
4. Добавить графики качества связи
