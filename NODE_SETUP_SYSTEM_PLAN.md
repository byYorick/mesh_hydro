# План: Система добавления узлов (Node Setup System)

## Обзор
Упрощенная система добавления узлов БЕЗ отдельной mesh сети:
1. **ROOT нода настраивается первой** - получает WiFi роутер + MQTT настройки
2. **ROOT создает mesh сеть** для своей зоны (например, HYDRO1_ZONE1)
3. **Новые ноды подключаются** к mesh сети настроенного ROOT
4. **Отправляют discovery + heartbeat** (БЕЗ полной телеметрии)
5. **Настраиваются через мастер** на фронтенде
6. **Перезагружаются** и начинают работать в обычном режиме

---

## 1. ESP32 Firmware - Поддержка режима "new"

### 1.1 Модификация `common/node_config`

**Файл: `common/node_config/node_config.h`**
```c
bool node_config_is_configured(void);
const char* node_config_get_mesh_mode(void); // "new" или "configured"
esp_err_t node_config_save_and_reboot(void *config, size_t size, const char *namespace);
```

**Файл: `common/node_config/node_config.c`**
```c
bool node_config_is_configured(void) {
    // Проверка: есть ли в NVS node_id И mesh_id
    char node_id[32] = {0};
    char mesh_id[32] = {0};
    
    nvs_handle_t handle;
    esp_err_t err = nvs_open("hydro_ns", NVS_READONLY, &handle);
    if (err != ESP_OK) return false;
    
    size_t len = sizeof(node_id);
    err = nvs_get_str(handle, "node_id", node_id, &len);
    bool has_node_id = (err == ESP_OK && strlen(node_id) > 0);
    
    len = sizeof(mesh_id);
    err = nvs_get_str(handle, "mesh_id", mesh_id, &len);
    bool has_mesh_id = (err == ESP_OK && strlen(mesh_id) > 0);
    
    nvs_close(handle);
    return has_node_id && has_mesh_id;
}

esp_err_t node_config_save_and_reboot(void *config, size_t size, const char *namespace) {
    esp_err_t err = node_config_save(config, size, namespace);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Config saved, rebooting in 3 seconds...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }
    return err;
}
```

### 1.2 ROOT Node - Режим "new"

**Файл: `root_node/main/app_main.c`**

**Логика:**
- При старте проверить `node_config_is_configured()`
- Если `false` (режим "new"):
  - Подключиться к WiFi роутеру (используя дефолтные credentials из mesh_config.h)
  - НЕ подключаться к MQTT
  - Создать mesh сеть с временным ID (например, "HYDRO_NEW")
  - Отправлять discovery через HTTP на сервер (напрямую, без MQTT)
  - Ждать команду `write_config` через HTTP или mesh
- Если `true` (режим "configured"):
  - Загрузить настройки из NVS
  - Подключиться к WiFi роутеру и MQTT
  - Создать mesh сеть с настроенным mesh_id

**Пример кода:**
```c
void app_main(void) {
    // ...NVS init...
    
    bool is_configured = node_config_is_configured();
    
    if (!is_configured) {
        ESP_LOGW(TAG, "ROOT NODE IN 'NEW' MODE - waiting for configuration");
        
        // Подключение к WiFi роутеру
        wifi_connect(MESH_ROUTER_SSID, MESH_ROUTER_PASSWORD);
        
        // Создать временную mesh сеть для discovery
        mesh_manager_config_t cfg = {
            .mode = MESH_MODE_ROOT,
            .mesh_id = "HYDRO_NEW",  // Временный ID
            .mesh_password = "setup_2025",
            .channel = 7,
            .router_ssid = MESH_ROUTER_SSID,
            .router_password = MESH_ROUTER_PASSWORD,
        };
        mesh_manager_init(&cfg);
        
        // Отправить discovery на сервер через HTTP
        send_root_discovery_http();
        
        // Ждать команду write_config
        wait_for_config();
        
    } else {
        ESP_LOGI(TAG, "ROOT NODE CONFIGURED - normal operation");
        
        // Загрузить конфигурацию
        load_root_config();
        
        // Подключиться к MQTT
        mqtt_init();
        
        // Создать mesh с настроенным ID
        mesh_manager_config_t cfg = {
            .mode = MESH_MODE_ROOT,
            .mesh_id = s_config.mesh_id,  // Из NVS
            .mesh_password = "hydro_mesh_2025",
            .channel = 7,
            .router_ssid = MESH_ROUTER_SSID,
            .router_password = MESH_ROUTER_PASSWORD,
        };
        mesh_manager_init(&cfg);
        
        // Обычная работа
        start_normal_operation();
    }
}
```

