# 🏗️ ГЛОБАЛЬНАЯ ИНТЕГРАЦИЯ ЗОНИРОВАНИЯ

**Дата:** 6 ноября 2025  
**Версия:** 1.0  
**Приоритет:** КРИТИЧЕСКИЙ - Архитектурное изменение

---

## 🎯 КОНЦЕПЦИЯ

### Старая архитектура:
```
1 Root Node (root_001)
    └── 1 Mesh сеть (HYDRO1)
        ├── climate_001
        ├── ph_001
        ├── relay_001
        └── water_001
```

### Новая архитектура с зонированием:
```
┌─────────────────────────────────────────────────────────┐
│ Root Node #1 (root_001) - Зона 1 NFT                    │
│   Mesh ID: HYDRO1_ZONE1                                 │
│   ├── ph_001       (pH/EC узел)                         │
│   ├── climate_001  (Климат узел)                        │
│   ├── relay_001    (Реле)                               │
│   └── water_001    (Вода)                               │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ Root Node #2 (root_002) - Зона 2 DWC                    │
│   Mesh ID: HYDRO1_ZONE2                                 │
│   ├── ph_002       (pH/EC узел)                         │
│   ├── climate_002  (Климат узел)                        │
│   └── relay_002    (Реле)                               │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ Root Node #3 (root_003) - Зона 3 Капельный полив        │
│   Mesh ID: HYDRO1_ZONE3                                 │
│   ├── ph_003       (pH/EC узел)                         │
│   ├── climate_003  (Климат узел)                        │
│   ├── relay_003    (Реле)                               │
│   └── water_003    (Вода)                               │
└─────────────────────────────────────────────────────────┘

              ↓ MQTT
              
┌─────────────────────────────────────────────────────────┐
│                  Server Backend                         │
│                                                          │
│  Zones:                                                  │
│  • Зона 1 → root_001 → HYDRO1_ZONE1                     │
│  • Зона 2 → root_002 → HYDRO1_ZONE2                     │
│  • Зона 3 → root_003 → HYDRO1_ZONE3                     │
└─────────────────────────────────────────────────────────┘
```

---

## 📊 DATABASE CHANGES

### 1. Обновление таблицы `zones`

```sql
-- Добавляем привязку к Root Node
ALTER TABLE zones ADD COLUMN root_node_id VARCHAR(50);
ALTER TABLE zones ADD COLUMN mesh_network_id VARCHAR(50); -- "HYDRO1_ZONE1"
ALTER TABLE zones ADD COLUMN mqtt_topic_prefix VARCHAR(100); -- "hydro/zone1/"

-- Добавляем индексы
CREATE INDEX idx_zones_root_node ON zones(root_node_id);
CREATE UNIQUE INDEX idx_zones_mesh_network ON zones(mesh_network_id);

-- Ограничение: один Root Node = одна зона
ALTER TABLE zones ADD CONSTRAINT unique_root_per_zone UNIQUE (root_node_id);

-- Внешний ключ на nodes
ALTER TABLE zones ADD CONSTRAINT fk_zones_root_node 
    FOREIGN KEY (root_node_id) REFERENCES nodes(node_id) ON DELETE RESTRICT;
```

**Обновленная структура `zones`:**
```sql
CREATE TABLE zones (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    
    -- ⭐ НОВОЕ: Привязка к Root Node
    root_node_id VARCHAR(50) NOT NULL UNIQUE,  -- "root_001"
    mesh_network_id VARCHAR(50) NOT NULL UNIQUE, -- "HYDRO1_ZONE1"
    mqtt_topic_prefix VARCHAR(100),             -- "hydro/zone1/"
    
    -- Физическое расположение
    location VARCHAR(100),
    zone_type VARCHAR(50),
    
    -- Объем системы
    reservoir_volume_liters DECIMAL(10,2),
    growing_area_m2 DECIMAL(10,2),
    plant_capacity INTEGER,
    
    -- Узлы зоны
    assigned_nodes JSONB NOT NULL DEFAULT '{}',
    
    -- Статус
    is_active BOOLEAN DEFAULT true,
    is_available BOOLEAN DEFAULT true,
    current_cycle_id BIGINT,
    
    -- Метаданные
    image_url VARCHAR(255),
    notes TEXT,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (root_node_id) REFERENCES nodes(node_id) ON DELETE RESTRICT,
    FOREIGN KEY (current_cycle_id) REFERENCES growth_cycles(id) ON DELETE SET NULL
);
```

### 2. Обновление таблицы `nodes`

```sql
-- Добавляем привязку к Root Node
ALTER TABLE nodes ADD COLUMN root_node_id VARCHAR(50);

-- Для Root Node: root_node_id = node_id (сам себя)
-- Для обычных узлов: root_node_id = ID родительского Root

-- Индекс для быстрого поиска узлов зоны
CREATE INDEX idx_nodes_root_node ON nodes(root_node_id);

-- Можем удалить старое поле zone (string) - заменено на root_node_id
-- ALTER TABLE nodes DROP COLUMN zone;
```

