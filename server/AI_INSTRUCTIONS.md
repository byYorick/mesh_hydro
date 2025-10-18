# 🤖 ИНСТРУКЦИЯ ДЛЯ ИИ: SERVER

**Веб-сервер для мониторинга и управления Mesh Hydro System**

---

## 🎯 НАЗНАЧЕНИЕ СЕРВЕРА

**Server (Linux/Windows ПК)** - центральный веб-сервер для хранения данных, визуализации и управления всей mesh-системой.

### 🌐 Ключевые функции:

1. **MQTT Broker** - Mosquitto для связи с ROOT узлом
2. **Backend API** - Laravel 10 REST API
3. **Frontend Dashboard** - Vue.js 3 + Vuetify 3 веб-интерфейс
4. **База данных** - PostgreSQL для хранения всей истории
5. **MQTT Listener** - PHP демон для обработки телеметрии
6. **Telegram Bot** - уведомления и команды через Telegram
7. **SMS Gateway** - критичные алерты на телефон

### ⚠️ КРИТИЧНЫЕ ТРЕБОВАНИЯ:

1. ✅ **24/7 работа** - сервер должен работать постоянно
2. ✅ **PostgreSQL** - не MySQL! Нужны JSONB и timestamp with timezone
3. ✅ **Real-time обновления** - WebSocket для live данных
4. ✅ **Хранение истории** - минимум 1 год телеметрии
5. ✅ **Backup** - ежедневный backup БД

---

## 🏗️ АРХИТЕКТУРА

```
SERVER (Linux/Windows ПК)
│
├── Mosquitto MQTT Broker (port 1883)
│   ├── Topic: hydro/telemetry/#
│   ├── Topic: hydro/event/#
│   ├── Topic: hydro/heartbeat/#
│   ├── Topic: hydro/command/#
│   └── Topic: hydro/config/#
│
├── Backend (Laravel 10)
│   ├── REST API (port 8000)
│   ├── Models (Node, Telemetry, Event, Command)
│   ├── Controllers (NodeController, TelemetryController)
│   ├── Services (MqttService, TelegramService, SmsService)
│   ├── Commands (mqtt:listen, telegram:bot)
│   └── Broadcasting (WebSocket для real-time)
│
├── Frontend (Vue.js 3)
│   ├── Dashboard (port 3000 dev / 80 prod)
│   ├── Material Design (Vuetify 3)
│   ├── Real-time charts (Chart.js)
│   ├── Node management
│   └── Settings
│
├── PostgreSQL 15
│   ├── Table: nodes
│   ├── Table: telemetry (партиционирование по дате)
│   ├── Table: events
│   ├── Table: commands
│   └── Table: ota_updates
│
├── Supervisor (управление процессами)
│   ├── mqtt-listener (php artisan mqtt:listen)
│   ├── telegram-bot (php artisan telegram:bot)
│   └── laravel-queue (php artisan queue:work)
│
└── Nginx (reverse proxy)
    ├── Frontend → localhost:3000
    ├── API → localhost:8000
    └── WebSocket → localhost:6001
```

---

## 📦 ТЕХНОЛОГИЧЕСКИЙ СТЕК

### Backend:
- **PHP 8.2+** - современный PHP с типизацией
- **Laravel 10** - фреймворк с отличной документацией
- **PostgreSQL 15** - мощная БД с JSONB
- **phpMQTT** - библиотека для MQTT
- **Supervisor** - управление фоновыми процессами

### Frontend:
- **Vue.js 3** - Composition API
- **Vuetify 3** - Material Design компоненты
- **Chart.js** - графики телеметрии
- **Axios** - HTTP клиент
- **Socket.io** - WebSocket для real-time

### Infrastructure:
- **Mosquitto** - MQTT broker
- **Nginx** - reverse proxy
- **Redis** (опционально) - кэширование и очереди
- **Certbot** (опционально) - SSL сертификаты

