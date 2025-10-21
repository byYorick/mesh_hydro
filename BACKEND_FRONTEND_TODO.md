# Backend и Frontend обновления для pH/EC нод

**Статус:** 📝 Требуется реализация  
**Приоритет:** Средний (ноды работают автономно, но не отображаются в интерфейсе)

---

## 🔧 Backend обновления

### 1. Модель Node (server/backend/app/Models/Node.php)

Добавить новые типы нод в константы:

```php
class Node extends Model
{
    const TYPE_PH_EC = 'ph_ec';
    const TYPE_PH = 'ph';      // НОВЫЙ
    const TYPE_EC = 'ec';      // НОВЫЙ
    const TYPE_CLIMATE = 'climate';
    const TYPE_DISPLAY = 'display';
    const TYPE_RELAY = 'relay';
    const TYPE_WATER = 'water';
    
    protected $fillable = [
        'node_id',
        'node_type',
        'mac',
        'zone',
        'online',
        'last_seen',
        'config',
        // ... остальные поля
    ];
}
```

### 2. MQTT Service (server/backend/app/Services/MqttService.php)

Обновить обработку discovery и telemetry для новых типов:

```php
private function handleDiscovery(array $data)
{
    $nodeType = $data['node_type'] ?? null;
    
    // Поддержка новых типов
    if (in_array($nodeType, ['ph', 'ec', 'ph_ec', 'climate', ...])) {
        $node = Node::updateOrCreate(
            ['node_id' => $data['node_id']],
            [
                'node_type' => $nodeType,
                'mac' => $data['mac'] ?? null,
                'online' => true,
                'last_seen' => now(),
                'sensors' => json_encode($data['sensors'] ?? []),
                'actuators' => json_encode($data['actuators'] ?? []),
            ]
        );
        
        // Отправка через WebSocket
        broadcast(new NodeDiscovered($node));
    }
}

private function handleTelemetry(array $data)
{
    $nodeId = $data['node_id'] ?? null;
    $telemetryData = $data['data'] ?? [];
    
    // Обработка pH ноды
    if ($node->node_type === 'ph') {
        SensorData::create([
            'node_id' => $node->id,
            'sensor_type' => 'ph',
            'value' => $telemetryData['ph'] ?? null,
            'timestamp' => now(),
        ]);
        
        // Дополнительные данные насосов
        // ...
    }
    
    // Обработка EC ноды
    if ($node->node_type === 'ec') {
        SensorData::create([
            'node_id' => $node->id,
            'sensor_type' => 'ec',
            'value' => $telemetryData['ec'] ?? null,
            'timestamp' => now(),
        ]);
        
        // Дополнительные данные насосов
        // ...
    }
}
```

### 3. API Controllers

Создать endpoints для управления pH и EC нодами:

**server/backend/app/Http/Controllers/PhNodeController.php:**
```php
class PhNodeController extends Controller
{
    public function setTarget(Request $request, $nodeId)
    {
        $validated = $request->validate([
            'target' => 'required|numeric|between:5.5,7.5'
        ]);
        
        // Отправка команды через MQTT
        $command = [
            'type' => 'command',
            'node_id' => $nodeId,
            'command' => 'set_ph_target',
            'params' => [
                'target' => $validated['target']
            ]
        ];
        
        MqttService::publish("mesh/command/{$nodeId}", $command);
        
        return response()->json(['success' => true]);
    }
    
    public function setPidParams(Request $request, $nodeId)
    {
        // Обновление PID параметров
        // ...
    }
}
```

**server/backend/app/Http/Controllers/EcNodeController.php:**
```php
class EcNodeController extends Controller
{
    public function setTarget(Request $request, $nodeId)
    {
        $validated = $request->validate([
            'target' => 'required|numeric|between:1.5,4.0'
        ]);
        
        // Отправка команды через MQTT
        $command = [
            'type' => 'command',
            'node_id' => $nodeId,
            'command' => 'set_ec_target',
            'params' => [
                'target' => $validated['target']
            ]
        ];
        
        MqttService::publish("mesh/command/{$nodeId}", $command);
        
        return response()->json(['success' => true]);
    }
}
```