**Обновленная структура `nodes`:**
```sql
CREATE TABLE nodes (
    id BIGSERIAL PRIMARY KEY,
    node_id VARCHAR(50) NOT NULL UNIQUE,    -- "ph_001", "root_001"
    node_type VARCHAR(50) NOT NULL,         -- "ph_ec", "climate", "root"
    
    -- ⭐ НОВОЕ: Привязка к Root Node
    root_node_id VARCHAR(50),               -- "root_001" (для всех узлов зоны)
    
    -- Старое поле zone можно оставить для совместимости
    zone VARCHAR(100),                      -- Deprecated, но для обратной совместимости
    
    mac_address VARCHAR(17),
    online BOOLEAN DEFAULT false,
    last_seen_at TIMESTAMP,
    
    config JSONB,
    metadata JSONB,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    -- Внешний ключ на Root Node
    FOREIGN KEY (root_node_id) REFERENCES nodes(node_id) ON DELETE SET NULL
);
```

### 3. Новая таблица `root_nodes` (опционально - для метаданных Root)

```sql
CREATE TABLE root_nodes (
    id BIGSERIAL PRIMARY KEY,
    node_id VARCHAR(50) NOT NULL UNIQUE,    -- "root_001"
    zone_id BIGINT NOT NULL UNIQUE,         -- Связь с zones
    
    -- Mesh сеть
    mesh_network_id VARCHAR(50) NOT NULL,   -- "HYDRO1_ZONE1"
    mesh_password VARCHAR(100),             -- Пароль mesh сети
    mesh_channel INTEGER,                   -- WiFi канал
    
    -- MQTT
    mqtt_topic_prefix VARCHAR(100),         -- "hydro/zone1/"
    mqtt_qos INTEGER DEFAULT 1,
    
    -- WiFi Router
    router_ssid VARCHAR(100),
    router_channel INTEGER,
    
    -- Статус
    is_primary BOOLEAN DEFAULT false,       -- Главный Root (для fallback)
    max_mesh_connections INTEGER DEFAULT 6,
    
    -- Метрики
    mesh_nodes_count INTEGER DEFAULT 0,
    last_discovery_at TIMESTAMP,
    
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    
    FOREIGN KEY (node_id) REFERENCES nodes(node_id) ON DELETE CASCADE,
    FOREIGN KEY (zone_id) REFERENCES zones(id) ON DELETE CASCADE
);

CREATE INDEX idx_root_nodes_zone ON root_nodes(zone_id);
CREATE UNIQUE INDEX idx_root_nodes_mesh ON root_nodes(mesh_network_id);
```

---

## 🔧 ESP32 FIRMWARE CHANGES

### 1. `common/mesh_config/mesh_config.h` - Динамическая конфигурация

**Старый подход:**
```c
#define MESH_NETWORK_ID "HYDRO1"  // Одна сеть для всех
```

**Новый подход:**
```c
// ⚠️ КАЖДЫЙ ROOT И ЕГО УЗЛЫ ДОЛЖНЫ ИМЕТЬ УНИКАЛЬНЫЙ MESH_NETWORK_ID!

// Для Root Node #1 и его узлов:
#define MESH_NETWORK_ID "HYDRO1_ZONE1"

// Для Root Node #2 и его узлов:
#define MESH_NETWORK_ID "HYDRO1_ZONE2"

// Для Root Node #3 и его узлов:
#define MESH_NETWORK_ID "HYDRO1_ZONE3"

// Root Node ID (определяется при компиляции или в NVS)
#ifndef ROOT_NODE_ID
#define ROOT_NODE_ID "root_001"  // Default для Zone 1
#endif

// Зона определяется автоматически из MESH_NETWORK_ID или ROOT_NODE_ID
```

**Еще лучше - через NVS:**
```c
// В mesh_config.h:
// Дефолтные значения (можно переопределить через NVS)
#define DEFAULT_MESH_NETWORK_ID "HYDRO1_ZONE1"
#define DEFAULT_ROOT_NODE_ID    "root_001"

// При инициализации читаем из NVS:
esp_err_t mesh_config_load_zone_settings(mesh_zone_config_t *config) {
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("mesh_config", NVS_READONLY, &nvs);
    
    if (ret == ESP_OK) {
        // Читаем zone_id, root_node_id, mesh_network_id из NVS
        size_t len = sizeof(config->mesh_network_id);
        nvs_get_str(nvs, "mesh_id", config->mesh_network_id, &len);
        
        len = sizeof(config->root_node_id);
        nvs_get_str(nvs, "root_id", config->root_node_id, &len);
        
        nvs_close(nvs);
    } else {
        // Используем дефолтные значения
        strcpy(config->mesh_network_id, DEFAULT_MESH_NETWORK_ID);
        strcpy(config->root_node_id, DEFAULT_ROOT_NODE_ID);
    }
    
    return ESP_OK;
}
```

### 2. `common/mesh_protocol/mesh_protocol.h` - Добавление zone_id в протокол

```c
/**
 * @brief Базовая структура сообщения
 */
typedef struct {
    mesh_msg_type_t type;
    char node_id[32];
    char root_node_id[32];    // ⭐ НОВОЕ: ID Root Node (зоны)
    char zone_id[32];         // ⭐ НОВОЕ: ID зоны (опционально)
    uint64_t timestamp;
    cJSON *data;
} mesh_message_t;
```