---

## 📁 СТРУКТУРА ПРОЕКТА

```
server/
├── backend/                      # Laravel API
│   ├── app/
│   │   ├── Console/
│   │   │   └── Commands/
│   │   │       ├── MqttListenerCommand.php
│   │   │       └── TelegramBotCommand.php
│   │   ├── Http/
│   │   │   └── Controllers/
│   │   │       ├── NodeController.php
│   │   │       ├── TelemetryController.php
│   │   │       ├── EventController.php
│   │   │       └── DashboardController.php
│   │   ├── Models/
│   │   │   ├── Node.php
│   │   │   ├── Telemetry.php
│   │   │   ├── Event.php
│   │   │   └── Command.php
│   │   └── Services/
│   │       ├── MqttService.php
│   │       ├── TelegramService.php
│   │       └── SmsService.php
│   ├── database/
│   │   ├── migrations/
│   │   └── seeders/
│   ├── routes/
│   │   ├── api.php
│   │   └── web.php
│   ├── .env.example
│   ├── composer.json
│   └── artisan
│
├── frontend/                     # Vue.js Dashboard
│   ├── src/
│   │   ├── components/
│   │   │   ├── NodeCard.vue
│   │   │   ├── TelemetryChart.vue
│   │   │   ├── EventLog.vue
│   │   │   └── ControlPanel.vue
│   │   ├── views/
│   │   │   ├── Dashboard.vue
│   │   │   ├── NodeDetail.vue
│   │   │   ├── History.vue
│   │   │   └── Settings.vue
│   │   ├── router/
│   │   ├── store/
│   │   ├── App.vue
│   │   └── main.js
│   ├── package.json
│   └── vite.config.js
│
├── nginx/
│   └── hydro-system.conf         # Nginx конфигурация
│
├── supervisor/
│   ├── mqtt-listener.conf
│   ├── telegram-bot.conf
│   └── laravel-queue.conf
│
└── README.md
```

---

## 💻 РЕАЛИЗАЦИЯ BACKEND (Laravel)

### 1. Models

#### app/Models/Node.php

```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Node extends Model
{
    protected $fillable = [
        'node_id',        // "ph_ec_001"
        'node_type',      // "ph_ec", "climate", "relay", "water"
        'zone',           // "Zone 1"
        'mac_address',    // "AA:BB:CC:DD:EE:FF"
        'online',         // true/false
        'last_seen_at',   // timestamp
        'config',         // jsonb - полная конфигурация узла
        'metadata',       // jsonb - дополнительная информация
    ];

    protected $casts = [
        'online' => 'boolean',
        'last_seen_at' => 'datetime',
        'config' => 'array',
        'metadata' => 'array',
    ];

    // Отношения
    public function telemetry()
    {
        return $this->hasMany(Telemetry::class, 'node_id', 'node_id');
    }

    public function events()
    {
        return $this->hasMany(Event::class, 'node_id', 'node_id');
    }

    // Проверка таймаута (offline > 30 сек)
    public function isOnline(): bool
    {
        if (!$this->last_seen_at) {
            return false;
        }
        
        $timeout = 30; // секунд
        return $this->last_seen_at->diffInSeconds(now()) < $timeout;
    }
}
```

#### app/Models/Telemetry.php

```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Telemetry extends Model
{
    protected $table = 'telemetry';
    
    protected $fillable = [
        'node_id',
        'node_type',
        'data',           // jsonb - данные телеметрии
        'received_at',    // timestamp
    ];

    protected $casts = [
        'data' => 'array',
        'received_at' => 'datetime',
    ];

    // Индексы для быстрых запросов
    // created_at - автоматически индексируется
    // node_id + created_at - составной индекс

    public function node()
    {
        return $this->belongsTo(Node::class, 'node_id', 'node_id');
    }

    // Пример данных для pH/EC:
    // {
    //   "ph": 6.5,
    //   "ec": 1.8,
    //   "temp": 24.3,
    //   "timestamp": 1697548805
    // }
}
```