### 1.3 Обычные ноды - Режим "new"

**Файлы: `node_ph/main/app_main.c`, `node_ec/main/app_main.c`, и т.д.**

**Логика:**
- При старте проверить `node_config_is_configured()`
- Если `false` (режим "new"):
  - Подключиться к любой доступной mesh сети (scan + connect)
  - Отправлять только discovery (1 раз) и heartbeat (каждые 10 сек)
  - НЕ запускать сенсоры и исполнительные механизмы
  - Ждать команду `write_config`
- Если `true` (режим "configured"):
  - Загрузить mesh_id из NVS
  - Подключиться к настроенной mesh сети
  - Запустить сенсоры/актуаторы
  - Отправлять полную телеметрию

**Пример кода:**
```c
void app_main(void) {
    // ...NVS init...
    
    bool is_configured = node_config_is_configured();
    
    if (!is_configured) {
        ESP_LOGW(TAG, "NODE IN 'NEW' MODE - waiting for configuration");
        
        // Подключиться к любой mesh сети (scan)
        mesh_manager_config_t cfg = {
            .mode = MESH_MODE_NODE,
            .mesh_id = NULL,  // Auto-scan
            .mesh_password = "hydro_mesh_2025",  // Попробовать общий пароль
            .channel = 0,  // Auto
        };
        mesh_manager_init(&cfg);
        
        // Отправить discovery
        send_discovery_once();
        
        // Запустить heartbeat task
        xTaskCreate(heartbeat_task, "heartbeat", 4096, NULL, 5, NULL);
        
        // Ждать команду write_config
        wait_for_config();
        
    } else {
        ESP_LOGI(TAG, "NODE CONFIGURED - normal operation");
        
        // Загрузить конфигурацию
        load_node_config();
        
        // Подключиться к mesh
        mesh_manager_config_t cfg = {
            .mode = MESH_MODE_NODE,
            .mesh_id = s_config.mesh_id,  // Из NVS
            .mesh_password = "hydro_mesh_2025",
            .channel = 7,
        };
        mesh_manager_init(&cfg);
        
        // Запустить сенсоры/актуаторы
        start_sensors();
        start_actuators();
        
        // Отправлять телеметрию
        start_telemetry_task();
    }
}
```

### 1.4 Discovery и Heartbeat сообщения

**Файл: `common/mesh_protocol/mesh_protocol.h`**
```c
#define MSG_TYPE_DISCOVERY    "discovery"
#define MSG_TYPE_HEARTBEAT    "heartbeat"
```

**Discovery (1 раз при подключении):**
```json
{
  "type": "discovery",
  "node_type": "ph",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "chip_model": "ESP32-C3",
  "firmware_version": "1.0.0",
  "free_heap": 180000,
  "rssi": -45,
  "timestamp": 1234567890
}
```

**Heartbeat (каждые 10 секунд):**
```json
{
  "type": "heartbeat",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "uptime": 3600,
  "free_heap": 178000,
  "timestamp": 1234567890
}
```

### 1.5 Обработка команды `write_config`

**Все ноды (включая ROOT)**

```c
void handle_write_config_command(cJSON *params) {
    const char *node_id = cJSON_GetObjectItem(params, "node_id")->valuestring;
    const char *mesh_id = cJSON_GetObjectItem(params, "mesh_id")->valuestring;
    const char *zone = cJSON_GetObjectItem(params, "zone")->valuestring;
    
    // Для ROOT дополнительно:
    if (is_root_node) {
        const char *mqtt_host = cJSON_GetObjectItem(params, "mqtt_broker_host")->valuestring;
        int mqtt_port = cJSON_GetObjectItem(params, "mqtt_broker_port")->valueint;
        // Сохранить MQTT настройки
    }
    
    // Сохранить в структуру конфигурации
    strcpy(s_config.base.node_id, node_id);
    strcpy(s_config.base.zone, zone);
    node_config_set_mesh_network_id(mesh_id);
    
    // Сохранить в NVS
    node_config_save(&s_config, sizeof(s_config), "..._ns");
    
    // Отправить подтверждение
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "config_confirmation");
    cJSON_AddStringToObject(response, "mac_address", get_mac_address());
    cJSON_AddStringToObject(response, "node_id", node_id);
    cJSON_AddStringToObject(response, "status", "success");
    
    char *json_str = cJSON_PrintUnformatted(response);
    mesh_manager_send_to_root((uint8_t*)json_str, strlen(json_str));
    cJSON_Delete(response);
    free(json_str);
    
    // Перезагрузка через 3 секунды
    ESP_LOGI(TAG, "Configuration saved, rebooting...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
}
```