**Обновление функций создания сообщений:**
```c
bool mesh_protocol_create_telemetry(
    const char *node_id,
    const char *root_node_id,  // ⭐ НОВОЕ
    const char *node_type,
    cJSON *data,
    char *out_json,
    size_t max_len
);

bool mesh_protocol_create_heartbeat(
    const char *node_id,
    const char *root_node_id,  // ⭐ НОВОЕ
    const char *node_type,
    uint32_t uptime,
    uint32_t heap_free,
    char *out_json,
    size_t max_len
);

// ... аналогично для всех типов сообщений
```

**Пример JSON с zone_id:**
```json
{
  "type": "telemetry",
  "node_id": "ph_001",
  "root_node_id": "root_001",
  "zone_id": "1",
  "node_type": "ph_ec",
  "timestamp": 1699267200,
  "data": {
    "ph": 6.5,
    "ec": 1.8,
    "temp": 22.5
  }
}
```

### 3. Root Node - Идентификация зоны

**`root_node/main/root_config.h`:**
```c
#ifndef ROOT_CONFIG_H
#define ROOT_CONFIG_H

// ⭐ КОНФИГУРАЦИЯ ЗОНЫ ROOT NODE
typedef struct {
    char root_node_id[32];      // "root_001"
    char zone_name[64];         // "Зона 1 - NFT"
    char mesh_network_id[32];   // "HYDRO1_ZONE1"
    char mqtt_topic_prefix[64]; // "hydro/zone1/"
} root_zone_config_t;

// Инициализация конфигурации зоны
esp_err_t root_config_init(root_zone_config_t *config);

// Получить текущую конфигурацию
const root_zone_config_t* root_config_get(void);

#endif
```

**`root_node/main/root_config.c`:**
```c
#include "root_config.h"
#include "nvs_flash.h"
#include <string.h>

static root_zone_config_t g_root_config = {0};

esp_err_t root_config_init(root_zone_config_t *config) {
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("root_config", NVS_READONLY, &nvs);
    
    if (ret == ESP_OK) {
        // Читаем из NVS
        size_t len = sizeof(config->root_node_id);
        nvs_get_str(nvs, "root_id", config->root_node_id, &len);
        
        len = sizeof(config->zone_name);
        nvs_get_str(nvs, "zone_name", config->zone_name, &len);
        
        len = sizeof(config->mesh_network_id);
        nvs_get_str(nvs, "mesh_id", config->mesh_network_id, &len);
        
        len = sizeof(config->mqtt_topic_prefix);
        nvs_get_str(nvs, "mqtt_prefix", config->mqtt_topic_prefix, &len);
        
        nvs_close(nvs);
    } else {
        // Дефолтные значения
        strcpy(config->root_node_id, "root_001");
        strcpy(config->zone_name, "Zone 1");
        strcpy(config->mesh_network_id, "HYDRO1_ZONE1");
        strcpy(config->mqtt_topic_prefix, "hydro/zone1/");
    }
    
    memcpy(&g_root_config, config, sizeof(root_zone_config_t));
    return ESP_OK;
}

const root_zone_config_t* root_config_get(void) {
    return &g_root_config;
}
```

### 4. Root Node - MQTT топики с зонированием

**Старые топики:**
```
hydro/telemetry/ph_001
hydro/command/ph_001
hydro/discovery
```

**Новые топики с зонами:**
```
hydro/zone1/telemetry/ph_001      # Zone 1
hydro/zone1/command/ph_001
hydro/zone1/discovery
hydro/zone1/status

hydro/zone2/telemetry/ph_002      # Zone 2
hydro/zone2/command/ph_002
hydro/zone2/discovery
hydro/zone2/status

hydro/zone3/telemetry/ph_003      # Zone 3
hydro/zone3/command/ph_003
hydro/zone3/discovery
hydro/zone3/status
```

**Обновление `root_node/components/mqtt_client_manager`:**
```c
// Получение topic prefix из конфигурации
const root_zone_config_t* zone_config = root_config_get();
char topic[256];

// Telemetry topic
snprintf(topic, sizeof(topic), "%stelemetry/%s", 
         zone_config->mqtt_topic_prefix, node_id);

// Command topic (подписка)
snprintf(topic, sizeof(topic), "%scommand/%s", 
         zone_config->mqtt_topic_prefix, node_id);

// Discovery topic
snprintf(topic, sizeof(topic), "%sdiscovery", 
         zone_config->mqtt_topic_prefix);
```

### 5. Обычные узлы - знание своего Root Node

**В `node_config` компоненте:**
```c
typedef struct {
    char node_id[32];           // "ph_001"
    char root_node_id[32];      // ⭐ НОВОЕ: "root_001"
    char mesh_network_id[32];   // ⭐ НОВОЕ: "HYDRO1_ZONE1"
    // ... остальные параметры
} node_config_t;
```

**При отправке сообщений в mesh:**
```c
// Включаем root_node_id в telemetry
cJSON *root = cJSON_CreateObject();
cJSON_AddStringToObject(root, "type", "telemetry");
cJSON_AddStringToObject(root, "node_id", config->node_id);
cJSON_AddStringToObject(root, "root_node_id", config->root_node_id);  // ⭐
cJSON_AddStringToObject(root, "node_type", "ph_ec");
// ... data
```

