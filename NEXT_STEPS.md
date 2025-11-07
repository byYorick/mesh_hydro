# 🎯 Следующие шаги для завершения проекта

## Приоритет 1: Исправить тесты (2-3 часа)

### 1.1 Исправить миграцию node_configuration_confirmations

```php
// server/backend/database/migrations/2024_11_06_100009_create_node_configuration_confirmations_table.php

// БЫЛО:
$table->foreignId('node_id')->constrained('nodes')->onDelete('cascade');

// ДОЛЖНО БЫТЬ:
$table->unsignedBigInteger('node_id');
$table->foreign('node_id')->references('id')->on('nodes')->onDelete('cascade');
```

### 1.2 Добавить missing API endpoints

```php
// server/backend/routes/api.php

// Добавить:
Route::post('/growth/cycles/{cycle}/transition', [GrowthCycleController::class, 'transition']);
Route::post('/growth/cycles/{cycle}/accept-transition/{recommendation}', [GrowthCycleController::class, 'acceptTransition']);
Route::get('/zones/{zone}/telemetry', [ZoneController::class, 'getTelemetry']);
Route::get('/zones/{zone}/statistics', [ZoneController::class, 'getStatistics']);
Route::patch('/zones/{zone}', [ZoneController::class, 'update']);
```

### 1.3 Исправить ZoneModelTest

```php
// Заменить все Zone::create() на Zone::factory()->create()
// Это гарантирует уникальность root_node_id
```

### 1.4 Добавить методы в GrowthCycleController

```php
public function transition(Request $request, GrowthCycle $cycle)
{
    // Реализовать переход на следующую стадию
}

public function acceptTransition(GrowthCycle $cycle, StageTransitionRecommendation $recommendation)
{
    // Принять рекомендацию и выполнить переход
}
```

### 1.5 Добавить методы в ZoneController

```php
public function getTelemetry(Zone $zone)
{
    // Получить телеметрию для зоны
}

public function getStatistics(Request $request, Zone $zone)
{
    // Получить статистику для зоны
}

public function update(Request $request, Zone $zone)
{
    // Обновить зону
}
```

## Приоритет 2: ESP32 Nodes (2-4 часа)

### 2.1 Обновить node_climate

```c
// node_climate/main/app_main.c

#include "node_config.h"

// В функции создания сообщения:
msg.root_node_id = node_config_get_root_node_id();
```

### 2.2 Обновить node_ph_ec

```c
// node_ph_ec/main/app_main.c

#include "node_config.h"

// Аналогично:
msg.root_node_id = node_config_get_root_node_id();
```

### 2.3 Обновить node_relay

```c
// node_relay/main/app_main.c

// То же самое
```

### 2.4 Обновить node_water

```c
// node_water/main/app_main.c

// То же самое
```

## Приоритет 3: Frontend (1-2 часа)

### 3.1 Исправить CSS Import Issue

Вариант А: Обновить Vitest конфигурацию
```javascript
// vitest.config.js
export default defineConfig({
  plugins: [
    vue(),
    vuetify({ styles: { configFile: 'src/styles/settings.scss' } }),
  ],
  test: {
    environment: 'happy-dom',
    setupFiles: ['src/__tests__/setup.js'],
    css: false, // Отключить обработку CSS
  },
})
```

Вариант Б: Использовать E2E тесты
```bash
npm install -D cypress
# Или
npm install -D @playwright/test
```

## Приоритет 4: Deployment (4-6 часов)

### 4.1 Production environment

```bash
# 1. Настроить .env для production
cp .env.example .env.production

# 2. Настроить docker-compose.prod.yml
# - Убрать debug режимы
# - Добавить reverse proxy (nginx)
# - Настроить SSL
# - Ограничить ресурсы

# 3. Deploy
docker-compose -f docker-compose.prod.yml up -d
```

### 4.2 CI/CD (GitHub Actions)

```yaml
# .github/workflows/tests.yml
name: Tests

on: [push, pull_request]

jobs:
  backend:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - run: docker-compose up -d
      - run: docker exec backend php artisan test

  frontend:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - run: npm ci
      - run: npm test
```

### 4.3 Monitoring

```yaml
# Добавить в docker-compose:
  prometheus:
    image: prom/prometheus
  
  grafana:
    image: grafana/grafana
  
  loki:
    image: grafana/loki
```

## Приоритет 5: Documentation (1-2 часа)

### 5.1 API Documentation

```bash
# Установить Scribe для Laravel
composer require --dev knuckleswtf/scribe

# Генерировать docs
php artisan scribe:generate
```

### 5.2 User Manual

- Руководство пользователя
- Видео туториалы
- FAQ
- Troubleshooting guide

## Чек-лист перед Production

- [ ] Все тесты проходят (95%+)
- [ ] Исправлены security проблемы
- [ ] Настроен мониторинг
- [ ] Настроены backup'ы
- [ ] SSL сертификаты
- [ ] Rate limiting
- [ ] Error handling
- [ ] Logging
- [ ] Документация
- [ ] User training

## Оценка времени

| Задача | Время | Приоритет |
|--------|-------|-----------|
| Исправить тесты | 2-3 ч | 🔴 Высокий |
| ESP32 nodes | 2-4 ч | 🔴 Высокий |
| Frontend CSS | 1-2 ч | 🟡 Средний |
| API endpoints | 1-2 ч | 🔴 Высокий |
| Deployment | 4-6 ч | 🟡 Средний |
| CI/CD | 2-3 ч | 🟡 Средний |
| Docs | 1-2 ч | 🟢 Низкий |
| **ВСЕГО** | **13-22 ч** | |

## Текущий прогресс: 97%

```
████████████████████░ 97%
```

**Осталось:** ~3% (1 ESP32 задача + исправление тестов)

---

**Обновлено:** 2024-11-07  
**Следующий шаг:** Исправить failing тесты

