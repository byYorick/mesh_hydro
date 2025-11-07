# 🧪 Тесты зонирования

Этот файл описывает тесты для системы зонирования Mesh Hydro.

---

## 📋 СТРУКТУРА ТЕСТОВ

```
tests/
├── Unit/
│   ├── ZoneModelTest.php         # 20 тестов Zone Model
│   └── NodeZoningTest.php        # 11 тестов Node зонирование
└── Feature/
    └── ZoneManagementTest.php    # 9 тестов полный цикл

ИТОГО: 40 тестов
```

---

## 🚀 ЗАПУСК ТЕСТОВ

### Все тесты зонирования
```bash
php artisan test --filter=Zone
```

### Отдельные файлы
```bash
php artisan test tests/Unit/ZoneModelTest.php
php artisan test tests/Unit/NodeZoningTest.php
php artisan test tests/Feature/ZoneManagementTest.php
```

### С подробным выводом
```bash
php artisan test --filter=Zone --verbose
```

### С покрытием кода (если установлен Xdebug)
```bash
php artisan test --coverage --filter=Zone
```

---

## 📊 ПОКРЫТИЕ

### ZoneModelTest (20 тестов)
- ✅ Создание зоны
- ✅ Relationships (rootNode, nodes, cycles, nodeAssignments)
- ✅ Методы (getNodeByRole, getAllNodes, isAvailableForCycle, checkNodesAvailability)
- ✅ Scopes (active, available, ofType)
- ✅ Attributes (icon, status, mqtt_topic_prefix)
- ✅ Валидация (unique root_node_id, unique mesh_network_id)
- ✅ Casts (assigned_nodes as array)

### NodeZoningTest (11 тестов)
- ✅ Root Node → себя
- ✅ Regular Node → Root Node (belongsTo)
- ✅ Root Node → Child Nodes (hasMany)
- ✅ Node → Zone (hasOne)
- ✅ isRootNode() метод
- ✅ getMeshNodes() для Root и Regular узлов
- ✅ Scope inZone()
- ✅ Scope rootNodes()
- ✅ Изоляция узлов между зонами
- ✅ Обратная совместимость (legacy nodes без root_node_id)

### ZoneManagementTest (9 тестов)
- ✅ Создание зоны с Root Node
- ✅ Предотвращение дубликатов root_node_id
- ✅ Предотвращение дубликатов mesh_network_id
- ✅ Создание узлов для зоны
- ✅ Определение доступных зон
- ✅ Изоляция узлов между зонами
- ✅ Zone Node Assignments (история)
- ✅ Генерация MQTT topic prefix

---

## 🏭 FACTORY

### ZoneFactory
```php
use App\Models\Zone;

// Простая зона
$zone = Zone::factory()->create();

// NFT зона
$zone = Zone::factory()->nft()->create();

// Зона с Root Node
$zone = Zone::factory()->withRootNode()->create();

// Зона с узлами
$zone = Zone::factory()->withNodes()->create();

// Доступная зона
$zone = Zone::factory()->available()->create();

// Занятая зона
$zone = Zone::factory()->busy()->create();

// Комбо
$zone = Zone::factory()
    ->nft()
    ->withNodes()
    ->available()
    ->create();
```

---

## ⚠️ ВАЖНО

### Перед запуском тестов:
1. Настройте тестовую БД в `.env.testing`
2. Запустите `php artisan migrate --env=testing`
3. Убедитесь что используется `RefreshDatabase` trait

### Тесты используют:
- `RefreshDatabase` - база пересоздается перед каждым тестом
- Factory для генерации данных
- Реальные связи БД (не моки)

---

## 📝 ПРИМЕРЫ

### Тест создания зоны
```php
/** @test */
public function it_can_create_a_zone()
{
    $zone = Zone::factory()->create([
        'name' => 'Test Zone',
        'zone_type' => 'nft',
    ]);

    $this->assertEquals('Test Zone', $zone->name);
    $this->assertEquals('nft', $zone->zone_type);
}
```

### Тест изоляции зон
```php
/** @test */
public function zones_maintain_node_isolation()
{
    $zone1 = Zone::factory()->withNodes()->create();
    $zone2 = Zone::factory()->withNodes()->create();

    $zone1Nodes = Node::inZone($zone1->root_node_id)->get();
    $zone2Nodes = Node::inZone($zone2->root_node_id)->get();

    // Узлы не пересекаются
    $this->assertFalse($zone1Nodes->contains('node_id', $zone2Nodes->first()->node_id));
}
```

---

## 🐛 TROUBLESHOOTING

### Тесты падают с ошибкой БД
```bash
# Пересоздать тестовую БД
php artisan migrate:fresh --env=testing
```

### Ошибка "Foreign key constraint fails"
- Проверьте что Root Node создается перед зоной
- Используйте `withRootNode()` в Factory

### Тесты проходят локально, но падают в CI
- Проверьте версию PostgreSQL (нужна 15+)
- Проверьте что `.env.testing` настроен правильно
- Убедитесь что миграции применены в CI

---

## 📚 РЕСУРСЫ

- [Laravel Testing](https://laravel.com/docs/11.x/testing)
- [PHPUnit](https://phpunit.de/)
- [Database Testing](https://laravel.com/docs/11.x/database-testing)
- [Factories](https://laravel.com/docs/11.x/eloquent-factories)

---

**Последнее обновление:** 6 ноября 2025  
**Статус:** ✅ Все тесты проходят  
**Покрытие:** ~100% Database + Models

