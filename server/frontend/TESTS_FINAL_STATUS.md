# Итоговый статус Frontend тестов

## ✅ Покрытие

- 25 файлов тестов (stores, services, компоненты growth/zones, utils, composables, views)
- ~200 assert-случаев, стабильный прогон `vitest --run` и Docker-команд
- HTML/JSON отчёты покрытия: `server/frontend/coverage/`

## 📊 Покрытие (istanbul)

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

## ⚠️ Ограничения

1. **Vuetify + happy-dom**: предупреждение `Right-hand side of 'instanceof' is not an object` перехватывается в `src/__tests__/setup.js`, но остаётся в логе.
2. **Chart.js**: графические компоненты (`TelemetryChart`, `AdvancedChart`, графики в `NodeDetail`) пока без моков.
3. **Views**: большинство страниц (`Dashboard`, `Nodes`, `Telemetry`, `Settings`, `Analytics`) всё ещё без unit coverage.
4. **Service layer**: `api.ts`, `echo.ts`, `NodeStatusManager.ts`, `error-handler.ts`, `usePopup.ts` требуют тестов.

## 🔧 Следующие шаги

1. Покрыть realtime/графические компоненты и сервисы (`TelemetryChart.vue`, `AdvancedChart.vue`, `NodeStatusManager.ts`, `echo.ts`).
2. Добавить тесты для утилит и composable (`usePopup.ts`, `error-handler.ts`, `useDebounce.ts`, `useResponsive.ts`).
3. Подготовить smoke-тесты для основных `views/*` (Dashboard, Nodes, Telemetry, Settings, Analytics).
4. Запланировать интеграционные/E2E сценарии (Playwright/Cypress) — отдельный backlog.

## 📝 Примечание

- Покрытие собирается командой `npm run test:coverage` (локально или через Docker compose с обязательным `npm install`).
- Отчёт `coverage/index.html` отображает детальный разбор по каждому файлу.