---

## 2. Backend - Временная таблица и API

### 2.1 Migration для таблицы `new_nodes`

**Файл: `server/backend/database/migrations/2025_01_XX_create_new_nodes_table.php`**
```php
<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::create('new_nodes', function (Blueprint $table) {
            $table->id();
            $table->string('mac_address', 17)->unique();
            $table->string('node_type');
            $table->boolean('is_root')->default(false);
            $table->jsonb('metadata');
            $table->timestamp('discovered_at');
            $table->timestamp('last_heartbeat_at')->nullable();
            $table->timestamps();
            
            $table->index('mac_address');
            $table->index('node_type');
            $table->index('is_root');
            $table->index('last_heartbeat_at');
        });
    }

    public function down(): void
    {
        Schema::dropIfExists('new_nodes');
    }
};
```

### 2.2 Модель `NewNode`

**Файл: `server/backend/app/Models/NewNode.php`**
```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use App\Services\MqttService;

class NewNode extends Model
{
    protected $fillable = [
        'mac_address', 
        'node_type', 
        'is_root',
        'metadata', 
        'discovered_at', 
        'last_heartbeat_at'
    ];
    
    protected $casts = [
        'metadata' => 'array',
        'is_root' => 'boolean',
        'discovered_at' => 'datetime',
        'last_heartbeat_at' => 'datetime',
    ];
    
    public function isOnline(): bool
    {
        if (!$this->last_heartbeat_at) {
            return false;
        }
        return $this->last_heartbeat_at->diffInSeconds(now()) < 30;
    }
    
    public function configure(array $config): void
    {
        // Отправить команду write_config через MQTT или HTTP
        if ($this->is_root) {
            // Для ROOT отправить через HTTP (если еще нет MQTT)
            $this->sendConfigViaHttp($config);
        } else {
            // Для обычных нод через MQTT (через ROOT)
            app(MqttService::class)->sendCommand(
                $this->mac_address, 
                'write_config', 
                $config
            );
        }
    }
    
    protected function sendConfigViaHttp(array $config): void
    {
        // TODO: Реализовать HTTP отправку для ROOT
        // В режиме "new" ROOT слушает на HTTP endpoint
    }
    
    public function moveToNodes(string $node_id): Node
    {
        // Создать запись в nodes
        $node = Node::create([
            'node_id' => $node_id,
            'node_type' => $this->node_type,
            'mac_address' => $this->mac_address,
            'online' => false,
            'config' => [],
            'metadata' => $this->metadata,
        ]);
        
        // Удалить из new_nodes
        $this->delete();
        
        return $node;
    }
}
```

### 2.3 Контроллер `NewNodeController`

**Файл: `server/backend/app/Http/Controllers/NewNodeController.php`**
```php
<?php

namespace App\Http\Controllers;

use App\Models\NewNode;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;

class NewNodeController extends Controller
{
    public function index(): JsonResponse
    {
        $newNodes = NewNode::orderBy('is_root', 'desc')
            ->orderBy('discovered_at', 'desc')
            ->get();
            
        // Добавить online статус
        $newNodes->each(function($node) {
            $node->is_online = $node->isOnline();
        });
        
        return response()->json($newNodes);
    }
    
    public function show(string $mac): JsonResponse
    {
        $newNode = NewNode::where('mac_address', $mac)->firstOrFail();
        $newNode->is_online = $newNode->isOnline();
        
        return response()->json($newNode);
    }
    
    public function configure(Request $request, string $mac): JsonResponse
    {
        $newNode = NewNode::where('mac_address', $mac)->firstOrFail();
        
        $validated = $request->validate([
            'node_id' => 'required|string|max:32|unique:nodes,node_id',
            'mesh_id' => 'required|string|max:32',
            'zone' => 'required|string|max:100',
            // Для ROOT дополнительно:
            'mqtt_broker_host' => 'required_if:is_root,true|string',
            'mqtt_broker_port' => 'required_if:is_root,true|integer',
        ]);
        
        // Отправить конфигурацию
        $newNode->configure($validated);
        
        return response()->json([
            'success' => true,
            'message' => 'Configuration sent to node',
        ]);
    }
    
    public function delete(string $mac): JsonResponse
    {
        $newNode = NewNode::where('mac_address', $mac)->firstOrFail();
        $newNode->delete();
        
        return response()->json([
            'success' => true,
            'message' => 'New node deleted',
        ]);
    }
}
```

