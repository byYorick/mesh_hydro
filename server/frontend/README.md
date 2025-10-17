# Vue.js Frontend

Vue.js 3 + Vuetify 3 Dashboard для Mesh Hydro System

## Установка

```bash
npm install
npm run dev      # Development server
npm run build    # Production build
npm run preview  # Preview production build
```

## Структура

```
src/
├── components/         # Компоненты
│   ├── Dashboard.vue
│   ├── NodeCard.vue
│   ├── NodeDetail.vue
│   ├── LineChart.vue
│   ├── PumpControl.vue
│   ├── ClimateControl.vue
│   ├── LightDimmer.vue
│   ├── OtaManager.vue
│   └── AlertsPanel.vue
├── views/             # Страницы
│   ├── Home.vue
│   ├── Nodes.vue
│   ├── NodeDetailView.vue
│   ├── History.vue
│   ├── Analytics.vue
│   ├── Settings.vue
│   ├── Login.vue
│   └── OtaPage.vue
├── stores/            # Pinia stores
│   ├── app.js
│   ├── nodes.js
│   ├── telemetry.js
│   ├── auth.js
│   └── alerts.js
├── services/          # API сервисы
│   ├── api.js
│   └── websocket.js
├── router/            # Vue Router
│   └── index.js
├── plugins/           # Плагины
│   └── vuetify.js
├── App.vue
└── main.js
```

## Основные компоненты

### Dashboard.vue
Главный dashboard с карточками всех узлов

### NodeCard.vue
Карточка узла (pH/EC, Climate, etc)

### LineChart.vue
График линейный (pH, EC, Temp во времени)

### LightDimmer.vue
PWM диммер света (слайдер 0-100%)

### OtaManager.vue
Управление OTA обновлениями

## API

API взаимодействие через `services/api.js` (Axios)

```javascript
import api from '@/services/api'

// Получить узлы
const nodes = await api.getNodes()

// Отправить команду
await api.sendCommand(nodeId, 'run_pump', {pump: 'ph_up'})
```

## WebSocket

Real-time обновления через `services/websocket.js`

```javascript
import websocket from '@/services/websocket'

websocket.on('telemetry', (data) => {
  // Обновление данных в реальном времени
})
```

## Технологии

- Vue.js 3 (Composition API)
- Vuetify 3 (UI framework)
- Pinia (State management)
- Vue Router
- Axios (HTTP client)
- Chart.js (Графики)

