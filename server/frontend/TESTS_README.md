# Frontend Tests

## 📋 Обзор

Фронтенд покрывается Vitest (happy-dom). Текущие тесты закрывают ключевые сценарии Growth Planner и Zone Management.

### Сводка покрытия (istanbul)

| Модуль | Stmts | Branches | Funcs | Lines |
| --- | --- | --- | --- | --- |
| `src/components/growth` | 82.84% | 77.77% | 80.39% | 83.66% |
| `src/components/EventLog.vue` | 84% | 89.28% | 72.72% | 82.6% |
| `src/components/NodeCard.vue` | 62.93% | 37.8% | 65.38% | 70.58% |
| `src/stores/nodes.ts` | 55% | 47.5% | 81.48% | 52.74% |
| `src/stores/telemetry.ts` | 65.07% | 50% | 92.85% | 63.33% |
| `src/stores/zones.ts` | 52.94% | 32.43% | 68.96% | 51.85% |
| `src/services/NodeStatusManager.ts` | 53.08% | 67.74% | 40% | 53.24% |

Пробелы: остальные stores (`app`, `events`, `settings`, `nodeConfig`), сервисы (`api.ts`, `echo.ts`, `nodes-api.ts`), утилиты и `views/*`.

## ✅ Имеющиеся тесты

### Stores
- `src/__tests__/stores/growth.test.ts`
- `src/__tests__/stores/nodes.test.js`
- `src/__tests__/stores/telemetry.test.js`
- `src/__tests__/stores/zones.test.ts`
- `src/__tests__/stores/errors.test.js`

### Services
- `src/__tests__/services/api.test.js`

### Components
- Growth: `CreateCycleDialog`, `CycleCard`, `GrowthPresetCard`, `StageTimeline`, `ZoneSelector`
- Общие: `EventLog`, `NodeCard`, `ConfigEditor`

## 🚀 Запуск тестов

```bash
# Локально (Node 18+)
npm install
npm test

# В watch режиме
npm test -- --watch

# Покрытие (./coverage/)
npm run test:coverage

# Конкретный файл
npm test -- src/__tests__/stores/zones.test.ts
```

### Через Docker compose

```powershell
cd server
docker compose -f docker-compose.dev.yml run --rm frontend sh -c "npm install && npm test"
docker compose -f docker-compose.dev.yml run --rm frontend sh -c "npm install && npm run test:coverage"
```

> `npm install` внутри контейнера обязателен: volume `/app/node_modules` создаётся заново при каждом `docker compose run --rm`.

## ⚠️ Известные проблемы

1. **Vuetify + happy-dom**: предупреждение `Right-hand side of 'instanceof' is not an object` появляется в логе. Сообщение перехватывается в `src/__tests__/setup.js`, на результаты не влияет.
2. **Глубокие компоненты Vuetify**: `VDataTable`, `VTreeview`, сложные layout-комбинации требуют точечных моков/стабов.
3. **Chart.js**: графические компоненты пока без тестов.

## 📝 TODO

- [ ] Stores: `app`, `events`, `settings`, `nodeConfig`
- [ ] Services/утилиты: `api.ts`, `echo.ts`, `nodes-api.ts`, `useOfflineMode.ts`, `usePopup.ts`
- [ ] Ключевые UI: `AdvancedChart.vue`, `TelemetryChart.vue`, `NodeManagementCard.vue`, `views/*`
- [ ] Интеграционные/E2E (Playwright/Cypress) — отдельный backlog