### 2.4 MQTT обработка

**Файл: `server/backend/app/Services/MqttService.php`**

Добавить в метод `handleMessage()`:

```php
// Discovery от новой ноды
if ($data['type'] === 'discovery') {
    $isRoot = ($data['node_type'] === 'root');
    
    $newNode = NewNode::updateOrCreate(
        ['mac_address' => $data['mac_address']],
        [
            'node_type' => $data['node_type'],
            'is_root' => $isRoot,
            'metadata' => $data,
            'discovered_at' => now(),
            'last_heartbeat_at' => now(),
        ]
    );
    
    // Отправить событие через websocket
    broadcast(new NewNodeDiscovered($newNode));
}

// Heartbeat от новой ноды
if ($data['type'] === 'heartbeat') {
    NewNode::where('mac_address', $data['mac_address'])
        ->update(['last_heartbeat_at' => now()]);
}

// Подтверждение конфигурации
if ($data['type'] === 'config_confirmation' && $data['status'] === 'success') {
    $newNode = NewNode::where('mac_address', $data['mac_address'])->first();
    if ($newNode) {
        // Переместить в nodes
        $node = $newNode->moveToNodes($data['node_id']);
        broadcast(new NodeConfigured($node));
    }
}
```

### 2.5 API Routes

**Файл: `server/backend/routes/api.php`**

Добавить:

```php
// Специальный endpoint для ROOT в режиме "new" (без MQTT)
Route::post('/discovery/root', function(Request $request) {
    $validated = $request->validate([
        'mac_address' => 'required|string',
        'node_type' => 'required|string',
        'chip_model' => 'required|string',
        'firmware_version' => 'required|string',
    ]);
    
    $newNode = NewNode::updateOrCreate(
        ['mac_address' => $validated['mac_address']],
        [
            'node_type' => 'root',
            'is_root' => true,
            'metadata' => $validated,
            'discovered_at' => now(),
            'last_heartbeat_at' => now(),
        ]
    );
    
    broadcast(new NewNodeDiscovered($newNode));
    
    return response()->json(['success' => true]);
});

// API для new nodes
Route::prefix('new-nodes')->group(function () {
    Route::get('/', [NewNodeController::class, 'index']);
    Route::get('/{mac}', [NewNodeController::class, 'show']);
    Route::post('/{mac}/configure', [NewNodeController::class, 'configure']);
    Route::delete('/{mac}', [NewNodeController::class, 'delete']);
});
```

---

## 3. Frontend - Мастер настройки

### 3.1 Store для new nodes

**Файл: `server/frontend/src/stores/newNodes.ts`**
```typescript
import { defineStore } from 'pinia'
import api from '@/services/api'

export const useNewNodesStore = defineStore('newNodes', {
  state: () => ({
    newNodes: [] as any[],
    loading: false,
  }),
  
  actions: {
    async fetchNewNodes() {
      this.loading = true
      try {
        const response = await api.get('/new-nodes')
        this.newNodes = response.data
      } finally {
        this.loading = false
      }
    },
    
    async configureNode(mac: string, config: any) {
      return await api.post(`/new-nodes/${mac}/configure`, config)
    },
    
    async deleteNewNode(mac: string) {
      await api.delete(`/new-nodes/${mac}`)
      await this.fetchNewNodes()
    },
  },
  
  getters: {
    rootNodes: (state) => state.newNodes.filter(n => n.is_root),
    regularNodes: (state) => state.newNodes.filter(n => !n.is_root),
    onlineNodes: (state) => state.newNodes.filter(n => n.is_online),
    hasConfiguredRoot: (state) => {
      // Проверить, есть ли уже настроенная ROOT нода в системе
      // TODO: реализовать проверку через nodes store
      return false
    }
  }
})
```

### 3.2 Страница Setup Wizard

**Файл: `server/frontend/src/views/SetupWizard.vue`**

