# Статус Frontend тестов

## ✅ Покрытые области (Vitest)

- Growth Planner: `CreateCycleDialog`, `CycleCard`, `GrowthPresetCard`, `StageTimeline`, `ZoneSelector`
- Общие компоненты: `EventLog`, `NodeCard`, `ConfigEditor`
- Stores: `growth`, `zones`, `nodes`, `telemetry`, `errors`
- Services: `api`

**Всего:** 14 файлов тестов, 140+ assert-случаев.

## 📊 Сводка покрытия (istanbul)

| Область | Stmts | Branches | Funcs | Lines |
| --- | --- | --- | --- | --- |
| Компоненты Growth | 82.84% | 77.77% | 80.39% | 83.66% |
| `EventLog.vue` | 84% | 89.28% | 72.72% | 82.6% |
| `NodeCard.vue` | 62.93% | 37.8% | 65.38% | 70.58% |
| `stores/nodes.ts` | 55% | 47.5% | 81.48% | 52.74% |
| `stores/telemetry.ts` | 65.07% | 50% | 92.85% | 63.33% |
| `stores/zones.ts` | 52.94% | 32.43% | 68.96% | 51.85% |

## ⚠️ Известные ограничения

1. **Vuetify + happy-dom**: предупреждение `Right-hand side of 'instanceof' is not an object` перехватывается в `setup.js`, но остаётся в логах.
2. **Chart.js / canvas**: компоненты `TelemetryChart`, `AdvancedChart` пока не покрыты.
3. **Views и оставшиеся stores** требуют новых сценариев.

## 🚀 Следующие шаги

1. Stores: `app`, `events`, `settings`, `nodeConfig`
2. Services/утилиты: `api.ts`, `echo.ts`, `nodes-api.ts`, `useOfflineMode.ts`, `usePopup.ts`
3. Key UI/Views: `AdvancedChart.vue`, `TelemetryChart.vue`, `NodeManagementCard.vue`, `views/*`
4. Интеграционное/E2E покрытие (Playwright/Cypress) — отдельный backlog

