# Frontend Tests

## 📋 Обзор

Созданы unit тесты для компонентов и stores Growth Planner и Zoning.

## ✅ Созданные тесты

### Stores (2 файла)
- ✅ `src/__tests__/stores/zones.test.ts` - Тесты для zones store
- ✅ `src/__tests__/stores/growth.test.ts` - Тесты для growth store

### Components (5 файлов)
- ✅ `src/__tests__/components/growth/GrowthPresetCard.test.ts` - Тесты для карточки пресета
- ✅ `src/__tests__/components/growth/CycleCard.test.ts` - Тесты для карточки цикла
- ✅ `src/__tests__/components/growth/StageTimeline.test.ts` - Тесты для временной шкалы стадий
- ✅ `src/__tests__/components/growth/ZoneSelector.test.ts` - Тесты для селектора зон
- ✅ `src/__tests__/components/growth/CreateCycleDialog.test.ts` - Тесты для диалога создания цикла

## 🚀 Запуск тестов

```bash
# Все тесты
npm test

# В watch режиме
npm test -- --watch

# С покрытием
npm run test:coverage

# Конкретный файл
npm test -- src/__tests__/stores/zones.test.ts
```

## ⚠️ Известные проблемы

1. **CSS файлы Vuetify**: Требуется дополнительная настройка для обработки CSS импортов в тестах
2. **Vuetify компоненты**: Некоторые компоненты могут требовать дополнительных моков

## 📝 TODO

- [ ] Тесты для страниц (GrowthPlanner, PresetLibrary)
- [ ] Интеграционные тесты
- [ ] E2E тесты (Playwright/Cypress)