**Структура:**
```vue
<template>
  <v-container>
    <v-card>
      <v-card-title>Мастер настройки узлов</v-card-title>
      
      <!-- Проверка наличия ROOT -->
      <v-alert v-if="!hasConfiguredRoot && rootNodes.length > 0" type="warning">
        Обнаружена новая ROOT нода. Сначала настройте ROOT.
      </v-alert>
      
      <!-- Список ROOT нодов для настройки -->
      <RootSetupDialog 
        v-if="!hasConfiguredRoot"
        :root-nodes="rootNodes"
        @configured="onRootConfigured"
      />
      
      <!-- Список обычных нодов -->
      <NewNodesList 
        v-else
        :nodes="regularNodes"
        @configure="openNodeWizard"
      />
      
      <!-- Мастер настройки ноды -->
      <NodeSetupWizard
        v-model="showWizard"
        :node="selectedNode"
        @configured="onNodeConfigured"
      />
    </v-card>
  </v-container>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useNewNodesStore } from '@/stores/newNodes'
import RootSetupDialog from '@/components/setup/RootSetupDialog.vue'
import NewNodesList from '@/components/setup/NewNodesList.vue'
import NodeSetupWizard from '@/components/setup/NodeSetupWizard.vue'

const newNodesStore = useNewNodesStore()

const showWizard = ref(false)
const selectedNode = ref(null)

const rootNodes = computed(() => newNodesStore.rootNodes)
const regularNodes = computed(() => newNodesStore.regularNodes)
const hasConfiguredRoot = computed(() => newNodesStore.hasConfiguredRoot)

onMounted(() => {
  newNodesStore.fetchNewNodes()
})

function openNodeWizard(node: any) {
  selectedNode.value = node
  showWizard.value = true
}

function onRootConfigured() {
  newNodesStore.fetchNewNodes()
}

function onNodeConfigured() {
  newNodesStore.fetchNewNodes()
  showWizard.value = false
}
</script>
```

### 3.3 Компоненты

**Файл: `server/frontend/src/components/setup/RootSetupDialog.vue`**
```vue
<template>
  <v-dialog v-model="dialog" max-width="600">
    <v-card>
      <v-card-title>Настройка ROOT ноды</v-card-title>
      
      <v-card-text>
        <v-form ref="form">
          <v-select
            v-model="selectedRoot"
            :items="rootNodes"
            item-title="mac_address"
            item-value="mac_address"
            label="Выберите ROOT ноду"
            return-object
          />
          
          <v-text-field
            v-model="config.node_id"
            label="Node ID"
            placeholder="root_001"
            :rules="[rules.required]"
          />
          
          <v-text-field
            v-model="config.zone"
            label="Зона"
            placeholder="Zone 1"
            :rules="[rules.required]"
          />
          
          <v-text-field
            v-model="config.mqtt_broker_host"
            label="MQTT Broker Host"
            placeholder="192.168.1.100"
            :rules="[rules.required]"
          />
          
          <v-text-field
            v-model="config.mqtt_broker_port"
            label="MQTT Broker Port"
            type="number"
            placeholder="1883"
            :rules="[rules.required]"
          />
        </v-form>
      </v-card-text>
      
      <v-card-actions>
        <v-spacer />
        <v-btn @click="dialog = false">Отмена</v-btn>
        <v-btn color="primary" @click="configure">Настроить</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
// Реализация компонента
</script>
```

**Файл: `server/frontend/src/components/setup/NodeSetupWizard.vue`**
```vue
<template>
  <v-dialog v-model="dialog" max-width="800" persistent>
    <v-card>
      <v-card-title>Настройка узла</v-card-title>
      
      <v-stepper v-model="step">
        <v-stepper-header>
          <v-stepper-item value="1">Выбор</v-stepper-item>
          <v-stepper-item value="2">Параметры</v-stepper-item>
          <v-stepper-item value="3">Подтверждение</v-stepper-item>
        </v-stepper-header>
        
        <v-stepper-window>
          <!-- Шаги мастера -->
        </v-stepper-window>
      </v-stepper>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
// Реализация stepper
</script>
```