#### app/Models/Event.php

```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Event extends Model
{
    protected $fillable = [
        'node_id',
        'level',          // "info", "warning", "critical", "emergency"
        'message',        // "pH critically low"
        'data',           // jsonb - дополнительные данные
        'resolved_at',    // timestamp когда решено
    ];

    protected $casts = [
        'data' => 'array',
        'resolved_at' => 'datetime',
    ];

    // Критичное событие?
    public function isCritical(): bool
    {
        return in_array($this->level, ['critical', 'emergency']);
    }

    // Не решено?
    public function isActive(): bool
    {
        return $this->resolved_at === null;
    }
}
```

### 2. Services

#### app/Services/MqttService.php

```php
<?php

namespace App\Services;

use PhpMqtt\Client\MqttClient;
use PhpMqtt\Client\ConnectionSettings;
use App\Models\Node;
use App\Models\Telemetry;
use App\Models\Event;
use Illuminate\Support\Facades\Log;

class MqttService
{
    private MqttClient $mqtt;
    
    public function __construct()
    {
        $host = config('mqtt.host', 'localhost');
        $port = config('mqtt.port', 1883);
        
        $this->mqtt = new MqttClient($host, $port, 'hydro-server-' . uniqid());
    }

    public function connect(): void
    {
        $settings = (new ConnectionSettings())
            ->setUsername(config('mqtt.username'))
            ->setPassword(config('mqtt.password'))
            ->setKeepAliveInterval(60)
            ->setLastWillTopic('hydro/server/status')
            ->setLastWillMessage('offline')
            ->setLastWillQualityOfService(1);

        $this->mqtt->connect($settings, true);
        Log::info('MQTT connected');
    }

    public function subscribe(string $topic, callable $callback): void
    {
        $this->mqtt->subscribe($topic, function ($topic, $message) use ($callback) {
            $callback($topic, $message);
        }, 0);
        
        Log::info("MQTT subscribed to: {$topic}");
    }

    public function publish(string $topic, string $message, int $qos = 0): void
    {
        $this->mqtt->publish($topic, $message, $qos);
        Log::debug("MQTT published: {$topic}");
    }

    public function loop(): void
    {
        $this->mqtt->loop(true);
    }

    public function disconnect(): void
    {
        $this->mqtt->disconnect();
    }

    // Обработка телеметрии
    public function handleTelemetry(string $topic, string $payload): void
    {
        $data = json_decode($payload, true);
        
        if (!$data || !isset($data['node_id'])) {
            Log::warning("Invalid telemetry data: {$payload}");
            return;
        }

        // Сохранение в БД
        Telemetry::create([
            'node_id' => $data['node_id'],
            'node_type' => $data['type'] ?? 'unknown',
            'data' => $data['data'] ?? [],
            'received_at' => now(),
        ]);

        // Обновление статуса узла
        Node::updateOrCreate(
            ['node_id' => $data['node_id']],
            [
                'online' => true,
                'last_seen_at' => now(),
            ]
        );

        Log::debug("Telemetry saved: {$data['node_id']}");
    }

    // Обработка событий
    public function handleEvent(string $topic, string $payload): void
    {
        $data = json_decode($payload, true);
        
        if (!$data || !isset($data['node_id'])) {
            return;
        }

        // Сохранение события
        $event = Event::create([
            'node_id' => $data['node_id'],
            'level' => $data['level'] ?? 'info',
            'message' => $data['message'] ?? '',
            'data' => $data['data'] ?? [],
        ]);

        // Если критичное - отправить уведомления
        if ($event->isCritical()) {
            app(TelegramService::class)->sendAlert($event);
            app(SmsService::class)->sendAlert($event);
        }

        Log::info("Event saved: {$event->message}");
    }

    // Отправка команды узлу
    public function sendCommand(string $nodeId, string $command, array $params = []): void
    {
        $payload = json_encode([
            'type' => 'command',
            'node_id' => $nodeId,
            'command' => $command,
            'params' => $params,
            'timestamp' => time(),
        ]);

        $this->publish("hydro/command/{$nodeId}", $payload, 1);
        
        Log::info("Command sent to {$nodeId}: {$command}");
    }

    // Отправка конфигурации узлу
    public function sendConfig(string $nodeId, array $config): void
    {
        $payload = json_encode([
            'type' => 'config',
            'node_id' => $nodeId,
            'config' => $config,
            'timestamp' => time(),
        ]);

        $this->publish("hydro/config/{$nodeId}", $payload, 1);
        
        Log::info("Config sent to {$nodeId}");
    }
}
```

