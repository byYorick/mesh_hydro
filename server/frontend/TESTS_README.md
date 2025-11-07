# Frontend Tests

## 📋 Обзор

Фронтенд покрывается Vitest (happy-dom). Текущие тесты закрывают ключевые сценарии Growth Planner, Zone Management, базовые store-операции и ряд утилит.

### Сводка покрытия (istanbul)

| Модуль | Stmts | Branches | Funcs | Lines |
| --- | --- | --- | --- | --- |
| `src/components/growth` | 82.84% | 77.77% | 80.39% | 83.66% |
| `src/components/EventLog.vue` | 84% | 89.28% | 72.72% | 82.6% |
| `src/components/NodeCard.vue` | 62.93% | 37.8% | 65.38% | 70.58% |
| `src/components/zones/ZoneCard.vue` | 84.09% | 70.68% | 77.27% | 87.5% |
| `src/stores/app.ts` | 69.23% | 67.34% | 64.28% | 69.86% |
| `src/stores/events.ts` | 55.38% | 66.66% | 57.69% | 56.19% |
| `src/stores/settings.js` | 65.38% | 95.23% | 61.11% | 67.34% |
| `src/composables/useOfflineMode.ts` | 57.5% | 68% | 42.85% | 58.97% |
| `src/services/nodes-api.js` | 100% | 100% | 100% | 100% |
| `src/utils/logger.ts` | 68.85% | 54.54% | 48.14% | 76% |
| `src/utils/time.ts` | 95% | 90% | 100% | 100% |

Пробелы: сервисы `api.ts`, `echo.ts`, сложные компоненты (Chart.js, node-config модальные), основные `views/*`, утилиты `usePopup`, `error-handler`, `useDebounce` и др.

## ✅ Имеющиеся тесты

### Stores
- `src/__tests__/stores/growth.test.ts`
- `src/__tests__/stores/nodes.test.js`
- `src/__tests__/stores/telemetry.test.js`
- `src/__tests__/stores/zones.test.ts`
- `src/__tests__/stores/errors.test.js`
- `src/__tests__/stores/app.test.ts`
- `src/__tests__/stores/events.test.ts`
- `src/__tests__/stores/settings.test.ts`
- `src/__tests__/stores/nodeConfig.test.ts`

### Services / Composables / Utils
- `src/__tests__/services/api.test.js`
- `src/__tests__/services/nodes-api.test.ts`
- `src/__tests__/composables/useOfflineMode.test.ts`
- `src/__tests__/utils/array.test.ts`
- `src/__tests__/utils/logger.test.ts`
- `src/__tests__/utils/time.test.ts`

### Components / Views
- Growth: `CreateCycleDialog`, `CycleCard`, `GrowthPresetCard`, `StageTimeline`, `ZoneSelector`
- Общие: `EventLog`, `NodeCard`, `ConfigEditor`
- Zones: `ZoneCard`, `views/Zones.vue`

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

1. **Vuetify + happy-dom**: предупреждение `Right-hand side of 'instanceof' is not an object` остаётся в логах (см. `src/__tests__/setup.js`).
2. **Глубокие компоненты Vuetify**: `VDataTable`, `VTreeview`, сложные layout-комбинации требуют точечных моков/стабов.
3. **Chart.js**: графические компоненты (`TelemetryChart`, `AdvancedChart`) пока без тестов.

## 📝 TODO

- [ ] Services: `api.ts`, `echo.ts`, `nodes-api.ts` (TypeScript версия) + realtime-слой.
- [ ] Composables/утилиты: `usePopup.ts`, `error-handler.ts`, `useDebounce.ts`, `useResponsive.ts`.
- [ ] Ключевые UI: `AdvancedChart.vue`, `TelemetryChart.vue`, node-config диалоги, странички `views/*` кроме `Zones`.
- [ ] Интеграционные/E2E (Playwright/Cypress) — отдельный backlog.