### 4. Routes (server/backend/routes/api.php)

Добавить новые маршруты:

```php
Route::prefix('nodes/ph')->group(function () {
    Route::get('/', [PhNodeController::class, 'index']);
    Route::post('/{nodeId}/target', [PhNodeController::class, 'setTarget']);
    Route::post('/{nodeId}/pid', [PhNodeController::class, 'setPidParams']);
    Route::post('/{nodeId}/emergency-stop', [PhNodeController::class, 'emergencyStop']);
});

Route::prefix('nodes/ec')->group(function () {
    Route::get('/', [EcNodeController::class, 'index']);
    Route::post('/{nodeId}/target', [EcNodeController::class, 'setTarget']);
    Route::post('/{nodeId}/pid', [EcNodeController::class, 'setPidParams']);
    Route::post('/{nodeId}/emergency-stop', [EcNodeController::class, 'emergencyStop']);
});
```

---

## 🎨 Frontend обновления

### 1. Компонент для pH ноды

**server/frontend/src/components/nodes/PhNode.vue:**

```vue
<template>
  <div class="node-card ph-node">
    <div class="node-header">
      <h3>{{ node.node_id }}</h3>
      <span class="node-type badge">pH Control</span>
      <span :class="['status-badge', node.online ? 'online' : 'offline']">
        {{ node.online ? 'Online' : 'Offline' }}
      </span>
    </div>
    
    <div class="node-body">
      <!-- Текущее значение pH -->
      <div class="sensor-value">
        <div class="value-large">{{ currentPh.toFixed(2) }}</div>
        <div class="value-label">pH</div>
      </div>
      
      <!-- Target и диапазон -->
      <div class="config-section">
        <label>Target pH:</label>
        <input 
          type="number" 
          v-model.number="phTarget" 
          step="0.1"
          min="5.5"
          max="7.5"
          @change="updateTarget"
        />
      </div>
      
      <!-- График -->
      <div class="chart-container">
        <LineChart :data="chartData" :options="chartOptions" />
      </div>
      
      <!-- Статистика насосов -->
      <div class="pump-stats">
        <div class="pump-stat">
          <span>pH UP:</span>
          <span>{{ pumpStats.ph_up_ml.toFixed(1) }} ml</span>
        </div>
        <div class="pump-stat">
          <span>pH DOWN:</span>
          <span>{{ pumpStats.ph_down_ml.toFixed(1) }} ml</span>
        </div>
      </div>
      
      <!-- Кнопки управления -->
      <div class="actions">
        <button @click="emergencyStop" class="btn-danger">
          Emergency Stop
        </button>
        <button @click="showPidConfig" class="btn-secondary">
          PID Settings
        </button>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'PhNode',
  props: {
    node: Object
  },
  data() {
    return {
      currentPh: 0,
      phTarget: 6.5,
      pumpStats: {
        ph_up_ml: 0,
        ph_down_ml: 0
      },
      chartData: {
        labels: [],
        datasets: [{
          label: 'pH',
          data: [],
          borderColor: '#4CAF50'
        }]
      }
    }
  },
  methods: {
    async updateTarget() {
      await this.$axios.post(`/api/nodes/ph/${this.node.node_id}/target`, {
        target: this.phTarget
      });
    },
    async emergencyStop() {
      await this.$axios.post(`/api/nodes/ph/${this.node.node_id}/emergency-stop`);
    },
    showPidConfig() {
      // Открыть модальное окно с настройками PID
    }
  }
}
</script>
```

### 2. Компонент для EC ноды

**server/frontend/src/components/nodes/EcNode.vue:**

Аналогичная структура, но для EC с 3 насосами:

```vue
<template>
  <div class="node-card ec-node">
    <!-- Аналогично PhNode.vue, но для EC -->
    <div class="sensor-value">
      <div class="value-large">{{ currentEc.toFixed(2) }}</div>
      <div class="value-label">EC (mS/cm)</div>
    </div>
    
    <!-- 3 насоса вместо 2 -->
    <div class="pump-stats">
      <div class="pump-stat">
        <span>EC A:</span>
        <span>{{ pumpStats.ec_a_ml.toFixed(1) }} ml</span>
      </div>
      <div class="pump-stat">
        <span>EC B:</span>
        <span>{{ pumpStats.ec_b_ml.toFixed(1) }} ml</span>
      </div>
      <div class="pump-stat">
        <span>EC C:</span>
        <span>{{ pumpStats.ec_c_ml.toFixed(1) }} ml</span>
      </div>
    </div>
  </div>
</template>
```

### 3. Обновление Dashboard

**server/frontend/src/views/Dashboard.vue:**

```vue
<template>
  <div class="dashboard">
    <div class="nodes-grid">
      <!-- pH ноды -->
      <PhNode 
        v-for="node in phNodes" 
        :key="node.id" 
        :node="node" 
      />
      
      <!-- EC ноды -->
      <EcNode 
        v-for="node in ecNodes" 
        :key="node.id" 
        :node="node" 
      />
      
      <!-- Другие типы нод -->
      <!-- ... -->
    </div>
  </div>
</template>

<script>
import PhNode from '@/components/nodes/PhNode.vue';
import EcNode from '@/components/nodes/EcNode.vue';

export default {
  components: {
    PhNode,
    EcNode
  },
  computed: {
    phNodes() {
      return this.$store.state.nodes.filter(n => n.node_type === 'ph');
    },
    ecNodes() {
      return this.$store.state.nodes.filter(n => n.node_type === 'ec');
    }
  }
}
</script>
```

### 4. Vuex Store обновления

**server/frontend/src/store/modules/nodes.js:**

```javascript
export default {
  state: {
    nodes: []
  },
  mutations: {
    SET_NODES(state, nodes) {
      state.nodes = nodes;
    },
    UPDATE_NODE_DATA(state, { nodeId, data }) {
      const node = state.nodes.find(n => n.node_id === nodeId);
      if (node) {
        Object.assign(node, data);
      }
    }
  },
  actions: {
    // WebSocket подписка на telemetry
    subscribeToTelemetry({ commit }) {
      window.Echo.channel('telemetry')
        .listen('TelemetryReceived', (e) => {
          commit('UPDATE_NODE_DATA', {
            nodeId: e.node_id,
            data: e.data
          });
        });
    }
  }
}
```

---

## 📊 Миграции базы данных

Если требуется, обновить таблицы:

```php
Schema::table('nodes', function (Blueprint $table) {
    // Добавить новые поля если нужно
    $table->json('pump_stats')->nullable();
    $table->float('current_value')->nullable();
    $table->float('target_value')->nullable();
});
```

---

## ✅ Чек-лист реализации

### Backend:
- [ ] Обновить модель Node
- [ ] Обновить MQTT Service (discovery/telemetry)
- [ ] Создать PhNodeController
- [ ] Создать EcNodeController
- [ ] Добавить routes
- [ ] Создать миграции (если нужно)
- [ ] Тестирование API endpoints

### Frontend:
- [ ] Создать компонент PhNode.vue
- [ ] Создать компонент EcNode.vue
- [ ] Обновить Dashboard.vue
- [ ] Обновить Vuex store
- [ ] Добавить WebSocket слушатели
- [ ] Создать модальное окно PID настроек
- [ ] Стилизация компонентов

---

**Приоритет:** Средний  
**Время:** ~4-6 часов работы  
**Зависимости:** Ноды уже работают автономно, это только для визуализации