### 3. Console Commands

#### app/Console/Commands/MqttListenerCommand.php

```php
<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Services\MqttService;

class MqttListenerCommand extends Command
{
    protected $signature = 'mqtt:listen';
    protected $description = 'Listen for MQTT messages from ROOT node';

    public function handle(MqttService $mqtt)
    {
        $this->info('Starting MQTT Listener...');
        
        try {
            $mqtt->connect();
            
            // Подписка на телеметрию
            $mqtt->subscribe('hydro/telemetry/#', function ($topic, $message) use ($mqtt) {
                $mqtt->handleTelemetry($topic, $message);
            });
            
            // Подписка на события
            $mqtt->subscribe('hydro/event/#', function ($topic, $message) use ($mqtt) {
                $mqtt->handleEvent($topic, $message);
            });
            
            // Подписка на heartbeat
            $mqtt->subscribe('hydro/heartbeat/#', function ($topic, $message) {
                // Обновление last_seen_at
                $data = json_decode($message, true);
                if ($data && isset($data['node_id'])) {
                    \App\Models\Node::where('node_id', $data['node_id'])
                        ->update(['last_seen_at' => now()]);
                }
            });
            
            $this->info('MQTT Listener running... Press Ctrl+C to stop');
            
            // Бесконечный цикл
            $mqtt->loop();
            
        } catch (\Exception $e) {
            $this->error("MQTT Error: {$e->getMessage()}");
            return 1;
        }
    }
}
```

### 4. Controllers

#### app/Http/Controllers/NodeController.php

```php
<?php

namespace App\Http\Controllers;

use App\Models\Node;
use App\Services\MqttService;
use Illuminate\Http\Request;

class NodeController extends Controller
{
    // Список всех узлов
    public function index()
    {
        $nodes = Node::with(['telemetry' => function ($query) {
            $query->latest()->limit(1);
        }])->get();

        return response()->json($nodes);
    }

    // Детали узла
    public function show(string $nodeId)
    {
        $node = Node::with(['telemetry', 'events'])
            ->where('node_id', $nodeId)
            ->firstOrFail();

        return response()->json($node);
    }

    // Отправка команды узлу
    public function sendCommand(Request $request, string $nodeId, MqttService $mqtt)
    {
        $validated = $request->validate([
            'command' => 'required|string',
            'params' => 'array',
        ]);

        $node = Node::where('node_id', $nodeId)->firstOrFail();

        if (!$node->isOnline()) {
            return response()->json([
                'error' => 'Node is offline'
            ], 400);
        }

        $mqtt->sendCommand($nodeId, $validated['command'], $validated['params'] ?? []);

        return response()->json([
            'success' => true,
            'message' => 'Command sent to node'
        ]);
    }

    // Обновление конфигурации узла
    public function updateConfig(Request $request, string $nodeId, MqttService $mqtt)
    {
        $node = Node::where('node_id', $nodeId)->firstOrFail();

        $config = $request->all();
        
        // Сохранение в БД
        $node->update(['config' => $config]);

        // Отправка на узел через MQTT
        if ($node->isOnline()) {
            $mqtt->sendConfig($nodeId, $config);
        }

        return response()->json([
            'success' => true,
            'message' => 'Config updated'
        ]);
    }
}
```

