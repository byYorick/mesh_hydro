# 🌱 ПЛАН РЕАЛИЗАЦИИ: Growth Planner System

**Дата:** 6 ноября 2025  
**Версия:** 1.0  
**Приоритет:** Высокий  

---

## 📋 ОГЛАВЛЕНИЕ

1. [Обзор системы](#обзор-системы)
2. [Архитектура](#архитектура)
3. [Database Schema](#database-schema)
4. [Backend Implementation](#backend-implementation)
5. [Frontend Implementation](#frontend-implementation)
6. [MQTT Integration](#mqtt-integration)
7. [Система подтверждения](#система-подтверждения)
8. [Уведомления](#уведомления)
9. [Аналитика](#аналитика)
10. [План работ](#план-работ)

---

## 🎯 ОБЗОР СИСТЕМЫ

### Основные требования

✅ **4 базовых пресета:** Салат, Клубника, Томаты, Огурцы  
✅ **Создание пользовательских пресетов** (клонирование + с нуля)  
✅ **⭐ ЗОНИРОВАНИЕ:** Каждый Root Node = отдельная зона со своей mesh сетью  
✅ **Множественные циклы** (разные зоны, разные культуры, параллельно)  
✅ **Максимальный набор параметров** + кастомные поля  
✅ **Полуавтоматический режим** (с подтверждением перехода)  
✅ **Timeline визуализация** (красивая шкала прогресса)  
✅ **Система подтверждения** от узлов (важно!)  
✅ **Расширенная аналитика** (графики, сравнения, история)  
✅ **Уведомления:** Web Push, In-app, Telegram, SMS

### ⭐ Зонирование (критически важно!)

**Каждая зона = отдельный Root Node + своя mesh сеть:**
- Root Node #1 (root_001) → Mesh HYDRO1_ZONE1 → Зона 1 NFT
- Root Node #2 (root_002) → Mesh HYDRO1_ZONE2 → Зона 2 DWC
- Root Node #3 (root_003) → Mesh HYDRO1_ZONE3 → Зона 3 Капельный полив

**Полная изоляция зон:**
- Отдельные MQTT топики (`hydro/zone1/`, `hydro/zone2/`, ...)
- Узлы привязаны к конкретному Root Node
- Циклы роста работают независимо в каждой зоне
- Конфликты узлов между зонами невозможны

📖 **Подробности:** см. `ZONING_ARCHITECTURE_PLAN.md`  

---

## 🏗️ АРХИТЕКТУРА

```
┌─────────────────────────────────────────────────────────┐
│                   FRONTEND (Vue.js)                     │
│                                                          │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐       │
│  │  Growth    │  │  Preset    │  │   Cycle    │       │
│  │  Planner   │  │  Library   │  │  Details   │       │
│  └────────────┘  └────────────┘  └────────────┘       │
│                                                          │
│  ┌─────────────────────────────────────────────┐       │
│  │         Growth Store (Pinia)                │       │
│  │  • Cycles • Presets • Analytics             │       │
│  └─────────────────────────────────────────────┘       │
└──────────────────────┬──────────────────────────────────┘
                       │ REST API + WebSocket
┌──────────────────────▼──────────────────────────────────┐
│                  BACKEND (Laravel)                       │
│                                                          │
│  ┌──────────────────────────────────────────┐          │
│  │         Growth Controllers               │          │
│  │  • PresetController                      │          │
│  │  • CycleController                       │          │
│  │  • StageTransitionController             │          │
│  │  • AnalyticsController                   │          │
│  └──────────────────────────────────────────┘          │
│                                                          │
│  ┌──────────────────────────────────────────┐          │
│  │         Services                         │          │
│  │  • GrowthCycleService                    │          │
│  │  • StageTransitionService                │          │
│  │  • ConfigurationApplierService ⭐        │          │
│  │  • ConfirmationTrackerService ⭐         │          │
│  │  • NotificationService                   │          │
│  └──────────────────────────────────────────┘          │
│                                                          │
│  ┌──────────────────────────────────────────┐          │
│  │         Scheduler (Cron)                 │          │
│  │  • CheckStageTransitions (hourly)        │          │
│  │  • CheckTargetCompliance (every 5min)    │          │
│  │  • SendReminders (daily)                 │          │
│  └──────────────────────────────────────────┘          │
└──────────────────────┬──────────────────────────────────┘
                       │ MQTT
┌──────────────────────▼──────────────────────────────────┐
│              MQTT BROKER (Mosquitto)                     │
│                                                          │
│  Topics:                                                 │
│  • hydro/command/{node_id}  → команды узлам             │
│  • hydro/response/{node_id} → подтверждения ⭐          │
│  • hydro/telemetry/{node_id} → текущие значения         │
└──────────────────────┬──────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────┐
│                   ESP32 NODES                            │
│                                                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │ pH Node  │  │ Climate  │  │  Relay   │             │
│  │          │  │  Node    │  │  Node    │             │
│  └──────────┘  └──────────┘  └──────────┘             │
└──────────────────────────────────────────────────────────┘
```

---

## 💾 DATABASE SCHEMA

### Новые таблицы

#### 1. `zones` - ⭐ Зоны выращивания (отдельные баки/системы)
```sql
CREATE TABLE zones (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,              -- "Зона 1", "Бак А", "Теплица 1"
    description TEXT,
    
    -- Физическое расположение
    location VARCHAR(100),                   -- "Комната 1", "Теплица секция A"
    
    -- Тип зоны
    zone_type VARCHAR(50),                   -- "nft", "dwc", "ebb_flow", "drip"
    
    -- Объем системы
    reservoir_volume_liters DECIMAL(10,2),   -- Объем бака
    growing_area_m2 DECIMAL(10,2),           -- Площадь выращивания
    plant_capacity INTEGER,                  -- Сколько растений помещается
    
    -- Узлы, закрепленные за зоной
    assigned_nodes JSONB NOT NULL DEFAULT '{}',
    /*
    Структура assigned_nodes:
    {
      "ph_node": "ph_001",              // pH/EC узел для этой зоны
      "climate_node": "climate_001",    // Климат узел
      "relay_node": "relay_001",        // Реле (свет, вентиляция)
      "water_node": "water_001",        // Водяной узел
      "display_node": "display_001"     // Дисплей (опционально)
    }
    */
    
    -- Статус
    is_active BOOLEAN DEFAULT true,
    is_available BOOLEAN DEFAULT true,       -- Доступна для новых циклов
    
    -- Текущее состояние
    current_cycle_id BIGINT,                 -- Активный цикл в зоне (NULL если свободна)
    
    -- Метаданные
    image_url VARCHAR(255),                  -- Фото зоны/бака
    notes TEXT,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

CREATE INDEX idx_zones_active ON zones(is_active);
CREATE INDEX idx_zones_available ON zones(is_available);
CREATE INDEX idx_zones_current_cycle ON zones(current_cycle_id);

-- Триггер для проверки доступности узлов
-- (один узел не может быть в двух активных зонах одновременно)
```

**Примеры зон:**
```sql
-- Зона 1: NFT система, 100л бак, 2м², до 20 растений
INSERT INTO zones (name, zone_type, reservoir_volume_liters, growing_area_m2, plant_capacity, assigned_nodes) VALUES
('Зона 1 - NFT', 'nft', 100.0, 2.0, 20, '{
  "ph_node": "ph_001",
  "climate_node": "climate_001", 
  "relay_node": "relay_001",
  "water_node": "water_001"
}');

-- Зона 2: DWC система, 50л бак, 1м², до 10 растений
INSERT INTO zones (name, zone_type, reservoir_volume_liters, growing_area_m2, plant_capacity, assigned_nodes) VALUES
('Зона 2 - DWC', 'dwc', 50.0, 1.0, 10, '{
  "ph_node": "ph_002",
  "climate_node": "climate_002",
  "relay_node": "relay_002"
}');

-- Зона 3: Капельный полив, 200л бак, 5м², до 50 растений  
INSERT INTO zones (name, zone_type, reservoir_volume_liters, growing_area_m2, plant_capacity, assigned_nodes) VALUES
('Теплица А - Капельный полив', 'drip', 200.0, 5.0, 50, '{
  "ph_node": "ph_003",
  "climate_node": "climate_003",
  "relay_node": "relay_003",
  "water_node": "water_003"
}');
```

#### 2. `growth_stages` - Стадии роста (справочник)
```sql
CREATE TABLE growth_stages (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,              -- Germination, Vegetative, etc.
    name_ru VARCHAR(100),                    -- Прорастание, Вегетация
    description TEXT,
    icon VARCHAR(50),                        -- emoji или иконка
    order_index INTEGER NOT NULL DEFAULT 0,  -- Порядок стадий
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

-- Базовые стадии
INSERT INTO growth_stages (name, name_ru, icon, order_index) VALUES
('Germination', 'Прорастание', '🌱', 1),
('Vegetative', 'Вегетация', '🌿', 2),
('Flowering', 'Цветение', '🌸', 3),
('Fruiting', 'Плодоношение', '🍎', 4);
```

#### 2. `growth_presets` - Пресеты культур
```sql
CREATE TABLE growth_presets (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,              -- Tomatoes, Lettuce, etc.
    name_ru VARCHAR(100),                    -- Томаты, Салат
    description TEXT,
    icon VARCHAR(50),                        -- emoji культуры
    image_url VARCHAR(255),                  -- фото культуры (optional)
    
    -- Метаданные
    is_system BOOLEAN DEFAULT false,         -- Системный (не удаляемый)
    is_public BOOLEAN DEFAULT false,         -- Публичный (community)
    user_id BIGINT,                          -- NULL для системных
    
    -- Статистика
    total_duration_days INTEGER,             -- Общая длительность
    difficulty_level VARCHAR(20),            -- easy, medium, hard
    popularity_score INTEGER DEFAULT 0,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX idx_growth_presets_system ON growth_presets(is_system);
CREATE INDEX idx_growth_presets_user ON growth_presets(user_id);
```

#### 3. `preset_stages` - Настройки для каждой стадии пресета
```sql
CREATE TABLE preset_stages (
    id BIGSERIAL PRIMARY KEY,
    preset_id BIGINT NOT NULL,
    stage_id BIGINT NOT NULL,
    
    -- Длительность
    duration_days INTEGER NOT NULL,          -- Рекомендуемая длительность
    duration_days_min INTEGER,               -- Минимум
    duration_days_max INTEGER,               -- Максимум
    
    -- Целевые параметры (JSONB для гибкости)
    target_params JSONB NOT NULL DEFAULT '{}',
    /*
    Структура target_params:
    {
      "ph": {"min": 5.8, "max": 6.2, "target": 6.0},
      "ec": {"min": 1.8, "max": 2.2, "target": 2.0},
      "temp": {"min": 22, "max": 26, "target": 24},
      "humidity": {"min": 60, "max": 70, "target": 65},
      "co2": {"target": 800},
      "light": {
        "hours_on": 16,
        "hours_off": 8,
        "intensity": 100,  // %
        "schedule": "06:00-22:00"
      },
      "water": {
        "frequency_per_day": 3,
        "volume_liters": 5,
        "level_min": 20,
        "level_max": 80
      },
      "custom": {
        // Пользовательские параметры
        "myParam": {"value": 123}
      }
    }
    */
    
    -- Описание стадии
    description TEXT,                        -- Что происходит на этой стадии
    tips TEXT,                               -- Советы пользователю
    
    -- Порядок
    order_index INTEGER NOT NULL DEFAULT 0,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (preset_id) REFERENCES growth_presets(id) ON DELETE CASCADE,
    FOREIGN KEY (stage_id) REFERENCES growth_stages(id) ON DELETE RESTRICT
);

CREATE INDEX idx_preset_stages_preset ON preset_stages(preset_id);
CREATE INDEX idx_preset_stages_stage ON preset_stages(stage_id);
```

#### 5. `growth_cycles` - Активные циклы выращивания
```sql
CREATE TABLE growth_cycles (
    id BIGSERIAL PRIMARY KEY,
    preset_id BIGINT NOT NULL,
    zone_id BIGINT NOT NULL,                 -- ⭐ Привязка к зоне!
    
    -- Основная информация
    name VARCHAR(150) NOT NULL,              -- "Томаты зимняя партия"
    description TEXT,
    
    -- Текущее состояние
    current_stage_id BIGINT,                 -- Текущая стадия
    current_preset_stage_id BIGINT,          -- Ссылка на preset_stages
    
    -- Даты
    started_at TIMESTAMP NOT NULL,
    current_stage_started_at TIMESTAMP,
    estimated_completion_at TIMESTAMP,       -- Расчетная дата завершения
    completed_at TIMESTAMP,                  -- Фактическая дата завершения
    
    -- Статус
    status VARCHAR(20) NOT NULL DEFAULT 'active',  -- active, paused, completed, cancelled
    
    -- Растения
    plant_count INTEGER,                     -- Количество растений в цикле
    
    -- Узлы берутся из зоны, но можно переопределить
    node_overrides JSONB DEFAULT NULL,
    /*
    Если NULL - используются узлы из zones.assigned_nodes
    Если задано - переопределяет узлы зоны для этого цикла:
    {
      "ph_node": "ph_special",  // Использовать другой pH узел
      "climate_node": null      // Не использовать климат узел вообще
    }
    */
    
    -- Метрики
    total_days INTEGER DEFAULT 0,
    delays_count INTEGER DEFAULT 0,          -- Количество задержек переходов
    manual_interventions INTEGER DEFAULT 0,  -- Ручных вмешательств
    
    -- Пользователь
    user_id BIGINT,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (preset_id) REFERENCES growth_presets(id) ON DELETE RESTRICT,
    FOREIGN KEY (zone_id) REFERENCES zones(id) ON DELETE RESTRICT,
    FOREIGN KEY (current_stage_id) REFERENCES growth_stages(id),
    FOREIGN KEY (current_preset_stage_id) REFERENCES preset_stages(id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL,
    
    -- Проверка: в одной зоне только один активный цикл
    CONSTRAINT unique_active_cycle_per_zone UNIQUE (zone_id) 
        WHERE (status = 'active')
);

CREATE INDEX idx_growth_cycles_status ON growth_cycles(status);
CREATE INDEX idx_growth_cycles_preset ON growth_cycles(preset_id);
CREATE INDEX idx_growth_cycles_zone ON growth_cycles(zone_id);
CREATE INDEX idx_growth_cycles_user ON growth_cycles(user_id);
```

#### 6. `cycle_stage_history` - История переходов стадий
```sql
CREATE TABLE cycle_stage_history (
    id BIGSERIAL PRIMARY KEY,
    cycle_id BIGINT NOT NULL,
    stage_id BIGINT NOT NULL,
    preset_stage_id BIGINT NOT NULL,
    
    -- Даты
    started_at TIMESTAMP NOT NULL,
    ended_at TIMESTAMP,
    
    -- Длительность
    planned_duration_days INTEGER,
    actual_duration_days INTEGER,
    
    -- Как был выполнен переход
    transition_type VARCHAR(20),             -- automatic, manual, forced
    triggered_by VARCHAR(20),                -- system, user, schedule
    
    -- Примечания
    notes TEXT,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (cycle_id) REFERENCES growth_cycles(id) ON DELETE CASCADE,
    FOREIGN KEY (stage_id) REFERENCES growth_stages(id),
    FOREIGN KEY (preset_stage_id) REFERENCES preset_stages(id)
);

CREATE INDEX idx_cycle_stage_history_cycle ON cycle_stage_history(cycle_id);
```

#### 7. `zone_node_assignments` - История назначения узлов зонам
```sql
CREATE TABLE zone_node_assignments (
    id BIGSERIAL PRIMARY KEY,
    zone_id BIGINT NOT NULL,
    node_id VARCHAR(50) NOT NULL,
    node_role VARCHAR(50) NOT NULL,          -- ph_node, climate_node, relay_node, etc.
    
    -- Период
    assigned_at TIMESTAMP NOT NULL,
    unassigned_at TIMESTAMP,
    
    -- Связано с циклом?
    cycle_id BIGINT,                         -- NULL если общее назначение зоны
    
    -- Статус
    is_active BOOLEAN DEFAULT true,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (zone_id) REFERENCES zones(id) ON DELETE CASCADE,
    FOREIGN KEY (node_id) REFERENCES nodes(node_id) ON DELETE CASCADE,
    FOREIGN KEY (cycle_id) REFERENCES growth_cycles(id) ON DELETE SET NULL
);

CREATE INDEX idx_zone_nodes_zone ON zone_node_assignments(zone_id);
CREATE INDEX idx_zone_nodes_node ON zone_node_assignments(node_id);
CREATE INDEX idx_zone_nodes_active ON zone_node_assignments(is_active);
```

#### 8. `stage_transition_proposals` - Предложения переходов (для полуавтомата)
```sql
CREATE TABLE stage_transition_proposals (
    id BIGSERIAL PRIMARY KEY,
    cycle_id BIGINT NOT NULL,
    
    -- Переход
    from_stage_id BIGINT NOT NULL,
    to_stage_id BIGINT NOT NULL,
    to_preset_stage_id BIGINT NOT NULL,
    
    -- Статус предложения
    status VARCHAR(20) NOT NULL DEFAULT 'pending',  -- pending, approved, rejected, expired
    
    -- Новые параметры
    new_config JSONB NOT NULL,               -- Что будет применено
    affected_nodes JSONB NOT NULL,           -- Какие узлы будут изменены
    
    -- Причины
    reason TEXT,                             -- Почему предлагается переход
    
    -- Даты
    proposed_at TIMESTAMP NOT NULL,
    expires_at TIMESTAMP,                    -- Предложение истекает
    responded_at TIMESTAMP,
    responded_by_user_id BIGINT,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (cycle_id) REFERENCES growth_cycles(id) ON DELETE CASCADE,
    FOREIGN KEY (from_stage_id) REFERENCES growth_stages(id),
    FOREIGN KEY (to_stage_id) REFERENCES growth_stages(id),
    FOREIGN KEY (to_preset_stage_id) REFERENCES preset_stages(id)
);

CREATE INDEX idx_stage_proposals_cycle ON stage_transition_proposals(cycle_id);
CREATE INDEX idx_stage_proposals_status ON stage_transition_proposals(status);
```

#### 9. `node_configuration_confirmations` - ⭐ Подтверждения от узлов
```sql
CREATE TABLE node_configuration_confirmations (
    id BIGSERIAL PRIMARY KEY,
    
    -- Какая конфигурация
    cycle_id BIGINT,
    preset_stage_id BIGINT,
    node_id VARCHAR(50) NOT NULL,
    
    -- Что было отправлено
    config_sent JSONB NOT NULL,
    command_id VARCHAR(100) UNIQUE,          -- UUID команды
    
    -- Статус
    status VARCHAR(20) NOT NULL DEFAULT 'pending',
    -- pending, confirmed, failed, timeout, partial
    
    -- Ответ от узла
    config_received JSONB,                   -- Что узел подтвердил
    response_data JSONB,                     -- Полный ответ узла
    
    -- Даты
    sent_at TIMESTAMP NOT NULL,
    confirmed_at TIMESTAMP,
    timeout_at TIMESTAMP,                    -- Когда истекает
    
    -- Ошибки
    error_message TEXT,
    retry_count INTEGER DEFAULT 0,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (cycle_id) REFERENCES growth_cycles(id) ON DELETE CASCADE,
    FOREIGN KEY (preset_stage_id) REFERENCES preset_stages(id)
);

CREATE INDEX idx_node_confirmations_node ON node_configuration_confirmations(node_id);
CREATE INDEX idx_node_confirmations_status ON node_configuration_confirmations(status);
CREATE INDEX idx_node_confirmations_cycle ON node_configuration_confirmations(cycle_id);
CREATE INDEX idx_node_confirmations_command ON node_configuration_confirmations(command_id);
```

#### 10. `cycle_analytics` - Аналитика циклов
```sql
CREATE TABLE cycle_analytics (
    id BIGSERIAL PRIMARY KEY,
    cycle_id BIGINT NOT NULL UNIQUE,
    
    -- Статистика параметров
    avg_ph DECIMAL(4,2),
    avg_ec DECIMAL(4,2),
    avg_temp DECIMAL(4,2),
    avg_humidity DECIMAL(4,2),
    avg_co2 INTEGER,
    avg_lux INTEGER,
    
    -- Отклонения от целевых
    ph_deviations JSONB,                     -- {"stage_1": 0.3, "stage_2": 0.1}
    ec_deviations JSONB,
    temp_deviations JSONB,
    
    -- Ресурсы
    total_water_liters DECIMAL(10,2),
    total_nutrients_ml JSONB,                -- {"ph_up": 120, "ph_down": 80, "ec_a": 500}
    total_energy_kwh DECIMAL(10,2),
    
    -- События
    alerts_count INTEGER DEFAULT 0,
    errors_count INTEGER DEFAULT 0,
    manual_adjustments INTEGER DEFAULT 0,
    
    -- Результат
    harvest_weight_kg DECIMAL(10,2),         -- Вес урожая
    quality_rating INTEGER,                  -- 1-5 звезд
    user_notes TEXT,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (cycle_id) REFERENCES growth_cycles(id) ON DELETE CASCADE
);
```

#### 11. `cycle_photos` - Фото-дневник (опционально)
```sql
CREATE TABLE cycle_photos (
    id BIGSERIAL PRIMARY KEY,
    cycle_id BIGINT NOT NULL,
    stage_id BIGINT,
    
    filename VARCHAR(255) NOT NULL,
    file_path VARCHAR(500) NOT NULL,
    file_size INTEGER,
    
    caption TEXT,
    taken_at TIMESTAMP NOT NULL,
    day_of_cycle INTEGER,                    -- День цикла
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (cycle_id) REFERENCES growth_cycles(id) ON DELETE CASCADE,
    FOREIGN KEY (stage_id) REFERENCES growth_stages(id)
);

CREATE INDEX idx_cycle_photos_cycle ON cycle_photos(cycle_id);
```

---

## 🔧 BACKEND IMPLEMENTATION

### Models

#### 1. `Zone.php`
```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\HasOne;
use Illuminate\Database\Eloquent\Relations\HasMany;

class Zone extends Model
{
    protected $fillable = [
        'name',
        'description',
        'location',
        'zone_type',
        'reservoir_volume_liters',
        'growing_area_m2',
        'plant_capacity',
        'assigned_nodes',
        'is_active',
        'is_available',
        'current_cycle_id',
        'image_url',
        'notes',
    ];

    protected $casts = [
        'assigned_nodes' => 'array',
        'reservoir_volume_liters' => 'decimal:2',
        'growing_area_m2' => 'decimal:2',
        'plant_capacity' => 'integer',
        'is_active' => 'boolean',
        'is_available' => 'boolean',
    ];

    /**
     * Текущий активный цикл
     */
    public function currentCycle(): HasOne
    {
        return $this->hasOne(GrowthCycle::class, 'zone_id')
                    ->where('status', 'active');
    }

    /**
     * Все циклы зоны
     */
    public function cycles(): HasMany
    {
        return $this->hasMany(GrowthCycle::class, 'zone_id');
    }

    /**
     * История назначения узлов
     */
    public function nodeAssignments(): HasMany
    {
        return $this->hasMany(ZoneNodeAssignment::class, 'zone_id');
    }

    /**
     * Получить узел по роли
     */
    public function getNodeByRole(string $role): ?string
    {
        return data_get($this->assigned_nodes, $role);
    }

    /**
     * Проверка доступности для нового цикла
     */
    public function isAvailableForCycle(): bool
    {
        return $this->is_available && 
               $this->is_active && 
               !$this->currentCycle;
    }

    /**
     * Получить все узлы зоны
     */
    public function getAllNodes(): array
    {
        return array_filter([
            $this->getNodeByRole('ph_node'),
            $this->getNodeByRole('climate_node'),
            $this->getNodeByRole('relay_node'),
            $this->getNodeByRole('water_node'),
            $this->getNodeByRole('display_node'),
        ]);
    }

    /**
     * Проверка занятости узлов
     */
    public function checkNodesAvailability(): array
    {
        $nodes = $this->getAllNodes();
        $busy = [];

        foreach ($nodes as $nodeId) {
            // Проверяем, не используется ли узел в другой активной зоне
            $otherZone = self::where('id', '!=', $this->id)
                ->where('is_active', true)
                ->whereJsonContains('assigned_nodes', $nodeId)
                ->whereHas('currentCycle')
                ->first();

            if ($otherZone) {
                $busy[$nodeId] = [
                    'zone_name' => $otherZone->name,
                    'zone_id' => $otherZone->id,
                ];
            }
        }

        return $busy;
    }

    /**
     * Доступные зоны
     */
    public function scopeAvailable($query)
    {
        return $query->where('is_available', true)
                     ->where('is_active', true)
                     ->doesntHave('currentCycle');
    }
}
```

#### 2. `GrowthStage.php`
```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class GrowthStage extends Model
{
    protected $fillable = [
        'name',
        'name_ru',
        'description',
        'icon',
        'order_index',
    ];

    protected $casts = [
        'order_index' => 'integer',
    ];

    /**
     * Пресеты, использующие эту стадию
     */
    public function presetStages()
    {
        return $this->hasMany(PresetStage::class, 'stage_id');
    }

    /**
     * Упорядочить по порядку
     */
    public function scopeOrdered($query)
    {
        return $query->orderBy('order_index');
    }
}
```

#### 3. `GrowthPreset.php`
```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\HasMany;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class GrowthPreset extends Model
{
    protected $fillable = [
        'name',
        'name_ru',
        'description',
        'icon',
        'image_url',
        'is_system',
        'is_public',
        'user_id',
        'total_duration_days',
        'difficulty_level',
        'popularity_score',
    ];

    protected $casts = [
        'is_system' => 'boolean',
        'is_public' => 'boolean',
        'total_duration_days' => 'integer',
        'popularity_score' => 'integer',
    ];

    /**
     * Стадии пресета
     */
    public function stages(): HasMany
    {
        return $this->hasMany(PresetStage::class, 'preset_id')
                    ->orderBy('order_index');
    }

    /**
     * Автор пресета
     */
    public function user(): BelongsTo
    {
        return $this->belongsTo(User::class);
    }

    /**
     * Циклы, использующие этот пресет
     */
    public function cycles(): HasMany
    {
        return $this->hasMany(GrowthCycle::class, 'preset_id');
    }

    /**
     * Системные пресеты
     */
    public function scopeSystem($query)
    {
        return $query->where('is_system', true);
    }

    /**
     * Пользовательские пресеты
     */
    public function scopeCustom($query)
    {
        return $query->where('is_system', false);
    }

    /**
     * Расчет общей длительности
     */
    public function calculateTotalDuration(): int
    {
        return $this->stages()->sum('duration_days');
    }
}
```

#### 4. `PresetStage.php`
```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class PresetStage extends Model
{
    protected $fillable = [
        'preset_id',
        'stage_id',
        'duration_days',
        'duration_days_min',
        'duration_days_max',
        'target_params',
        'description',
        'tips',
        'order_index',
    ];

    protected $casts = [
        'duration_days' => 'integer',
        'duration_days_min' => 'integer',
        'duration_days_max' => 'integer',
        'target_params' => 'array',
        'order_index' => 'integer',
    ];

    public function preset()
    {
        return $this->belongsTo(GrowthPreset::class, 'preset_id');
    }

    public function stage()
    {
        return $this->belongsTo(GrowthStage::class, 'stage_id');
    }

    /**
     * Получить параметр по ключу
     */
    public function getTargetParam(string $key, $default = null)
    {
        return data_get($this->target_params, $key, $default);
    }

    /**
     * Установить параметр
     */
    public function setTargetParam(string $key, $value): void
    {
        $params = $this->target_params;
        data_set($params, $key, $value);
        $this->target_params = $params;
    }
}
```

#### 5. `GrowthCycle.php`
```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Carbon\Carbon;

class GrowthCycle extends Model
{
    protected $fillable = [
        'preset_id',
        'zone_id',
        'name',
        'description',
        'current_stage_id',
        'current_preset_stage_id',
        'started_at',
        'current_stage_started_at',
        'estimated_completion_at',
        'completed_at',
        'status',
        'plant_count',
        'node_overrides',
        'total_days',
        'delays_count',
        'manual_interventions',
        'user_id',
    ];

    protected $casts = [
        'node_overrides' => 'array',
        'plant_count' => 'integer',
        'started_at' => 'datetime',
        'current_stage_started_at' => 'datetime',
        'estimated_completion_at' => 'datetime',
        'completed_at' => 'datetime',
        'total_days' => 'integer',
        'delays_count' => 'integer',
        'manual_interventions' => 'integer',
    ];

    public function preset()
    {
        return $this->belongsTo(GrowthPreset::class, 'preset_id');
    }

    public function zone()
    {
        return $this->belongsTo(Zone::class, 'zone_id');
    }

    public function currentStage()
    {
        return $this->belongsTo(GrowthStage::class, 'current_stage_id');
    }

    public function currentPresetStage()
    {
        return $this->belongsTo(PresetStage::class, 'current_preset_stage_id');
    }

    public function stageHistory()
    {
        return $this->hasMany(CycleStageHistory::class, 'cycle_id')
                    ->orderBy('started_at');
    }

    public function transitionProposals()
    {
        return $this->hasMany(StageTransitionProposal::class, 'cycle_id');
    }

    public function confirmations()
    {
        return $this->hasMany(NodeConfigurationConfirmation::class, 'cycle_id');
    }

    public function analytics()
    {
        return $this->hasOne(CycleAnalytics::class, 'cycle_id');
    }

    public function photos()
    {
        return $this->hasMany(CyclePhoto::class, 'cycle_id');
    }

    /**
     * Активные циклы
     */
    public function scopeActive($query)
    {
        return $query->where('status', 'active');
    }

    /**
     * Получить узлы цикла (из зоны + overrides)
     */
    public function getAssignedNodes(): array
    {
        $zoneNodes = $this->zone->assigned_nodes ?? [];
        
        if ($this->node_overrides) {
            // Применяем переопределения
            foreach ($this->node_overrides as $role => $nodeId) {
                if ($nodeId === null) {
                    unset($zoneNodes[$role]);
                } else {
                    $zoneNodes[$role] = $nodeId;
                }
            }
        }
        
        return $zoneNodes;
    }

    /**
     * Получить узел по роли
     */
    public function getNodeByRole(string $role): ?string
    {
        $nodes = $this->getAssignedNodes();
        return $nodes[$role] ?? null;
    }

    /**
     * Получить прогресс текущей стадии (0-100)
     */
    public function getCurrentStageProgress(): int
    {
        if (!$this->current_stage_started_at || !$this->currentPresetStage) {
            return 0;
        }

        $daysInStage = Carbon::now()->diffInDays($this->current_stage_started_at);
        $totalDays = $this->currentPresetStage->duration_days;

        return min(100, round(($daysInStage / $totalDays) * 100));
    }

    /**
     * Получить общий прогресс цикла (0-100)
     */
    public function getTotalProgress(): int
    {
        if (!$this->started_at || !$this->estimated_completion_at) {
            return 0;
        }

        $totalDays = $this->started_at->diffInDays($this->estimated_completion_at);
        $elapsedDays = $this->started_at->diffInDays(Carbon::now());

        return min(100, round(($elapsedDays / $totalDays) * 100));
    }

    /**
     * Дней до завершения стадии
     */
    public function getDaysUntilNextStage(): int
    {
        if (!$this->currentPresetStage || !$this->current_stage_started_at) {
            return 0;
        }

        $expectedEnd = $this->current_stage_started_at->addDays(
            $this->currentPresetStage->duration_days
        );

        return max(0, Carbon::now()->diffInDays($expectedEnd, false));
    }
}
```

### Controllers

#### `ZoneController.php`
```php
<?php

namespace App\Http\Controllers;

use App\Models\Zone;
use Illuminate\Http\Request;

class ZoneController extends Controller
{
    /**
     * Список всех зон
     */
    public function index(Request $request)
    {
        $query = Zone::with(['currentCycle.preset']);

        if ($request->has('available')) {
            $query->available();
        }

        $zones = $query->get();

        // Добавляем информацию о доступности узлов
        $zones->each(function ($zone) {
            $zone->busy_nodes = $zone->checkNodesAvailability();
        });

        return response()->json($zones);
    }

    /**
     * Создать зону
     */
    public function store(Request $request)
    {
        $validated = $request->validate([
            'name' => 'required|string|max:100',
            'zone_type' => 'required|in:nft,dwc,ebb_flow,drip,other',
            'reservoir_volume_liters' => 'nullable|numeric|min:0',
            'growing_area_m2' => 'nullable|numeric|min:0',
            'plant_capacity' => 'nullable|integer|min:0',
            'assigned_nodes' => 'required|array',
            'assigned_nodes.ph_node' => 'required|string|exists:nodes,node_id',
            'assigned_nodes.climate_node' => 'nullable|string|exists:nodes,node_id',
            'assigned_nodes.relay_node' => 'nullable|string|exists:nodes,node_id',
            'assigned_nodes.water_node' => 'nullable|string|exists:nodes,node_id',
        ]);

        $zone = Zone::create($validated);

        return response()->json($zone, 201);
    }

    /**
     * Обновить зону
     */
    public function update(Request $request, $id)
    {
        $zone = Zone::findOrFail($id);

        // Проверка: нельзя изменять узлы если есть активный цикл
        if ($zone->currentCycle && $request->has('assigned_nodes')) {
            return response()->json([
                'message' => 'Cannot change nodes while cycle is active'
            ], 422);
        }

        $validated = $request->validate([
            'name' => 'sometimes|string|max:100',
            'description' => 'nullable|string',
            'is_available' => 'sometimes|boolean',
            'assigned_nodes' => 'sometimes|array',
        ]);

        $zone->update($validated);

        return response()->json($zone);
    }

    /**
     * Проверить доступность зоны для нового цикла
     */
    public function checkAvailability($id)
    {
        $zone = Zone::with('currentCycle')->findOrFail($id);

        $available = $zone->isAvailableForCycle();
        $busyNodes = $zone->checkNodesAvailability();

        return response()->json([
            'available' => $available && empty($busyNodes),
            'reasons' => [
                'has_active_cycle' => !is_null($zone->currentCycle),
                'is_inactive' => !$zone->is_active,
                'is_unavailable' => !$zone->is_available,
                'busy_nodes' => $busyNodes,
            ],
            'zone' => $zone,
        ]);
    }
}
```

#### `GrowthPresetController.php`
```php
<?php

namespace App\Http\Controllers;

use App\Models\GrowthPreset;
use App\Models\PresetStage;
use Illuminate\Http\Request;

class GrowthPresetController extends Controller
{
    /**
     * Список всех пресетов
     */
    public function index(Request $request)
    {
        $query = GrowthPreset::with(['stages.stage', 'user']);

        // Фильтры
        if ($request->has('system')) {
            $query->system();
        }

        if ($request->has('user_id')) {
            $query->where('user_id', $request->user_id);
        }

        $presets = $query->get();

        return response()->json($presets);
    }

    /**
     * Получить пресет со всеми стадиями
     */
    public function show($id)
    {
        $preset = GrowthPreset::with(['stages.stage'])->findOrFail($id);
        return response()->json($preset);
    }

    /**
     * Создать пресет
     */
    public function store(Request $request)
    {
        $validated = $request->validate([
            'name' => 'required|string|max:100',
            'name_ru' => 'nullable|string|max:100',
            'description' => 'nullable|string',
            'icon' => 'nullable|string|max:50',
            'difficulty_level' => 'nullable|in:easy,medium,hard',
            'stages' => 'required|array|min:1',
            'stages.*.stage_id' => 'required|exists:growth_stages,id',
            'stages.*.duration_days' => 'required|integer|min:1',
            'stages.*.target_params' => 'required|array',
        ]);

        $preset = GrowthPreset::create([
            ...$validated,
            'user_id' => auth()->id(),
            'is_system' => false,
            'total_duration_days' => array_sum(array_column($validated['stages'], 'duration_days')),
        ]);

        foreach ($validated['stages'] as $index => $stageData) {
            PresetStage::create([
                'preset_id' => $preset->id,
                'stage_id' => $stageData['stage_id'],
                'duration_days' => $stageData['duration_days'],
                'target_params' => $stageData['target_params'],
                'order_index' => $index,
            ]);
        }

        return response()->json($preset->load('stages.stage'), 201);
    }

    /**
     * Клонировать пресет
     */
    public function clone($id, Request $request)
    {
        $original = GrowthPreset::with('stages')->findOrFail($id);

        $validated = $request->validate([
            'name' => 'required|string|max:100',
        ]);

        $clone = GrowthPreset::create([
            'name' => $validated['name'],
            'name_ru' => $validated['name'],
            'description' => $original->description,
            'icon' => $original->icon,
            'user_id' => auth()->id(),
            'is_system' => false,
            'difficulty_level' => $original->difficulty_level,
        ]);

        foreach ($original->stages as $stage) {
            PresetStage::create([
                'preset_id' => $clone->id,
                'stage_id' => $stage->stage_id,
                'duration_days' => $stage->duration_days,
                'target_params' => $stage->target_params,
                'description' => $stage->description,
                'tips' => $stage->tips,
                'order_index' => $stage->order_index,
            ]);
        }

        return response()->json($clone->load('stages.stage'), 201);
    }

    /**
     * Обновить пресет
     */
    public function update(Request $request, $id)
    {
        $preset = GrowthPreset::findOrFail($id);

        // Проверка прав (только свой или админ)
        if ($preset->is_system || ($preset->user_id !== auth()->id() && !auth()->user()->is_admin)) {
            return response()->json(['message' => 'Forbidden'], 403);
        }

        $validated = $request->validate([
            'name' => 'sometimes|string|max:100',
            'description' => 'nullable|string',
            'icon' => 'nullable|string|max:50',
        ]);

        $preset->update($validated);

        return response()->json($preset);
    }

    /**
     * Удалить пресет
     */
    public function destroy($id)
    {
        $preset = GrowthPreset::findOrFail($id);

        if ($preset->is_system) {
            return response()->json(['message' => 'Cannot delete system preset'], 403);
        }

        if ($preset->user_id !== auth()->id() && !auth()->user()->is_admin) {
            return response()->json(['message' => 'Forbidden'], 403);
        }

        // Проверка активных циклов
        if ($preset->cycles()->where('status', 'active')->exists()) {
            return response()->json([
                'message' => 'Cannot delete preset with active cycles'
            ], 422);
        }

        $preset->delete();

        return response()->json(['message' => 'Preset deleted']);
    }
}
```

#### `GrowthCycleController.php`
```php
<?php

namespace App\Http\Controllers;

use App\Models\GrowthCycle;
use App\Models\Zone;
use App\Models\GrowthPreset;
use App\Services\GrowthCycleService;
use Illuminate\Http\Request;
use Carbon\Carbon;

class GrowthCycleController extends Controller
{
    protected $cycleService;

    public function __construct(GrowthCycleService $cycleService)
    {
        $this->cycleService = $cycleService;
    }

    /**
     * Список активных циклов
     */
    public function index(Request $request)
    {
        $query = GrowthCycle::with([
            'preset',
            'zone',
            'currentStage',
            'currentPresetStage',
        ]);

        if ($request->has('status')) {
            $query->where('status', $request->status);
        }

        if ($request->has('zone_id')) {
            $query->where('zone_id', $request->zone_id);
        }

        $cycles = $query->get();

        // Добавляем прогресс
        $cycles->each(function ($cycle) {
            $cycle->stage_progress = $cycle->getCurrentStageProgress();
            $cycle->total_progress = $cycle->getTotalProgress();
            $cycle->days_until_next_stage = $cycle->getDaysUntilNextStage();
            $cycle->assigned_nodes_resolved = $cycle->getAssignedNodes();
        });

        return response()->json($cycles);
    }

    /**
     * Начать новый цикл
     */
    public function store(Request $request)
    {
        $validated = $request->validate([
            'preset_id' => 'required|exists:growth_presets,id',
            'zone_id' => 'required|exists:zones,id',
            'name' => 'required|string|max:150',
            'description' => 'nullable|string',
            'plant_count' => 'nullable|integer|min:1',
            'node_overrides' => 'nullable|array',
            'started_at' => 'nullable|date',
        ]);

        // Проверка доступности зоны
        $zone = Zone::findOrFail($validated['zone_id']);
        if (!$zone->isAvailableForCycle()) {
            return response()->json([
                'message' => 'Zone is not available for new cycle'
            ], 422);
        }

        $preset = GrowthPreset::with('stages')->findOrFail($validated['preset_id']);

        // Создаем цикл
        $cycle = $this->cycleService->startCycle($preset, $zone, [
            'name' => $validated['name'],
            'description' => $validated['description'] ?? null,
            'plant_count' => $validated['plant_count'] ?? null,
            'node_overrides' => $validated['node_overrides'] ?? null,
            'started_at' => $validated['started_at'] ?? Carbon::now(),
            'user_id' => auth()->id(),
        ]);

        return response()->json($cycle->load([
            'preset',
            'zone',
            'currentStage',
            'currentPresetStage',
        ]), 201);
    }

    /**
     * Детали цикла
     */
    public function show($id)
    {
        $cycle = GrowthCycle::with([
            'preset.stages.stage',
            'zone',
            'currentStage',
            'currentPresetStage',
            'stageHistory.stage',
            'transitionProposals' => fn($q) => $q->where('status', 'pending'),
            'confirmations' => fn($q) => $q->latest()->limit(10),
            'analytics',
        ])->findOrFail($id);

        $cycle->stage_progress = $cycle->getCurrentStageProgress();
        $cycle->total_progress = $cycle->getTotalProgress();
        $cycle->days_until_next_stage = $cycle->getDaysUntilNextStage();
        $cycle->assigned_nodes_resolved = $cycle->getAssignedNodes();

        return response()->json($cycle);
    }

    /**
     * Поставить на паузу / возобновить
     */
    public function togglePause($id)
    {
        $cycle = GrowthCycle::findOrFail($id);

        if ($cycle->status === 'active') {
            $cycle->update(['status' => 'paused']);
            $message = 'Cycle paused';
        } elseif ($cycle->status === 'paused') {
            $cycle->update(['status' => 'active']);
            $message = 'Cycle resumed';
        } else {
            return response()->json([
                'message' => 'Cycle cannot be paused/resumed'
            ], 422);
        }

        return response()->json([
            'message' => $message,
            'cycle' => $cycle,
        ]);
    }

    /**
     * Завершить цикл
     */
    public function complete($id, Request $request)
    {
        $validated = $request->validate([
            'harvest_weight_kg' => 'nullable|numeric|min:0',
            'quality_rating' => 'nullable|integer|min:1|max:5',
            'user_notes' => 'nullable|string',
        ]);

        $cycle = GrowthCycle::findOrFail($id);

        $this->cycleService->completeCycle($cycle, $validated);

        return response()->json([
            'message' => 'Cycle completed',
            'cycle' => $cycle->fresh(),
        ]);
    }
}
```

---

## 📅 ПЛАН РАБОТ

### Этап 1: Database & Models (2-3 дня)

**День 1:**
- [ ] Создать миграцию для `zones` ⭐ (новая таблица)
- [ ] Создать миграции для остальных таблиц (10 таблиц)
- [ ] Создать Model Zone с методами проверки доступности
- [ ] Создать остальные Models (9 моделей)
- [ ] Seeders для базовых стадий

**День 2:**
- [ ] Seeders для примеров зон (Зона 1 NFT, Зона 2 DWC, Зона 3 Drip)
- [ ] Seeders для 4 базовых пресетов (Салат, Клубника, Томаты, Огурцы)
- [ ] Тестовые данные для разработки
- [ ] Написать тесты для Zone модели

**День 3:**
- [ ] Доработка связей моделей
- [ ] Вспомогательные методы (getters, scopes)
- [ ] Логика проверки занятости узлов ⭐
- [ ] Документация моделей

### Этап 2: Backend API (3-4 дня)

**День 4-5:**
- [ ] ZoneController (CRUD + check availability) ⭐
- [ ] PresetController (CRUD)
- [ ] CycleController (CRUD с привязкой к зонам) ⭐
- [ ] StageTransitionController
- [ ] AnalyticsController

**День 6:**
- [ ] GrowthCycleService (start/complete cycle) ⭐
- [ ] ConfigurationApplierService (применение к узлам зоны) ⭐
- [ ] ConfirmationTrackerService ⭐
- [ ] NotificationService

**День 7:**
- [ ] Scheduler commands (проверка переходов по зонам)
- [ ] Валидация конфликтов узлов между зонами ⭐
- [ ] API тесты
- [ ] API документация

### Этап 3: Frontend (4-5 дней)

**День 8-9:**
- [ ] Pinia store zones.ts ⭐ (управление зонами)
- [ ] Pinia store growth.ts (циклы, пресеты)
- [ ] API service (zones-api.ts, growth-api.ts)
- [ ] Базовые компоненты

**День 10-11:**
- [ ] ZoneSelector компонент ⭐ (выбор зоны с проверкой доступности)
- [ ] ZoneCard компонент (карточка зоны с текущим циклом)
- [ ] Страницы (Planner, Library, Detail, Zones) ⭐
- [ ] Timeline компонент
- [ ] Dialogs (transition, create cycle с выбором зоны) ⭐

**День 12:**
- [ ] Визуализация множественных зон ⭐
- [ ] Dashboard с overview всех зон
- [ ] Стили и анимации
- [ ] Адаптив
- [ ] Тестирование UI

### Этап 4: Интеграция (2-3 дня)

**День 13:**
- [ ] MQTT integration
- [ ] Система подтверждения ⭐
- [ ] WebSocket events

**День 14:**
- [ ] Уведомления (Web Push, Telegram, SMS)
- [ ] Полное тестирование

**День 15:**
- [ ] Аналитика и графики
- [ ] Документация
- [ ] Деплой

---

**ИТОГО: 15-17 рабочих дней (3-3.5 недели)**

---

## 🎨 UI КОНЦЕПЦИЯ С ЗОНАМИ

### Dashboard с несколькими зонами

```
┌─────────────────────────────────────────────────────────────┐
│  🌱 Growth Planner - Dashboard                         [+]  │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Активные зоны:                                             │
│                                                              │
│  ┌────────────────────┐  ┌────────────────────┐            │
│  │ 🏠 Зона 1 - NFT    │  │ 🏠 Зона 2 - DWC    │            │
│  │ 100л | 2м² | 20шт  │  │ 50л | 1м² | 10шт   │            │
│  │                    │  │                    │            │
│  │ 🍅 Томаты Зима #1  │  │ 🥬 Салат Партия 2  │            │
│  │ Цветение (12/28д)  │  │ Вегетация (5/14д)  │            │
│  │ [████████░░░] 43%  │  │ [████░░░░░░░] 36%  │            │
│  │                    │  │                    │            │
│  │ Узлы: ✓✓✓✓        │  │ Узлы: ✓✓✓         │            │
│  │ pH_001, Climate_01 │  │ pH_002, Climate_02 │            │
│  └────────────────────┘  └────────────────────┘            │
│                                                              │
│  ┌────────────────────┐                                     │
│  │ 🏠 Зона 3 - Drip   │                                     │
│  │ 200л | 5м² | 50шт  │                                     │
│  │                    │                                     │
│  │ 💤 Свободна        │                                     │
│  │ [Начать цикл]     │                                     │
│  │                    │                                     │
│  │ Узлы: ✓✓✓✓        │                                     │
│  │ pH_003, Climate_03 │                                     │
│  └────────────────────┘                                     │
│                                                              │
│  [+ Добавить зону]                                          │
└─────────────────────────────────────────────────────────────┘
```

### Создание нового цикла - выбор зоны

```
┌─────────────────────────────────────────────────────────────┐
│  Начать новый цикл выращивания                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Шаг 1: Выберите зону                                       │
│                                                              │
│  ┌────────────────┐  ┌────────────────┐  ┌───────────────┐ │
│  │ ✓ Зона 1 NFT   │  │ ⚠ Зона 2 DWC   │  │ ✓ Зона 3 Drip │ │
│  │ Свободна       │  │ Занята         │  │ Свободна      │ │
│  │ 100л, 20 раст. │  │ (Салат Партия2)│  │ 200л, 50 раст.│ │
│  │ [Выбрать]      │  │ [Недоступна]   │  │ [Выбрать]     │ │
│  └────────────────┘  └────────────────┘  └───────────────┘ │
│                                                              │
│  Шаг 2: Выберите пресет                                     │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐                           │
│  │ 🍅  │ │ 🥬  │ │ 🍓  │ │ 🥒  │                           │
│  └─────┘ └─────┘ └─────┘ └─────┘                           │
│                                                              │
│  Шаг 3: Настройки                                           │
│  Название: [________________]                               │
│  Количество растений: [20] (макс 20 для выбранной зоны)    │
│  Дата начала: [06.11.2024]                                  │
│                                                              │
│  Узлы зоны:                                                 │
│  • pH/EC: ph_001 ✓                                          │
│  • Климат: climate_001 ✓                                    │
│  • Реле: relay_001 ✓                                        │
│  • Вода: water_001 ✓                                        │
│                                                              │
│  [Переопределить узлы] (опционально)                        │
│                                                              │
│  [Отмена]  [Начать цикл]                                    │
└─────────────────────────────────────────────────────────────┘
```

---

**Готов начать реализацию! С чего начнём?**

1. 🗄️ Создать миграции (включая zones)?
2. 🌱 Seeders с зонами и пресетами?
3. 🎨 Frontend mock-up сначала?