**Файл: `server/frontend/src/components/setup/NewNodeCard.vue`**
```vue
<template>
  <v-card>
    <v-card-title>
      <v-badge v-if="node.is_root" content="ROOT" color="primary">
        {{ node.node_type }}
      </v-badge>
      <span v-else>{{ node.node_type }}</span>
      
      <v-spacer />
      
      <v-chip :color="node.is_online ? 'success' : 'error'" size="small">
        {{ node.is_online ? 'Online' : 'Offline' }}
      </v-chip>
    </v-card-title>
    
    <v-card-text>
      <div>MAC: {{ node.mac_address }}</div>
      <div>Обнаружена: {{ formatDate(node.discovered_at) }}</div>
    </v-card-text>
    
    <v-card-actions>
      <v-btn color="primary" @click="$emit('configure', node)">
        Настроить
      </v-btn>
      <v-btn color="error" @click="$emit('delete', node)">
        Удалить
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup lang="ts">
// Реализация карточки
</script>
```

### 3.4 Роутинг

**Файл: `server/frontend/src/router/index.ts`**

Добавить:
```typescript
{
  path: '/setup',
  name: 'Setup',
  component: () => import('@/views/SetupWizard.vue'),
  meta: { 
    title: 'Мастер настройки узлов',
    icon: 'mdi-cog'
  }
}
```

### 3.5 Навигация

**Файл: `App.vue` или главное меню**

Добавить пункт меню:
```vue
<v-list-item to="/setup">
  <template v-slot:prepend>
    <v-icon>mdi-cog</v-icon>
  </template>
  <v-list-item-title>
    Настройка узлов
    <v-badge v-if="newNodesCount > 0" :content="newNodesCount" inline />
  </v-list-item-title>
</v-list-item>
```

---

## 4. Процесс работы (End-to-End)

### Сценарий 1: Настройка ROOT ноды

1. **Прошивка ROOT** → Первый запуск → NVS пустой → режим "new"
2. **ROOT подключается** к WiFi роутеру (credentials из mesh_config.h)
3. **Создает временную mesh** с ID "HYDRO_NEW"
4. **Отправка discovery** через HTTP на сервер (`POST /api/discovery/root`)
5. **Backend создает запись** в `new_nodes` (is_root=true)
6. **Фронтенд показывает** "Обнаружена новая ROOT нода"
7. **Пользователь открывает** мастер настройки ROOT
8. **Заполняет параметры:**
   - node_id: `root_001`
   - zone: `Zone 1`
   - mesh_id: `HYDRO1_ZONE1` (генерируется автоматически)
   - mqtt_broker_host: `192.168.1.100`
   - mqtt_broker_port: `1883`
9. **Отправка конфигурации:** Frontend → Backend → HTTP → ROOT
10. **ROOT сохраняет в NVS**, отправляет подтверждение, перезагружается
11. **После перезагрузки:** подключается к MQTT, создает mesh сеть `HYDRO1_ZONE1`
12. **Backend перемещает** из `new_nodes` в `nodes`

### Сценарий 2: Настройка обычной ноды

1. **Прошивка ноды** (например, pH) → Первый запуск → режим "new"
2. **Нода сканирует** доступные mesh сети
3. **Подключается к ROOT** (mesh: HYDRO1_ZONE1 или HYDRO_NEW)
4. **Отправка discovery** → ROOT → MQTT → Backend
5. **Отправка heartbeat** каждые 10 секунд
6. **Backend создает запись** в `new_nodes` (is_root=false)
7. **Фронтенд показывает** "Обнаружена новая нода pH"
8. **Пользователь открывает** мастер настройки
9. **Заполняет параметры:**
   - node_id: `ph_001`
   - zone: `Zone 1`
   - mesh_id: `HYDRO1_ZONE1` (выбор из списка существующих ROOT)
10. **Отправка конфигурации:** Frontend → Backend → MQTT → ROOT → нода
11. **Нода получает команду** `write_config`
12. **Сохранение в NVS**, отправка подтверждения, перезагрузка
13. **После перезагрузки:** подключается к mesh, запускает сенсоры
14. **Отправка полной телеметрии** в обычном режиме
15. **Backend перемещает** из `new_nodes` в `nodes`
16. **Фронтенд показывает** успех

---

## 5. Ключевые файлы для изменения

### Firmware (ESP32)

1. **common/node_config/node_config.h** + **.c**
   - `node_config_is_configured()`
   - `node_config_get_mesh_mode()`
   - `node_config_save_and_reboot()`

2. **common/mesh_protocol/mesh_protocol.h** + **.c**
   - Константы `MSG_TYPE_DISCOVERY`, `MSG_TYPE_HEARTBEAT`
   - Функции создания discovery/heartbeat сообщений