---

## 🔌 BACKEND CHANGES

### 1. Models - обновление

**`app/Models/Zone.php` - добавление Root Node:**
```php
class Zone extends Model
{
    protected $fillable = [
        'name',
        'description',
        'root_node_id',         // ⭐ НОВОЕ
        'mesh_network_id',      // ⭐ НОВОЕ
        'mqtt_topic_prefix',    // ⭐ НОВОЕ
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
     * Root Node зоны
     */
    public function rootNode(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'root_node_id', 'node_id');
    }

    /**
     * Все узлы зоны (через root_node_id)
     */
    public function nodes(): HasMany
    {
        return $this->hasMany(Node::class, 'root_node_id', 'root_node_id');
    }

    /**
     * Получить MQTT topic prefix
     */
    public function getMqttTopicPrefix(): string
    {
        return $this->mqtt_topic_prefix ?? "hydro/zone{$this->id}/";
    }
}
```

**`app/Models/Node.php` - добавление Root Node:**
```php
class Node extends Model
{
    protected $fillable = [
        'node_id',
        'node_type',
        'root_node_id',    // ⭐ НОВОЕ
        'zone',            // Deprecated
        'mac_address',
        'online',
        'last_seen_at',
        'config',
        'metadata',
    ];

    /**
     * Root Node (для обычных узлов)
     */
    public function rootNode(): BelongsTo
    {
        return $this->belongsTo(Node::class, 'root_node_id', 'node_id');
    }

    /**
     * Дочерние узлы (для Root Node)
     */
    public function childNodes(): HasMany
    {
        return $this->hasMany(Node::class, 'root_node_id', 'node_id');
    }

    /**
     * Зона узла (через root_node_id)
     */
    public function zone(): HasOne
    {
        return $this->hasOne(Zone::class, 'root_node_id', 'root_node_id');
    }

    /**
     * Проверка: это Root Node?
     */
    public function isRootNode(): bool
    {
        return $this->node_type === 'root';
    }

    /**
     * Получить все узлы своей mesh сети
     */
    public function getMeshNodes()
    {
        if ($this->isRootNode()) {
            return $this->childNodes;
        } else {
            return $this->rootNode->childNodes ?? collect();
        }
    }
}
```

### 2. MQTT Listener - маршрутизация по зонам

**`app/Services/MqttListenerService.php`:**
```php
class MqttListenerService
{
    /**
     * Подписка на топики всех зон
     */
    public function subscribeToAllZones()
    {
        $zones = Zone::where('is_active', true)->get();

        foreach ($zones as $zone) {
            $prefix = $zone->getMqttTopicPrefix();

            // Подписываемся на топики зоны
            $this->mqtt->subscribe("{$prefix}telemetry/#", function($topic, $message) use ($zone) {
                $this->handleTelemetry($zone, $topic, $message);
            });

            $this->mqtt->subscribe("{$prefix}event/#", function($topic, $message) use ($zone) {
                $this->handleEvent($zone, $topic, $message);
            });

            $this->mqtt->subscribe("{$prefix}heartbeat/#", function($topic, $message) use ($zone) {
                $this->handleHeartbeat($zone, $topic, $message);
            });

            $this->mqtt->subscribe("{$prefix}discovery", function($topic, $message) use ($zone) {
                $this->handleDiscovery($zone, $topic, $message);
            });

            Log::info("Subscribed to zone MQTT topics", [
                'zone_id' => $zone->id,
                'zone_name' => $zone->name,
                'topic_prefix' => $prefix,
            ]);
        }
    }

    /**
     * Обработка telemetry с учетом зоны
     */
    protected function handleTelemetry(Zone $zone, string $topic, string $message)
    {
        $data = json_decode($message, true);

        if (!$data) {
            Log::warning('Invalid telemetry JSON', ['topic' => $topic]);
            return;
        }

        $nodeId = $data['node_id'] ?? null;
        $rootNodeId = $data['root_node_id'] ?? null;

        // Проверка: node принадлежит этой зоне
        if ($rootNodeId !== $zone->root_node_id) {
            Log::warning('Telemetry from wrong zone', [
                'expected_root' => $zone->root_node_id,
                'received_root' => $rootNodeId,
                'node_id' => $nodeId,
            ]);
            return;
        }

        // Сохраняем telemetry
        Telemetry::create([
            'node_id' => $nodeId,
            'data' => $data['data'] ?? [],
            'received_at' => now(),
        ]);

        // Обновляем статус узла
        $node = Node::where('node_id', $nodeId)->first();
        if ($node) {
            $node->update([
                'online' => true,
                'last_seen_at' => now(),
                'root_node_id' => $rootNodeId,  // Обновляем привязку
            ]);
        }
    }
}
```

### 3. API Endpoints - зонирование