---

## 💻 РЕАЛИЗАЦИЯ FRONTEND (Vue.js)

### src/views/Dashboard.vue

```vue
<template>
  <v-container>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3">Hydro System Dashboard</h1>
      </v-col>
    </v-row>

    <v-row>
      <!-- Карточки узлов -->
      <v-col
        v-for="node in nodes"
        :key="node.node_id"
        cols="12"
        md="6"
        lg="4"
      >
        <node-card :node="node" @command="sendCommand" />
      </v-col>
    </v-row>

    <!-- Графики телеметрии -->
    <v-row>
      <v-col cols="12">
        <telemetry-chart :data="chartData" />
      </v-col>
    </v-row>

    <!-- Лог событий -->
    <v-row>
      <v-col cols="12">
        <event-log :events="events" />
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { io } from 'socket.io-client'
import axios from 'axios'
import NodeCard from '@/components/NodeCard.vue'
import TelemetryChart from '@/components/TelemetryChart.vue'
import EventLog from '@/components/EventLog.vue'

const nodes = ref([])
const events = ref([])
const chartData = ref([])
const socket = ref(null)

// Загрузка данных
const loadNodes = async () => {
  try {
    const response = await axios.get('/api/nodes')
    nodes.value = response.data
  } catch (error) {
    console.error('Error loading nodes:', error)
  }
}

const loadEvents = async () => {
  try {
    const response = await axios.get('/api/events?limit=50')
    events.value = response.data
  } catch (error) {
    console.error('Error loading events:', error)
  }
}

// WebSocket для real-time обновлений
const connectWebSocket = () => {
  socket.value = io('http://localhost:6001')

  socket.value.on('telemetry', (data) => {
    // Обновление узла в реальном времени
    const node = nodes.value.find(n => n.node_id === data.node_id)
    if (node) {
      node.last_data = data.data
      node.last_seen_at = new Date()
    }

    // Добавление в график
    chartData.value.push(data)
    if (chartData.value.length > 100) {
      chartData.value.shift()
    }
  })

  socket.value.on('event', (event) => {
    events.value.unshift(event)
    if (events.value.length > 50) {
      events.value.pop()
    }
  })
}

// Отправка команды узлу
const sendCommand = async (nodeId, command, params) => {
  try {
    await axios.post(`/api/nodes/${nodeId}/command`, {
      command,
      params
    })

    // Уведомление пользователя
    console.log(`Command "${command}" sent to ${nodeId}`)
  } catch (error) {
    console.error('Error sending command:', error)
  }
}

onMounted(() => {
  loadNodes()
  loadEvents()
  connectWebSocket()

  // Периодическое обновление
  setInterval(loadNodes, 30000) // Каждые 30 сек
})

onUnmounted(() => {
  if (socket.value) {
    socket.value.disconnect()
  }
})
</script>
```

### src/components/NodeCard.vue

