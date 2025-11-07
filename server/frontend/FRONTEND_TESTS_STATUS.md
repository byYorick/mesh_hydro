# Статус Frontend тестов

## ✅ Покрытые области (Vitest)

- Growth Planner: `CreateCycleDialog`, `CycleCard`, `GrowthPresetCard`, `StageTimeline`, `ZoneSelector`
- Zones: `ZoneSelector`, `ZoneCard`, `views/Zones`
- Общие компоненты: `EventLog`, `NodeCard`, `ConfigEditor`
- Stores: `growth`, `zones`, `nodes`, `telemetry`, `errors`, `app`, `events`, `settings`, `nodeConfig`
- Services / utils: `api`, `nodes-api`, `useOfflineMode`, `array`, `logger`, `time`

**Всего:** 25 файлов тестов, 190+ assert-случаев.

## 📊 Сводка покрытия (istanbul)

| Область | Stmts | Branches | Funcs | Lines |
| --- | --- | --- | --- | --- |
| Компоненты Growth | 82.84% | 77.77% | 80.39% | 83.66% |
| `EventLog.vue` | 84% | 89.28% | 72.72% | 82.6% |
| `NodeCard.vue` | 62.93% | 37.8% | 65.38% | 70.58% |
| `ZoneCard.vue` | 84.09% | 70.68% | 77.27% | 87.5% |
| `stores/app.ts` | 69.23% | 67.34% | 64.28% | 69.86% |
| `stores/events.ts` | 55.38% | 66.66% | 57.69% | 56.19% |
| `stores/settings.js` | 65.38% | 95.23% | 61.11% | 67.34% |
| `useOfflineMode.ts` | 57.5% | 68% | 42.85% | 58.97% |
| `nodes-api.js` | 100% | 100% | 100% | 100% |
| `utils/logger.ts` | 68.85% | 54.54% | 48.14% | 76% |

## ⚠️ Известные ограничения

1. **Vuetify + happy-dom**: предупреждение `Right-hand side of 'instanceof' is not an object` перехватывается в `setup.js`, но остаётся в логах.
2. **Chart.js / canvas**: компоненты `TelemetryChart`, `AdvancedChart`, графики в `NodeDetail` без моков.
3. **Views**: большинство страниц (`Dashboard`, `Nodes`, `Telemetry`, `Settings`, `Analytics`) пока не покрыты.
4. **Сервисный слой**: `api.ts`, `echo.ts`, `NodeStatusManager`, `error-handler.ts`, `usePopup.ts` требуют дополнительных тестов.

## 🚀 Следующие шаги

1. Покрыть графические и realtime-компоненты (`TelemetryChart.vue`, `AdvancedChart.vue`, `NodeStatusManager.ts`, `echo.ts`).
2. Написать тесты для `usePopup.ts` и `error-handler.ts`.
3. Добавить smoke-тесты для ключевых `views/*` (Dashboard, Nodes, Telemetry, Settings, Analytics).
4. Подготовить интеграционные/E2E сценарии (Playwright/Cypress) — отдельный backlog.