**Routes для зон:**
```php
// routes/api.php

Route::prefix('zones')->group(function () {
    Route::get('/', [ZoneController::class, 'index']);
    Route::post('/', [ZoneController::class, 'store']);
    Route::get('/{id}', [ZoneController::class, 'show']);
    Route::put('/{id}', [ZoneController::class, 'update']);
    Route::delete('/{id}', [ZoneController::class, 'destroy']);
    
    // Узлы зоны
    Route::get('/{id}/nodes', [ZoneController::class, 'getNodes']);
    
    // Root Node зоны
    Route::get('/{id}/root', [ZoneController::class, 'getRootNode']);
    
    // Статус зоны
    Route::get('/{id}/status', [ZoneController::class, 'getStatus']);
    
    // Циклы зоны
    Route::get('/{id}/cycles', [ZoneController::class, 'getCycles']);
});

// Команды для зон
Route::post('/zones/{id}/commands', [ZoneController::class, 'sendCommand']);
```

**`ZoneController.php`:**
```php
class ZoneController extends Controller
{
    /**
     * Получить все узлы зоны
     */
    public function getNodes($id)
    {
        $zone = Zone::with(['rootNode', 'nodes'])->findOrFail($id);

        return response()->json([
            'zone' => $zone,
            'root_node' => $zone->rootNode,
            'nodes' => $zone->nodes,
        ]);
    }

    /**
     * Отправить команду в зону
     */
    public function sendCommand(Request $request, $id)
    {
        $zone = Zone::findOrFail($id);

        $validated = $request->validate([
            'node_id' => 'required|string',
            'command' => 'required|string',
            'params' => 'nullable|array',
        ]);

        // Проверка: узел принадлежит этой зоне
        $node = Node::where('node_id', $validated['node_id'])
                    ->where('root_node_id', $zone->root_node_id)
                    ->firstOrFail();

        // Отправляем команду через MQTT
        $topic = "{$zone->getMqttTopicPrefix()}command/{$node->node_id}";
        $payload = json_encode([
            'type' => 'command',
            'node_id' => $node->node_id,
            'root_node_id' => $zone->root_node_id,
            'command' => $validated['command'],
            'params' => $validated['params'] ?? [],
            'timestamp' => now()->timestamp,
        ]);

        Mqtt::publish($topic, $payload);

        // Логируем команду
        Command::create([
            'node_id' => $node->node_id,
            'command' => $validated['command'],
            'params' => $validated['params'] ?? [],
            'sent_at' => now(),
        ]);

        return response()->json(['message' => 'Command sent']);
    }
}
```

---

## 🎨 FRONTEND CHANGES

### 1. Stores - Zones

**`frontend/src/stores/zones.ts`:**
```typescript
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { zonesApi } from '@/api/zones'

export const useZonesStore = defineStore('zones', () => {
  const zones = ref<Zone[]>([])
  const loading = ref(false)

  const activeZones = computed(() => 
    zones.value.filter(z => z.is_active)
  )

  const availableZones = computed(() =>
    zones.value.filter(z => z.is_available && !z.current_cycle_id)
  )

  async function fetchZones() {
    loading.value = true
    try {
      const response = await zonesApi.getAll()
      zones.value = response.data
    } catch (error) {
      console.error('Failed to fetch zones:', error)
    } finally {
      loading.value = false
    }
  }

  async function fetchZoneNodes(zoneId: number) {
    try {
      const response = await zonesApi.getNodes(zoneId)
      return response.data
    } catch (error) {
      console.error('Failed to fetch zone nodes:', error)
      throw error
    }
  }

  return {
    zones,
    loading,
    activeZones,
    availableZones,
    fetchZones,
    fetchZoneNodes,
  }
})

interface Zone {
  id: number
  name: string
  description: string
  root_node_id: string
  mesh_network_id: string
  mqtt_topic_prefix: string
  zone_type: 'nft' | 'dwc' | 'ebb_flow' | 'drip' | 'other'
  reservoir_volume_liters: number
  growing_area_m2: number
  plant_capacity: number
  assigned_nodes: {
    ph_node?: string
    climate_node?: string
    relay_node?: string
    water_node?: string
    display_node?: string
  }
  is_active: boolean
  is_available: boolean
  current_cycle_id: number | null
  root_node?: Node
  nodes?: Node[]
}
```

### 2. Components - Zone Dashboard

**`frontend/src/components/zones/ZoneDashboard.vue`:**
```vue
<template>
  <v-container fluid>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h4 mb-4">Зоны выращивания</h1>
      </v-col>
    </v-row>

    <v-row>
      <v-col
        v-for="zone in activeZones"
        :key="zone.id"
        cols="12"
        md="6"
        lg="4"
      >
        <ZoneCard :zone="zone" />
      </v-col>

      <!-- Добавить зону -->
      <v-col cols="12" md="6" lg="4">
        <v-card
          class="d-flex align-center justify-center"
          height="300"
          @click="showAddZoneDialog = true"
          style="cursor: pointer; border: 2px dashed #ccc"
        >
          <v-card-text class="text-center">
            <v-icon size="64" color="primary">mdi-plus-circle-outline</v-icon>
            <div class="text-h6 mt-4">Добавить зону</div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Dialog добавления зоны -->
    <AddZoneDialog v-model="showAddZoneDialog" @created="onZoneCreated" />
  </v-container>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useZonesStore } from '@/stores/zones'
import ZoneCard from './ZoneCard.vue'
import AddZoneDialog from './AddZoneDialog.vue'

const zonesStore = useZonesStore()
const showAddZoneDialog = ref(false)

const { activeZones } = zonesStore

onMounted(() => {
  zonesStore.fetchZones()
})

function onZoneCreated() {
  zonesStore.fetchZones()
}
</script>
```