```vue
<template>
  <v-card>
    <v-card-title>
      <v-icon :color="statusColor" class="mr-2">
        {{ statusIcon }}
      </v-icon>
      {{ node.node_id }}
    </v-card-title>

    <v-card-subtitle>
      {{ node.node_type }} • {{ node.zone }}
    </v-card-subtitle>

    <v-card-text>
      <!-- Данные телеметрии -->
      <div v-if="node.node_type === 'ph_ec'">
        <div class="text-h4">pH: {{ lastData.ph?.toFixed(1) }}</div>
        <div class="text-h4">EC: {{ lastData.ec?.toFixed(1) }}</div>
      </div>

      <div v-else-if="node.node_type === 'climate'">
        <div>Temp: {{ lastData.temp?.toFixed(1) }}°C</div>
        <div>Humidity: {{ lastData.humidity?.toFixed(0) }}%</div>
        <div>CO2: {{ lastData.co2 }} ppm</div>
      </div>

      <div class="text-caption mt-2">
        Last seen: {{ formatTime(node.last_seen_at) }}
      </div>
    </v-card-text>

    <v-card-actions>
      <v-btn
        size="small"
        :to="`/nodes/${node.node_id}`"
      >
        Details
      </v-btn>

      <v-spacer />

      <!-- Кнопки управления для разных типов узлов -->
      <v-btn
        v-if="node.node_type === 'relay'"
        size="small"
        color="primary"
        @click="$emit('command', node.node_id, 'open_windows', {})"
      >
        Open Windows
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  node: {
    type: Object,
    required: true
  }
})

const statusColor = computed(() => {
  if (!props.node.last_seen_at) return 'grey'
  
  const seconds = (Date.now() - new Date(props.node.last_seen_at)) / 1000
  
  if (seconds < 30) return 'green'
  if (seconds < 60) return 'orange'
  return 'red'
})

const statusIcon = computed(() => {
  return statusColor.value === 'green' ? 'mdi-check-circle' : 'mdi-alert-circle'
})

const lastData = computed(() => {
  return props.node.last_data || {}
})

const formatTime = (timestamp) => {
  if (!timestamp) return 'Never'
  
  const date = new Date(timestamp)
  const now = new Date()
  const seconds = Math.floor((now - date) / 1000)
  
  if (seconds < 60) return `${seconds}s ago`
  if (seconds < 3600) return `${Math.floor(seconds / 60)}m ago`
  return date.toLocaleTimeString()
}
</script>
```

---

## 🗄️ DATABASE (PostgreSQL)

### Migrations

#### create_nodes_table.php

```php
Schema::create('nodes', function (Blueprint $table) {
    $table->id();
    $table->string('node_id')->unique();  // "ph_ec_001"
    $table->string('node_type');          // "ph_ec", "climate", etc
    $table->string('zone')->nullable();
    $table->string('mac_address')->nullable();
    $table->boolean('online')->default(false);
    $table->timestamp('last_seen_at')->nullable();
    $table->jsonb('config')->nullable();   // Конфигурация узла
    $table->jsonb('metadata')->nullable(); // Дополнительная информация
    $table->timestamps();

    $table->index('node_id');
    $table->index('node_type');
    $table->index('online');
    $table->index('last_seen_at');
});
```

#### create_telemetry_table.php

```php
Schema::create('telemetry', function (Blueprint $table) {
    $table->id();
    $table->string('node_id');
    $table->string('node_type');
    $table->jsonb('data');            // Данные телеметрии
    $table->timestamp('received_at'); // Время получения на сервере
    $table->timestamps();

    $table->index('node_id');
    $table->index(['node_id', 'received_at']); // Составной индекс
    $table->index('received_at');
});

// Партиционирование по дате для больших объемов
// CREATE TABLE telemetry_2025_01 PARTITION OF telemetry
// FOR VALUES FROM ('2025-01-01') TO ('2025-02-01');
```

#### create_events_table.php

```php
Schema::create('events', function (Blueprint $table) {
    $table->id();
    $table->string('node_id');
    $table->enum('level', ['info', 'warning', 'critical', 'emergency']);
    $table->text('message');
    $table->jsonb('data')->nullable();
    $table->timestamp('resolved_at')->nullable();
    $table->timestamps();

    $table->index('node_id');
    $table->index('level');
    $table->index('resolved_at');
    $table->index('created_at');
});
```

---

## ⚙️ КОНФИГУРАЦИЯ

### .env (Backend)

