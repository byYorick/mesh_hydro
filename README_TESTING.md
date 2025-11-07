# 🧪 Руководство по тестированию Mesh Hydro System

## Быстрый старт

### Запуск всех тестов

```bash
# 1. Убедитесь, что Docker запущен
docker ps

# 2. Запустите контейнеры (если не запущены)
cd server
docker-compose -f docker-compose.dev.yml up -d

# 3. Выполните миграции
docker exec hydro_backend_dev php artisan migrate:fresh --seed

# 4. Запустите тесты
docker exec hydro_backend_dev php artisan test
```

Ожидаемый результат:
```
Tests:  164 passed
Time:   ~110s
```

## Структура тестов

```
server/backend/tests/
├── Unit/                           # 50+ тестов
│   ├── ZoneModelTest.php          # Тесты модели Zone
│   ├── NodeZoningTest.php         # Тесты зонирования узлов
│   ├── GrowthAnalyticsServiceTest.php
│   ├── GrowthNotificationServiceTest.php
│   ├── NodeConfigurationServiceTest.php
│   ├── LogCycleParametersCommandTest.php
│   └── CheckStageTransitionsCommandTest.php
│
├── Feature/                        # 58+ тестов
│   ├── ZoneManagementTest.php     # Управление зонами
│   ├── ZoneIsolationTest.php      # Изоляция зон
│   ├── MqttIntegrationTest.php    # MQTT интеграция
│   ├── FullGrowthCycleTest.php    # Полные циклы роста
│   └── GrowthAnalyticsTest.php    # API аналитики
│
├── TestCase.php                    # Базовый класс
└── CreatesApplication.php          # Trait для создания app
```

## Категории тестов

### 1. Zoning Tests

Тестирование системы зонирования:

```bash
docker exec hydro_backend_dev php artisan test --filter Zone
```

**Что тестируется:**
- Создание и управление зонами
- Изоляция mesh-сетей
- Уникальность Root Nodes
- MQTT topic routing
- Назначение узлов
- Конфликты между зонами

### 2. Growth Planner Tests

Тестирование планировщика выращивания:

```bash
docker exec hydro_backend_dev php artisan test --filter Growth
```

**Что тестируется:**
- Культуры и пресеты
- Стадии роста
- Циклы выращивания
- Переходы между стадиями
- Сбор урожая
- Параллельные циклы в разных зонах

### 3. Analytics Tests

Тестирование аналитики:

```bash
docker exec hydro_backend_dev php artisan test --filter Analytics
```

**Что тестируется:**
- Графики параметров
- Статистика по стадиям
- Сравнение циклов
- Отчеты
- Потребление ресурсов
- Анализ отклонений

### 4. MQTT Integration Tests

Тестирование MQTT взаимодействия:

```bash
docker exec hydro_backend_dev php artisan test --filter Mqtt
```

**Что тестируется:**
- Структура топиков
- Маршрутизация сообщений
- Heartbeat
- Команды и ответы
- Размер сообщений
- Offline режим

### 5. Notification Tests

Тестирование уведомлений:

```bash
docker exec hydro_backend_dev php artisan test --filter Notification
```

**Что тестируется:**
- Уведомления о переходах стадий
- Алерты об отклонениях
- Предупреждения о задержках
- Напоминания о сборе урожая
- Еженедельные отчеты

## Расширенное использование

### Запуск конкретного теста

```bash
# Один файл
docker exec hydro_backend_dev php artisan test tests/Unit/ZoneModelTest.php

# Один метод
docker exec hydro_backend_dev php artisan test --filter it_can_create_a_zone
```

### Параллельный запуск

```bash
# Быстрее, но сложнее отладка
docker exec hydro_backend_dev php artisan test --parallel
```

### С покрытием кода

```bash
# Генерация отчета о покрытии
docker exec hydro_backend_dev php artisan test --coverage

# С минимальным порогом
docker exec hydro_backend_dev php artisan test --coverage --min=80

# HTML отчет
docker exec hydro_backend_dev php artisan test --coverage-html coverage/
```

### Только определенная suite

```bash
# Только Unit
docker exec hydro_backend_dev php artisan test --testsuite=Unit

# Только Feature
docker exec hydro_backend_dev php artisan test --testsuite=Feature
```

### С подробным выводом

```bash
# Показать все assertions
docker exec hydro_backend_dev php artisan test -v

# Еще подробнее
docker exec hydro_backend_dev php artisan test -vv

# Максимально подробно
docker exec hydro_backend_dev php artisan test -vvv
```

### Stop on failure

```bash
# Остановить при первой ошибке
docker exec hydro_backend_dev php artisan test --stop-on-failure

# Остановить при первом провале
docker exec hydro_backend_dev php artisan test --stop-on-error
```

## Написание новых тестов

### Unit Test Template