**`frontend/src/components/zones/ZoneCard.vue`:**
```vue
<template>
  <v-card elevation="2" class="zone-card">
    <v-card-title class="d-flex align-center">
      <v-icon :color="zoneStatusColor" class="mr-2">
        {{ zoneIcon }}
      </v-icon>
      {{ zone.name }}
    </v-card-title>

    <v-card-subtitle>
      Root: {{ zone.root_node_id }} | Mesh: {{ zone.mesh_network_id }}
    </v-card-subtitle>

    <v-card-text>
      <!-- Характеристики зоны -->
      <div class="zone-specs mb-4">
        <v-chip size="small" class="mr-2">
          <v-icon start>mdi-water</v-icon>
          {{ zone.reservoir_volume_liters }}л
        </v-chip>
        <v-chip size="small" class="mr-2">
          <v-icon start>mdi-ruler-square</v-icon>
          {{ zone.growing_area_m2 }}м²
        </v-chip>
        <v-chip size="small">
          <v-icon start>mdi-sprout</v-icon>
          {{ zone.plant_capacity }} раст
        </v-chip>
      </div>

      <!-- Текущий цикл -->
      <div v-if="zone.current_cycle" class="current-cycle mb-4">
        <div class="text-subtitle-2 mb-2">Активный цикл:</div>
        <v-chip color="success" prepend-icon="mdi-leaf">
          {{ zone.current_cycle.name }}
        </v-chip>
        <div class="text-caption mt-2">
          {{ zone.current_cycle.current_stage_name }} 
          ({{ zone.current_cycle.days_in_stage }}д)
        </div>
        <v-progress-linear
          :model-value="zone.current_cycle.progress"
          color="success"
          class="mt-2"
        />
      </div>

      <div v-else class="text-caption text-medium-emphasis">
        💤 Свободна
      </div>

      <!-- Узлы зоны -->
      <v-divider class="my-3" />
      <div class="text-subtitle-2 mb-2">Узлы:</div>
      <div class="zone-nodes">
        <v-tooltip
          v-for="(nodeId, role) in zone.assigned_nodes"
          :key="role"
          :text="`${role}: ${nodeId}`"
          location="top"
        >
          <template #activator="{ props }">
            <v-chip
              v-bind="props"
              size="small"
              :color="getNodeStatus(nodeId) ? 'success' : 'error'"
              class="mr-1 mb-1"
            >
              <v-icon start>{{ getNodeIcon(role) }}</v-icon>
              {{ nodeId }}
            </v-chip>
          </template>
        </v-tooltip>
      </div>
    </v-card-text>

    <v-card-actions>
      <v-btn
        v-if="!zone.current_cycle"
        color="primary"
        variant="text"
        prepend-icon="mdi-play"
        @click="startCycle"
      >
        Начать цикл
      </v-btn>
      <v-btn
        v-else
        color="info"
        variant="text"
        prepend-icon="mdi-information"
        @click="viewCycle"
      >
        Подробнее
      </v-btn>
      <v-spacer />
      <v-btn icon @click="showZoneSettings">
        <v-icon>mdi-cog</v-icon>
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useRouter } from 'vue-router'
import type { Zone } from '@/types/zones'

const props = defineProps<{
  zone: Zone
}>()

const router = useRouter()

const zoneStatusColor = computed(() => {
  if (!props.zone.is_active) return 'grey'
  if (props.zone.current_cycle) return 'success'
  return 'info'
})

const zoneIcon = computed(() => {
  const icons: Record<string, string> = {
    nft: 'mdi-water-pump',
    dwc: 'mdi-water',
    ebb_flow: 'mdi-waves',
    drip: 'mdi-water-opacity',
    other: 'mdi-sprout',
  }
  return icons[props.zone.zone_type] || 'mdi-sprout'
})

function getNodeIcon(role: string): string {
  const icons: Record<string, string> = {
    ph_node: 'mdi-flask',
    climate_node: 'mdi-thermometer',
    relay_node: 'mdi-electric-switch',
    water_node: 'mdi-water-pump',
    display_node: 'mdi-monitor',
  }
  return icons[role] || 'mdi-circle'
}

function getNodeStatus(nodeId: string): boolean {
  // TODO: Получить статус узла из store
  return true
}

function startCycle() {
  router.push({
    name: 'growth-cycle-create',
    query: { zone_id: props.zone.id },
  })
}

function viewCycle() {
  router.push({
    name: 'growth-cycle-detail',
    params: { id: props.zone.current_cycle_id },
  })
}

function showZoneSettings() {
  router.push({
    name: 'zone-settings',
    params: { id: props.zone.id },
  })
}
</script>

<style scoped>
.zone-card {
  height: 100%;
}

.zone-specs {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.zone-nodes {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}
</style>
```

---

## 📦 SEEDERS

### Database Seeders с зонами