```env
APP_NAME="Hydro System"
APP_ENV=production
APP_DEBUG=false
APP_URL=http://localhost

# PostgreSQL
DB_CONNECTION=pgsql
DB_HOST=127.0.0.1
DB_PORT=5432
DB_DATABASE=hydro_system
DB_USERNAME=hydro
DB_PASSWORD=secure_password

# MQTT
MQTT_HOST=127.0.0.1
MQTT_PORT=1883
MQTT_USERNAME=hydro_server
MQTT_PASSWORD=mqtt_secure_pass

# Telegram
TELEGRAM_BOT_TOKEN=your_bot_token_here
TELEGRAM_CHAT_ID=your_chat_id

# SMS Gateway (опционально)
SMS_GATEWAY_URL=https://sms.ru/api/send
SMS_API_KEY=your_api_key
SMS_PHONE=+79991234567
```

### supervisor/mqtt-listener.conf

```ini
[program:hydro-mqtt-listener]
process_name=%(program_name)s
command=php /path/to/server/backend/artisan mqtt:listen
autostart=true
autorestart=true
startsecs=5
startretries=10
user=www-data
redirect_stderr=true
stdout_logfile=/var/log/supervisor/hydro-mqtt.log
stopwaitsecs=30
```

### nginx/hydro-system.conf

```nginx
server {
    listen 80;
    server_name hydro.local;

    # Frontend (Vue.js)
    location / {
        proxy_pass http://localhost:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }

    # Backend API (Laravel)
    location /api {
        proxy_pass http://localhost:8000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }

    # WebSocket
    location /socket.io {
        proxy_pass http://localhost:6001;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "Upgrade";
    }
}
```

---

## 🚫 ЧТО НЕЛЬЗЯ ДЕЛАТЬ

### 1. ❌ НЕ используй MySQL

```php
// ПЛОХО ❌
// MySQL не поддерживает JSONB эффективно
DB_CONNECTION=mysql

// ХОРОШО ✅
// PostgreSQL с нативной поддержкой JSONB
DB_CONNECTION=pgsql
```

### 2. ❌ НЕ блокируй HTTP запросы

```php
// ПЛОХО ❌
public function sendCommand(Request $request) {
    $mqtt->sendCommand(...);
    sleep(5); // Ожидание ответа ❌
    return response()->json(['status' => 'done']);
}

// ХОРОШО ✅
public function sendCommand(Request $request) {
    $mqtt->sendCommand(...);
    // Ответ придет через WebSocket
    return response()->json(['status' => 'sent']);
}
```

### 3. ❌ НЕ храни все в одной таблице

```sql
-- ПЛОХО ❌
-- 1 таблица на 10M+ записей = медленные запросы
CREATE TABLE all_data (...)

-- ХОРОШО ✅
-- Разные таблицы + партиционирование
CREATE TABLE telemetry (...) PARTITION BY RANGE (received_at);
CREATE TABLE events (...);
CREATE TABLE commands (...);
```

---

## ✅ ЧТО НУЖНО ДЕЛАТЬ

### 1. ✅ Партиционирование телеметрии

```sql
-- Партиции по месяцам
CREATE TABLE telemetry_2025_01 PARTITION OF telemetry
FOR VALUES FROM ('2025-01-01') TO ('2025-02-01');

CREATE TABLE telemetry_2025_02 PARTITION OF telemetry
FOR VALUES FROM ('2025-02-01') TO ('2025-03-01');

-- Автоматическое создание партиций (cron)
```

### 2. ✅ Индексы для быстрых запросов

```sql
-- Составные индексы
CREATE INDEX idx_telemetry_node_time 
ON telemetry (node_id, received_at DESC);

-- JSONB индексы (GIN)
CREATE INDEX idx_telemetry_data ON telemetry USING GIN (data);

-- Поиск по JSONB
SELECT * FROM telemetry 
WHERE data->>'ph' < '6.0';  -- Быстро с GIN индексом!
```

### 3. ✅ Очистка старых данных

```php
// Artisan команда для очистки
php artisan telemetry:cleanup --days=365

// Удаление телеметрии старше 1 года
Telemetry::where('received_at', '<', now()->subYear())->delete();
```

