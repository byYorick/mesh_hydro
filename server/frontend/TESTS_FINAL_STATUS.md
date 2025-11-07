# Итоговый статус Frontend тестов

## ✅ Покрытие

- 14 файлов тестов (stores, services, компоненты growth + общие)
- 140+ проверок, стабильный прогон `vitest run` / `docker compose run … npm test`
- HTML/JSON отчёты покрытия: `server/frontend/coverage/`

## 📊 Покрытие (istanbul)

| Модуль | Stmts | Branches | Funcs | Lines |
| --- | --- | --- | --- | --- |
| `src/components/growth` | 82.84% | 77.77% | 80.39% | 83.66% |
| `src/components/EventLog.vue` | 84% | 89.28% | 72.72% | 82.6% |
| `src/components/NodeCard.vue` | 62.93% | 37.8% | 65.38% | 70.58% |
| `src/stores/nodes.ts` | 55% | 47.5% | 81.48% | 52.74% |
| `src/stores/telemetry.ts` | 65.07% | 50% | 92.85% | 63.33% |
| `src/stores/zones.ts` | 52.94% | 32.43% | 68.96% | 51.85% |

## ⚠️ Ограничения

1. **Vuetify + happy-dom**: предупреждение `Right-hand side of 'instanceof' is not an object` перехватывается в `src/__tests__/setup.js`, но остаётся в логе.
2. **Chart.js**: компоненты визуализации (например, `TelemetryChart.vue`) пока без моков.
3. **Большинство views и вспомогательных stores** не покрыты.

## 🔧 Следующие шаги

1. Написать тесты для stores `app`, `events`, `settings`, `nodeConfig`.
2. Покрыть `services/api.ts`, `echo.ts`, `nodes-api.ts`, а также util-композаблы (`useOfflineMode`, `usePopup`).
3. Протестировать высокоуровневые UI (`AdvancedChart.vue`, `TelemetryChart.vue`, `NodeManagementCard.vue`, `views/*`).
4. Подготовить интеграционные/E2E сценарии (Playwright/Cypress) — отдельный backlog.

## 📝 Примечание

- Покрытие собирается командой `npm run test:coverage` (локально или через Docker compose с обязательным `npm install`).
- Отчёт `coverage/index.html` отображает детальный разбор по каждому файлу.