**`server/backend/database/seeders/ZoneSeeder.php`:**
```php
<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use App\Models\Zone;
use App\Models\Node;

class ZoneSeeder extends Seeder
{
    public function run(): void
    {
        // Убедимся что Root Nodes существуют
        $rootNodes = [
            ['node_id' => 'root_001', 'mesh_id' => 'HYDRO1_ZONE1'],
            ['node_id' => 'root_002', 'mesh_id' => 'HYDRO1_ZONE2'],
            ['node_id' => 'root_003', 'mesh_id' => 'HYDRO1_ZONE3'],
        ];

        foreach ($rootNodes as $rootData) {
            Node::updateOrCreate(
                ['node_id' => $rootData['node_id']],
                [
                    'node_type' => 'root',
                    'root_node_id' => $rootData['node_id'], // Root сам себя
                    'online' => true,
                    'last_seen_at' => now(),
                    'metadata' => [
                        'mesh_network_id' => $rootData['mesh_id'],
                    ],
                ]
            );
        }

        // Создаем зоны
        $zones = [
            [
                'name' => 'Зона 1 - NFT',
                'description' => 'NFT система, 100л бак, 2м²',
                'root_node_id' => 'root_001',
                'mesh_network_id' => 'HYDRO1_ZONE1',
                'mqtt_topic_prefix' => 'hydro/zone1/',
                'zone_type' => 'nft',
                'reservoir_volume_liters' => 100.0,
                'growing_area_m2' => 2.0,
                'plant_capacity' => 20,
                'assigned_nodes' => [
                    'ph_node' => 'ph_001',
                    'climate_node' => 'climate_001',
                    'relay_node' => 'relay_001',
                    'water_node' => 'water_001',
                ],
                'is_active' => true,
                'is_available' => true,
            ],
            [
                'name' => 'Зона 2 - DWC',
                'description' => 'DWC система, 50л бак, 1м²',
                'root_node_id' => 'root_002',
                'mesh_network_id' => 'HYDRO1_ZONE2',
                'mqtt_topic_prefix' => 'hydro/zone2/',
                'zone_type' => 'dwc',
                'reservoir_volume_liters' => 50.0,
                'growing_area_m2' => 1.0,
                'plant_capacity' => 10,
                'assigned_nodes' => [
                    'ph_node' => 'ph_002',
                    'climate_node' => 'climate_002',
                    'relay_node' => 'relay_002',
                ],
                'is_active' => true,
                'is_available' => true,
            ],
            [
                'name' => 'Теплица А - Капельный полив',
                'description' => 'Капельный полив, 200л бак, 5м²',
                'root_node_id' => 'root_003',
                'mesh_network_id' => 'HYDRO1_ZONE3',
                'mqtt_topic_prefix' => 'hydro/zone3/',
                'zone_type' => 'drip',
                'reservoir_volume_liters' => 200.0,
                'growing_area_m2' => 5.0,
                'plant_capacity' => 50,
                'assigned_nodes' => [
                    'ph_node' => 'ph_003',
                    'climate_node' => 'climate_003',
                    'relay_node' => 'relay_003',
                    'water_node' => 'water_003',
                ],
                'is_active' => true,
                'is_available' => true,
            ],
        ];

        foreach ($zones as $zoneData) {
            Zone::create($zoneData);

            // Создаем узлы для зоны
            foreach ($zoneData['assigned_nodes'] as $role => $nodeId) {
                $nodeType = explode('_', $role)[0]; // ph_node -> ph

                Node::updateOrCreate(
                    ['node_id' => $nodeId],
                    [
                        'node_type' => $nodeType,
                        'root_node_id' => $zoneData['root_node_id'],
                        'zone' => $zoneData['name'], // Deprecated field
                        'online' => rand(0, 1) === 1,
                        'last_seen_at' => now()->subMinutes(rand(1, 30)),
                    ]
                );
            }
        }
    }
}
```

---

## 🔥 КРИТИЧЕСКИЕ ТОЧКИ ВНИМАНИЯ

### 1. ⚠️ Конфликты узлов между зонами
- **Проблема:** Один физический узел может случайно подключиться к двум Root Nodes
- **Решение:** 
  - При регистрации узла проверять `root_node_id`
  - Backend валидация: один узел = одна зона
  - Frontend предупреждения при конфликтах

### 2. ⚠️ MQTT Topic Collisions
- **Проблема:** Без topic prefix сообщения смешиваются
- **Решение:**
  - **ОБЯЗАТЕЛЬНО** использовать `zone` в топиках
  - Listener подписывается на топики всех зон отдельно
  - Валидация `root_node_id` при получении сообщений

### 3. ⚠️ Mesh Network ID Conflicts
- **Проблема:** Два Root с одинаковым `MESH_NETWORK_ID` создадут одну сеть
- **Решение:**
  - **УНИКАЛЬНЫЙ** `MESH_NETWORK_ID` для каждого Root
  - Прописывать через NVS при первой настройке
  - Документация для пользователей

### 4. ⚠️ Миграция существующих данных
- **Проблема:** В текущей БД уже есть узлы без `root_node_id`
- **Решение:**
  - Миграция: установить `root_node_id` = 'root_001' для всех существующих узлов
  - Создать зону для текущего Root Node
  - Обновить конфигурацию ESP32

---

## 📋 ПЛАН ВНЕДРЕНИЯ (Приоритизированный)