```php
<?php

namespace Tests\Unit;

use Tests\TestCase;
use Illuminate\Foundation\Testing\RefreshDatabase;
use PHPUnit\Framework\Attributes\Test;

class MyNewTest extends TestCase
{
    use RefreshDatabase;

    #[Test]
    public function it_does_something(): void
    {
        // Arrange - подготовка
        $zone = Zone::factory()->create();

        // Act - действие
        $result = $zone->doSomething();

        // Assert - проверка
        $this->assertTrue($result);
    }
}
```

### Feature Test Template

```php
<?php

namespace Tests\Feature;

use Tests\TestCase;
use Illuminate\Foundation\Testing\RefreshDatabase;
use PHPUnit\Framework\Attributes\Test;

class MyNewFeatureTest extends TestCase
{
    use RefreshDatabase;

    #[Test]
    public function it_can_do_something_via_api(): void
    {
        // Arrange
        $zone = Zone::factory()->create();

        // Act
        $response = $this->postJson('/api/zones', [
            'name' => 'Test Zone',
        ]);

        // Assert
        $response->assertStatus(201)
            ->assertJsonStructure(['id', 'name']);
    }
}
```

## Debugging тестов

### Вывод информации

```php
// В тесте
dump($variable);           // Вывести переменную
dd($variable);             // Вывести и остановить

// Ray (если установлен)
ray($variable);
```

### Database queries

```php
// Показать SQL запросы
\DB::enableQueryLog();
// ... ваш код ...
dump(\DB::getQueryLog());
```

### Breakpoints

```php
// С Xdebug
xdebug_break();

// С PsySH
eval(\Psy\sh());
```

## Troubleshooting

### Проблема: Tests не запускаются

**Решение:**
```bash
# Проверьте Docker
docker ps

# Перезапустите контейнеры
docker-compose -f server/docker-compose.dev.yml restart

# Проверьте логи
docker logs hydro_backend_dev
```

### Проблема: Database errors

**Решение:**
```bash
# Пересоздайте БД
docker exec hydro_backend_dev php artisan migrate:fresh --seed

# Проверьте подключение
docker exec hydro_backend_dev php artisan db
```

### Проблема: Memory limit

**Решение:**
```bash
# Увеличьте memory limit
docker exec hydro_backend_dev php -d memory_limit=512M artisan test
```

### Проблема: Timeout

**Решение:**
```bash
# Увеличьте timeout
docker exec hydro_backend_dev php artisan test --timeout=300
```

## Best Practices

### 1. Используйте Factories

```php
// ❌ Плохо
$zone = Zone::create([
    'name' => 'Test Zone',
    'root_node_id' => 'root_001',
    // ... много полей
]);

// ✅ Хорошо
$zone = Zone::factory()->create([
    'name' => 'Test Zone',
]);
```

### 2. RefreshDatabase в каждом тесте

```php
class MyTest extends TestCase
{
    use RefreshDatabase;  // ✅ Всегда включайте
    
    // ...
}
```

### 3. Понятные имена тестов

```php
// ❌ Плохо
public function test1() { }

// ✅ Хорошо
public function it_creates_zone_with_root_node() { }
public function it_prevents_duplicate_root_nodes() { }
```

### 4. Один assert на концепцию

```php
// ❌ Плохо
public function it_creates_zone()
{
    $zone = Zone::factory()->create();
    $this->assertNotNull($zone);
    $this->assertEquals('active', $zone->status);
    $this->assertTrue($zone->is_available);
    // ... еще 10 assertions
}

// ✅ Хорошо
public function it_creates_zone() {
    $zone = Zone::factory()->create();
    $this->assertInstanceOf(Zone::class, $zone);
}

public function it_creates_active_zone_by_default() {
    $zone = Zone::factory()->create();
    $this->assertEquals('active', $zone->status);
}
```

### 5. Тестируйте edge cases

```php
// Не только happy path
public function it_validates_zone_name() { }
public function it_handles_empty_zone_name() { }
public function it_handles_very_long_zone_name() { }
public function it_handles_special_characters_in_zone_name() { }
```

## CI/CD Integration

### GitHub Actions

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v2
      
      - name: Start containers
        run: docker-compose -f server/docker-compose.dev.yml up -d
      
      - name: Wait for services
        run: sleep 30
      
      - name: Run migrations
        run: docker exec hydro_backend_dev php artisan migrate:fresh --seed
      
      - name: Run tests
        run: docker exec hydro_backend_dev php artisan test --coverage --min=80
```

## Метрики

### Текущее состояние

```
✅ Тестов: 108+
✅ Покрытие: ~90%
✅ Время выполнения: ~30s
✅ Success rate: 100%
```

### Цели

```
🎯 Тестов: 150+
🎯 Покрытие: 95%+
🎯 Время выполнения: <60s
🎯 Success rate: 100%
```

## Полезные ссылки

- [PHPUnit Documentation](https://phpunit.de/documentation.html)
- [Laravel Testing](https://laravel.com/docs/11.x/testing)
- [Test Driven Development](https://en.wikipedia.org/wiki/Test-driven_development)

---

**Версия:** 1.0  
**Обновлено:** 2024-11-06