3. **root_node/main/app_main.c**
   - Логика режима "new" для ROOT
   - HTTP discovery отправка
   - Обработчик `write_config`
   - Создание временной mesh "HYDRO_NEW"

4. **node_ph/main/app_main.c** (и аналогично для всех других нод)
   - Логика режима "new"
   - Auto-scan и подключение к mesh
   - Отправка discovery (1 раз)
   - Задача heartbeat (каждые 10 сек)
   - Обработчик `write_config`

### Backend

1. **database/migrations/2025_01_XX_create_new_nodes_table.php**
   - Таблица `new_nodes`

2. **app/Models/NewNode.php**
   - Модель с методами `configure()`, `moveToNodes()`, `isOnline()`

3. **app/Http/Controllers/NewNodeController.php**
   - CRUD для new nodes
   - Метод `configure()`

4. **app/Services/MqttService.php**
   - Обработка `discovery`
   - Обработка `heartbeat`
   - Обработка `config_confirmation`

5. **app/Events/NewNodeDiscovered.php**
   - Websocket событие

6. **app/Events/NodeConfigured.php**
   - Websocket событие

7. **routes/api.php**
   - Endpoint `POST /api/discovery/root`
   - Routes для `/api/new-nodes/*`

### Frontend

1. **stores/newNodes.ts**
   - State management для new nodes
   - Getters для root/regular/online nodes

2. **views/SetupWizard.vue**
   - Главная страница мастера
   - Логика проверки ROOT

3. **components/setup/RootSetupDialog.vue**
   - Диалог настройки ROOT

4. **components/setup/NodeSetupWizard.vue**
   - Stepper для обычных нод

5. **components/setup/NewNodeCard.vue**
   - Карточка new ноды

6. **components/setup/NewNodesList.vue**
   - Список new нодов

7. **router/index.ts**
   - Route `/setup`

8. **App.vue** (или главное меню)
   - Пункт меню "Настройка узлов" с badge

---

## 6. TODO List

- [ ] **Firmware: node_config** - проверка режима new/configured и save_and_reboot
- [ ] **Firmware: ROOT new mode** - подключение к WiFi, HTTP discovery, ожидание конфигурации
- [ ] **Firmware: Node new mode** - auto-scan mesh, discovery, heartbeat
- [ ] **Firmware: write_config** - обработчик команды с сохранением в NVS и перезагрузкой
- [ ] **Firmware: discovery messages** - отправка discovery (1 раз) и heartbeat (каждые 10 сек)
- [ ] **Backend: migration** - таблица new_nodes с полями is_root и last_heartbeat_at
- [ ] **Backend: model** - NewNode с методами configure, moveToNodes, isOnline
- [ ] **Backend: controller** - NewNodeController с методами index, show, configure, delete
- [ ] **Backend: MQTT handler** - обработка discovery, heartbeat, config_confirmation
- [ ] **Backend: HTTP discovery** - endpoint /discovery/root для ROOT нод в режиме new
- [ ] **Backend: routes** - API маршруты для управления new нодами
- [ ] **Frontend: store** - newNodes с getters для root/regular nodes
- [ ] **Frontend: ROOT setup** - компонент RootSetupDialog для упрощенной настройки ROOT
- [ ] **Frontend: wizard** - SetupWizard с проверкой наличия ROOT и stepper
- [ ] **Frontend: components** - NewNodeCard, NodeSetupWizard с индикацией ROOT
- [ ] **Frontend: routing** - добавление роута /setup и пункта меню с badge

---

## 7. Примечания

- **Безопасность:** Пароль mesh сети ("hydro_mesh_2025") должен быть изменен в продакшене
- **Временная mesh "HYDRO_NEW":** ROOT создает её только в режиме "new", после настройки переключается на постоянную
- **Auto-scan:** Обычные ноды в режиме "new" автоматически ищут любую доступную mesh сеть
- **HTTP для ROOT:** В режиме "new" ROOT отправляет discovery через HTTP, так как MQTT еще не настроен
- **Heartbeat:** Позволяет отслеживать online статус new нод до их настройки
- **Websockets:** События `NewNodeDiscovered` и `NodeConfigured` обновляют фронтенд в реальном времени
- **Очистка:** Таблица `new_nodes` очищается автоматически после успешной настройки (метод `moveToNodes()`)

---

**Статус:** План готов к реализации  
**Дата создания:** 2025-01-07  
**Версия:** 1.0