### Этап 1: Database & Backend (2-3 дня) - КРИТИЧЕСКИЙ
- [x] Миграция: добавить `root_node_id` в `zones`
- [ ] Миграция: добавить `root_node_id` в `nodes`
- [ ] Миграция: создать `root_nodes` (опционально)
- [ ] Миграция существующих данных (установить root_node_id)
- [ ] Обновить Model Zone
- [ ] Обновить Model Node
- [ ] ZoneSeeder с 3 зонами

### Этап 2: ESP32 Firmware - Common Components (2-3 дня) - КРИТИЧЕСКИЙ
- [ ] Обновить `mesh_config.h` (добавить zone support)
- [ ] Обновить `mesh_protocol.h` (добавить root_node_id)
- [ ] Обновить `mesh_protocol.c` (включать root_node_id в сообщения)
- [ ] Обновить `node_config` (сохранять root_node_id в NVS)
- [ ] Создать утилиты для настройки зоны через Serial

### Этап 3: Root Node Firmware (2 дня)
- [ ] Создать `root_config` компонент
- [ ] Обновить MQTT Client Manager (zone topics)
- [ ] Обновить Data Router (проверка zone)
- [ ] Тестирование нескольких Root Nodes

### Этап 4: Nodes Firmware (1 день)
- [ ] Обновить climate_node
- [ ] Обновить ph_ec_node
- [ ] Обновить relay_node
- [ ] Обновить water_node
- [ ] Обновить display_node

### Этап 5: Backend MQTT & API (2 дня)
- [ ] Обновить MqttListenerService (zone topics)
- [ ] Обновить ZoneController (getNodes, sendCommand)
- [ ] Обновить NodeController (zone filtering)
- [ ] API тесты с зонами

### Этап 6: Frontend (3 дня)
- [ ] zones.ts store
- [ ] ZoneDashboard компонент
- [ ] ZoneCard компонент
- [ ] AddZoneDialog
- [ ] ZoneSettingsPage
- [ ] Интеграция с Growth Planner

### Этап 7: Testing & Documentation (2 дня)
- [ ] Тестирование 3 Root Nodes одновременно
- [ ] Проверка изоляции зон
- [ ] Документация настройки
- [ ] Видео-инструкция

---

**ИТОГО: 14-16 рабочих дней (3-4 недели)**

---

## 🚀 БЫСТРЫЙ СТАРТ (для разработчиков)

### 1. Настройка первой зоны (текущая система)
```bash
# 1. Запустить миграции
cd server/backend
php artisan migrate

# 2. Запустить seeder
php artisan db:seed --class=ZoneSeeder

# 3. Проверить что зона создана
php artisan tinker
>>> Zone::with('rootNode')->first()
```

### 2. Прошивка Root Node для Zone 1
```bash
cd root_node

# Сконфигурировать через menuconfig
idf.py menuconfig
# -> Hydro Mesh Config
# -> Set Root Node ID: root_001
# -> Set Mesh Network ID: HYDRO1_ZONE1
# -> Set Zone Name: Zone 1

# Собрать и прошить
idf.py build flash monitor
```

### 3. Прошивка узлов для Zone 1
```bash
cd node_ph_ec

# Сконфигурировать
idf.py menuconfig
# -> Hydro Mesh Config
# -> Set Node ID: ph_001
# -> Set Root Node ID: root_001
# -> Set Mesh Network ID: HYDRO1_ZONE1

# Собрать и прошить
idf.py build flash monitor
```

### 4. Добавление второй зоны
```bash
# 1. Создать зону через API или админку
curl -X POST http://localhost:8000/api/zones \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Зона 2 - DWC",
    "root_node_id": "root_002",
    "mesh_network_id": "HYDRO1_ZONE2",
    "mqtt_topic_prefix": "hydro/zone2/",
    "zone_type": "dwc",
    "reservoir_volume_liters": 50,
    "growing_area_m2": 1,
    "plant_capacity": 10,
    "assigned_nodes": {
      "ph_node": "ph_002",
      "climate_node": "climate_002",
      "relay_node": "relay_002"
    }
  }'

# 2. Прошить Root Node #2 с MESH_NETWORK_ID = HYDRO1_ZONE2

# 3. Прошить узлы Zone 2 с root_node_id = root_002
```

---

## ✅ CHECKLIST

### Backend
- [ ] Миграции выполнены
- [ ] Models обновлены
- [ ] Seeders с зонами работают
- [ ] MQTT Listener поддерживает зоны
- [ ] API endpoints для зон готовы

### ESP32 Firmware
- [ ] mesh_config поддерживает zone_id
- [ ] mesh_protocol включает root_node_id
- [ ] Root Node знает свою зону
- [ ] Nodes знают своего Root
- [ ] MQTT топики с zone prefix

### Frontend
- [ ] zones store создан
- [ ] ZoneDashboard работает
- [ ] ZoneCard отображает статус
- [ ] Можно создать новую зону
- [ ] Growth Planner интегрирован

### Testing
- [ ] 2+ Root Nodes работают одновременно
- [ ] Узлы изолированы по зонам
- [ ] MQTT сообщения не смешиваются
- [ ] Циклы роста привязаны к зонам

---

**ГОТОВ НАЧАТЬ РЕАЛИЗАЦИЮ! 🚀**