### 4. ✅ Backup БД

```bash
#!/bin/bash
# backup.sh

DATE=$(date +%Y-%m-%d)
pg_dump -U hydro hydro_system > /backup/hydro_$DATE.sql
gzip /backup/hydro_$DATE.sql

# Удаление бэкапов старше 30 дней
find /backup -name "hydro_*.sql.gz" -mtime +30 -delete
```

---

## 🧪 ТЕСТИРОВАНИЕ

### Тест 1: Backend установка

```bash
cd server/backend

# Установка зависимостей
composer install

# Настройка .env
cp .env.example .env
# Отредактировать .env

# Генерация ключа
php artisan key:generate

# Миграции
php artisan migrate

# Тестовые данные
php artisan db:seed

# Запуск сервера
php artisan serve
```

**Ожидаемый результат:**
```
Laravel development server started on http://127.0.0.1:8000
```

### Тест 2: MQTT Listener

```bash
# Терминал 1: MQTT Listener
php artisan mqtt:listen

# Терминал 2: Тест публикации
mosquitto_pub -h localhost -t "hydro/telemetry" \
  -m '{"node_id":"test_001","data":{"ph":6.5}}'
```

**В БД должна появиться запись:**
```sql
SELECT * FROM telemetry ORDER BY created_at DESC LIMIT 1;
```

### Тест 3: Frontend

```bash
cd server/frontend

# Установка
npm install

# Запуск dev сервера
npm run dev
```

**Откройте браузер:** http://localhost:3000

---

## 📊 ЧЕКЛИСТ РАЗРАБОТКИ

### Backend:
- [ ] Laravel установка и настройка
- [ ] PostgreSQL база данных
- [ ] Модели (Node, Telemetry, Event, Command)
- [ ] Миграции и индексы
- [ ] MQTT Service
- [ ] MQTT Listener Command
- [ ] API Controllers (Node, Telemetry, Event)
- [ ] Telegram Service
- [ ] SMS Service (опционально)
- [ ] Broadcasting (WebSocket)

### Frontend:
- [ ] Vue.js + Vuetify установка
- [ ] Axios HTTP клиент
- [ ] Socket.io WebSocket
- [ ] Dashboard view
- [ ] NodeCard component
- [ ] TelemetryChart component (Chart.js)
- [ ] EventLog component
- [ ] NodeDetail view
- [ ] Settings view

### Infrastructure:
- [ ] Mosquitto MQTT Broker
- [ ] Supervisor конфигурация
- [ ] Nginx reverse proxy
- [ ] SSL сертификаты (Certbot)
- [ ] Backup скрипт (cron)
- [ ] Мониторинг (Grafana опционально)

### Тестирование:
- [ ] MQTT публикация → БД
- [ ] REST API эндпоинты
- [ ] WebSocket real-time
- [ ] Telegram уведомления
- [ ] Frontend UI/UX
- [ ] Load testing (1000+ записей/мин)

---

## 📚 ДОПОЛНИТЕЛЬНЫЕ РЕСУРСЫ

### Документация:
- [Laravel 10](https://laravel.com/docs/10.x)
- [Vue.js 3](https://vuejs.org/guide/introduction.html)
- [Vuetify 3](https://vuetifyjs.com/en/)
- [PostgreSQL](https://www.postgresql.org/docs/)
- [Mosquitto](https://mosquitto.org/documentation/)

### Полезные пакеты:
- `php-mqtt/client` - MQTT клиент для PHP
- `predis/predis` - Redis для кэширования
- `laravel/horizon` - мониторинг очередей
- `spatie/laravel-backup` - автоматический backup

---

**Server - мозг всей системы! Храни данные, визуализируй, управляй!** 🧠

**Приоритет:** 🔴 ВЫСОКИЙ (Фаза 5)

**Время реализации:** 5-7 дней

