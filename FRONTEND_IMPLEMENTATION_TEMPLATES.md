# Frontend - Шаблоны реализации мультизонной системы

## Обзор

Этот документ содержит готовые шаблоны кода для интеграции мультизонной архитектуры во frontend.

---

## 1. API клиент для зон

**Файл:** `server/frontend/src/api/zones.ts`

```typescript
import axios from 'axios';

export interface Zone {
  zone: string;
  nodes_total: number;
  nodes_online: number;
}

export interface ZoneStats {
  zone: string;
  nodes_total: number;
  nodes_online: number;
  nodes_by_type: Array<{
    node_type: string;
    count: number;
    online: number;
  }>;
  root_node: any;
  last_activity: string;
}

export interface ZoneHealth {
  zone: string;
  status: 'healthy' | 'degraded' | 'critical';
  issues: Array<{
    severity: string;
    message: string;
    code: string;
  }>;
  timestamp: string;
}

export const zonesApi = {
  /**
   * Получить список всех зон
   */
  async getAll(): Promise<Zone[]> {
    const response = await axios.get('/api/zones');
    return response.data.zones;
  },

  /**
   * Получить узлы конкретной зоны
   */
  async getNodes(zone: string) {
    const response = await axios.get(`/api/zones/${zone}/nodes`);
    return response.data.nodes;
  },

  /**
   * Получить телеметрию зоны
   */
  async getTelemetry(zone: string, params?: { hours?: number; limit?: number }) {
    const response = await axios.get(`/api/zones/${zone}/telemetry`, { params });
    return response.data.telemetry;
  },

  /**
   * Получить статистику зоны
   */
  async getStats(zone: string): Promise<ZoneStats> {
    const response = await axios.get(`/api/zones/${zone}/stats`);
    return response.data;
  },

  /**
   * Проверить здоровье зоны
   */
  async getHealth(zone: string): Promise<ZoneHealth> {
    const response = await axios.get(`/api/zones/${zone}/health`);
    return response.data;
  }
};
```

---

## 2. Pinia Store для зон

**Файл:** `server/frontend/src/store/zones.ts`

```typescript
import { defineStore } from 'pinia';
import { zonesApi, Zone, ZoneStats, ZoneHealth } from '@/api/zones';
import { subscribeToZone, unsubscribeFromZone } from '@/services/websocket';

interface ZonesState {
  zones: Zone[];
  currentZone: string | null;
  zoneStats: Record<string, ZoneStats>;
  zoneHealth: Record<string, ZoneHealth>;
  loading: boolean;
  error: string | null;
}

export const useZonesStore = defineStore('zones', {
  state: (): ZonesState => ({
    zones: [],
    currentZone: localStorage.getItem('currentZone') || null,
    zoneStats: {},
    zoneHealth: {},
    loading: false,
    error: null
  }),

  getters: {
    currentZoneStats: (state) => {
      return state.currentZone ? state.zoneStats[state.currentZone] : null;
    },

    currentZoneHealth: (state) => {
      return state.currentZone ? state.zoneHealth[state.currentZone] : null;
    },

    hasZones: (state) => state.zones.length > 0
  },

  actions: {
    /**
     * Загрузить список зон
     */
    async fetchZones() {
      this.loading = true;
      this.error = null;
      
      try {
        this.zones = await zonesApi.getAll();
        
        // Установить первую зону как текущую если не выбрана
        if (!this.currentZone && this.zones.length > 0) {
          // Фильтруем UNCONFIGURED
          const validZones = this.zones.filter(z => z.zone !== 'UNCONFIGURED');
          if (validZones.length > 0) {
            this.setCurrentZone(validZones[0].zone);
          }
        }
      } catch (error: any) {
        this.error = 'Failed to fetch zones';
        console.error('Zones fetch error:', error);
      } finally {
        this.loading = false;
      }
    },

    /**
     * Загрузить статистику зоны
     */
    async fetchZoneStats(zone: string) {
      try {
        const stats = await zonesApi.getStats(zone);
        this.zoneStats[zone] = stats;
      } catch (error) {
        console.error(`Failed to fetch stats for zone ${zone}:`, error);
      }
    },

    /**
     * Загрузить health check зоны
     */
    async fetchZoneHealth(zone: string) {
      try {
        const health = await zonesApi.getHealth(zone);
        this.zoneHealth[zone] = health;
      } catch (error) {
        console.error(`Failed to fetch health for zone ${zone}:`, error);
      }
    },

    /**
     * Установить текущую зону
     */
    setCurrentZone(zone: string) {
      // Отписаться от предыдущей зоны
      if (this.currentZone) {
        unsubscribeFromZone(this.currentZone);
      }
      
      this.currentZone = zone;
      localStorage.setItem('currentZone', zone);
      
      // Загрузить данные новой зоны
      this.fetchZoneStats(zone);
      this.fetchZoneHealth(zone);
      
      // Подписаться на WebSocket события новой зоны
      this.subscribeToCurrentZone();
    },

    /**
     * Подписаться на WebSocket события текущей зоны
     */
    subscribeToCurrentZone() {
      if (!this.currentZone) return;

      const nodesStore = useNodesStore();
      
      subscribeToZone(this.currentZone, {
        onNodeDiscovered: (node) => {
          console.log('Node discovered:', node);
          nodesStore.addNode(node);
          this.fetchZoneStats(this.currentZone!);
        },
        onNodeStatusChanged: (data) => {
          console.log('Node status changed:', data);
          nodesStore.updateNodeStatus(data);
          this.fetchZoneStats(this.currentZone!);
        },
        onTelemetryReceived: (data) => {
          console.log('Telemetry received:', data);
          // Обновить данные узла
        }
      });
    },

    /**
     * Обновить статус зоны (для периодического обновления)
     */
    async refreshCurrentZone() {
      if (this.currentZone) {
        await Promise.all([
          this.fetchZoneStats(this.currentZone),
          this.fetchZoneHealth(this.currentZone)
        ]);
      }
    }
  }
});

// Для использования в других stores
import { useNodesStore } from './nodes';
```

---

## 3. WebSocket сервис

**Файл:** `server/frontend/src/services/websocket.ts`

```typescript
import Echo from 'laravel-echo';
import Pusher from 'pusher-js';

// @ts-ignore
window.Pusher = Pusher;

// Инициализация Laravel Echo
export const echo = new Echo({
  broadcaster: 'pusher',
  key: import.meta.env.VITE_WEBSOCKET_KEY || 'hydro-key',
  wsHost: import.meta.env.VITE_WEBSOCKET_HOST || window.location.hostname,
  wsPort: import.meta.env.VITE_WEBSOCKET_PORT || 6001,
  wssPort: import.meta.env.VITE_WEBSOCKET_PORT || 6001,
  forceTLS: import.meta.env.VITE_WEBSOCKET_FORCE_TLS === 'true',
  disableStats: true,
  enabledTransports: ['ws', 'wss'],
});

interface ZoneCallbacks {
  onNodeDiscovered?: (node: any) => void;
  onNodeStatusChanged?: (data: any) => void;
  onTelemetryReceived?: (data: any) => void;
  onCommandSent?: (data: any) => void;
}

/**
 * Подписаться на события зоны
 */
export function subscribeToZone(zone: string, callbacks: ZoneCallbacks) {
  console.log(`Subscribing to zone: ${zone}`);
  
  const channel = echo.channel(`zone.${zone}`);
  
  if (callbacks.onNodeDiscovered) {
    channel.listen('.NodeDiscovered', callbacks.onNodeDiscovered);
  }
  
  if (callbacks.onNodeStatusChanged) {
    channel.listen('.NodeStatusChanged', callbacks.onNodeStatusChanged);
  }
  
  if (callbacks.onTelemetryReceived) {
    channel.listen('.TelemetryReceived', callbacks.onTelemetryReceived);
  }
  
  if (callbacks.onCommandSent) {
    channel.listen('.CommandSent', callbacks.onCommandSent);
  }
  
  return channel;
}

/**
 * Отписаться от событий зоны
 */
export function unsubscribeFromZone(zone: string) {
  console.log(`Unsubscribing from zone: ${zone}`);
  echo.leave(`zone.${zone}`);
}

/**
 * Подключиться к WebSocket
 */
export function connectWebSocket() {
  echo.connector.connect();
}

/**
 * Отключиться от WebSocket
 */
export function disconnectWebSocket() {
  echo.disconnect();
}
```

---

## 4. ZoneSelector компонент

**Файл:** `server/frontend/src/components/zones/ZoneSelector.vue`

```vue
<template>
  <div class="zone-selector">
    <label for="zone-select" class="zone-label">
      <i class="icon-globe"></i>
      Зона:
    </label>
    <select 
      id="zone-select" 
      v-model="selectedZone" 
      @change="onZoneChange"
      class="zone-select"
      :disabled="loading || zones.length === 0"
    >
      <option value="" disabled>
        {{ loading ? 'Загрузка...' : 'Выберите зону' }}
      </option>
      <option 
        v-for="zone in zones" 
        :key="zone.zone" 
        :value="zone.zone"
        :disabled="zone.zone === 'UNCONFIGURED'"
      >
        {{ formatZoneName(zone.zone) }} 
        <span class="zone-stats">
          ({{ zone.nodes_online }}/{{ zone.nodes_total }})
        </span>
      </option>
    </select>
    
    <div v-if="currentZoneHealth" class="zone-health-indicator">
      <span 
        :class="['health-badge', `health-${currentZoneHealth.status}`]"
        :title="getHealthTooltip()"
      >
        {{ healthStatusText }}
      </span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue';
import { useZonesStore } from '@/store/zones';

const zonesStore = useZonesStore();
const selectedZone = ref('');

const zones = computed(() => zonesStore.zones.filter(z => z.zone !== 'UNCONFIGURED'));
const loading = computed(() => zonesStore.loading);
const currentZoneHealth = computed(() => zonesStore.currentZoneHealth);

const healthStatusText = computed(() => {
  if (!currentZoneHealth.value) return '';
  
  const statusMap = {
    'healthy': '✓ OK',
    'degraded': '⚠ Warning',
    'critical': '✗ Critical'
  };
  
  return statusMap[currentZoneHealth.value.status] || '';
});

onMounted(async () => {
  await zonesStore.fetchZones();
  if (zonesStore.currentZone) {
    selectedZone.value = zonesStore.currentZone;
  }
});

// Автообновление каждые 30 секунд
setInterval(() => {
  zonesStore.refreshCurrentZone();
}, 30000);

const onZoneChange = () => {
  if (selectedZone.value) {
    zonesStore.setCurrentZone(selectedZone.value);
  }
};

const formatZoneName = (zone: string): string => {
  return zone.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase());
};

const getHealthTooltip = (): string => {
  if (!currentZoneHealth.value || currentZoneHealth.value.issues.length === 0) {
    return 'Zone healthy';
  }
  
  return currentZoneHealth.value.issues
    .map(issue => `${issue.severity}: ${issue.message}`)
    .join(', ');
};
</script>

<style scoped>
.zone-selector {
  display: flex;
  align-items: center;
  gap: 1rem;
  padding: 0.5rem;
  background: var(--surface-card);
  border-radius: 8px;
}

.zone-label {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-weight: 600;
  color: var(--text-color);
}

.zone-select {
  min-width: 250px;
  padding: 0.5rem 1rem;
  border: 1px solid var(--surface-border);
  border-radius: 6px;
  background: white;
  font-size: 1rem;
  cursor: pointer;
  transition: all 0.2s;
}

.zone-select:hover:not(:disabled) {
  border-color: var(--primary-color);
}

.zone-select:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.zone-stats {
  color: var(--text-color-secondary);
  font-size: 0.9em;
}

.zone-health-indicator {
  margin-left: auto;
}

.health-badge {
  padding: 0.25rem 0.75rem;
  border-radius: 12px;
  font-size: 0.875rem;
  font-weight: 600;
  white-space: nowrap;
}

.health-healthy {
  background: #d4edda;
  color: #155724;
}

.health-degraded {
  background: #fff3cd;
  color: #856404;
}

.health-critical {
  background: #f8d7da;
  color: #721c24;
}
</style>
```

---

## 5. ZoneDashboard компонент

**Файл:** `server/frontend/src/components/zones/ZoneDashboard.vue`

```vue
<template>
  <div class="zone-dashboard">
    <div v-if="loading" class="loading-state">
      <i class="pi pi-spin pi-spinner"></i>
      Загрузка статистики зоны...
    </div>
    
    <div v-else-if="stats" class="dashboard-content">
      <div class="stats-grid">
        <div class="stat-card">
          <div class="stat-icon nodes-total">
            <i class="pi pi-sitemap"></i>
          </div>
          <div class="stat-details">
            <span class="stat-label">Всего узлов</span>
            <span class="stat-value">{{ stats.nodes_total }}</span>
          </div>
        </div>
        
        <div class="stat-card">
          <div class="stat-icon nodes-online">
            <i class="pi pi-check-circle"></i>
          </div>
          <div class="stat-details">
            <span class="stat-label">Онлайн</span>
            <span class="stat-value success">{{ stats.nodes_online }}</span>
          </div>
        </div>
        
        <div class="stat-card">
          <div class="stat-icon nodes-offline">
            <i class="pi pi-times-circle"></i>
          </div>
          <div class="stat-details">
            <span class="stat-label">Офлайн</span>
            <span class="stat-value warning">
              {{ stats.nodes_total - stats.nodes_online }}
            </span>
          </div>
        </div>
        
        <div class="stat-card">
          <div class="stat-icon root-node">
            <i class="pi pi-server"></i>
          </div>
          <div class="stat-details">
            <span class="stat-label">Root Node</span>
            <span class="stat-value">
              {{ stats.root_node?.node_id || 'N/A' }}
            </span>
          </div>
        </div>
      </div>
      
      <div class="nodes-by-type">
        <h3>Узлы по типам</h3>
        <div class="type-list">
          <div 
            v-for="item in stats.nodes_by_type" 
            :key="item.node_type"
            class="type-item"
          >
            <span class="type-name">{{ formatNodeType(item.node_type) }}:</span>
            <span class="type-count">
              {{ item.online }} / {{ item.count }}
              <small class="text-muted">онлайн</small>
            </span>
          </div>
        </div>
      </div>
      
      <div v-if="stats.last_activity" class="last-activity">
        <i class="pi pi-clock"></i>
        Последняя активность: {{ formatLastActivity(stats.last_activity) }}
      </div>
    </div>
    
    <div v-else class="empty-state">
      <i class="pi pi-inbox"></i>
      <p>Нет данных о зоне</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useZonesStore } from '@/store/zones';
import { formatDistance } from 'date-fns';
import { ru } from 'date-fns/locale';

const zonesStore = useZonesStore();

const stats = computed(() => zonesStore.currentZoneStats);
const loading = computed(() => zonesStore.loading);

const formatNodeType = (type: string): string => {
  const typeMap: Record<string, string> = {
    'climate': 'Климат',
    'ph': 'pH',
    'ec': 'EC',
    'ph_ec': 'pH+EC',
    'water': 'Вода',
    'relay': 'Реле',
    'display': 'Дисплей',
    'root': 'Root'
  };
  return typeMap[type] || type;
};

const formatLastActivity = (dateTime: string): string => {
  return formatDistance(new Date(dateTime), new Date(), {
    addSuffix: true,
    locale: ru
  });
};
</script>

<style scoped>
.zone-dashboard {
  padding: 1rem;
}

.loading-state,
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 3rem;
  color: var(--text-color-secondary);
  gap: 1rem;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 1rem;
  margin-bottom: 2rem;
}

.stat-card {
  display: flex;
  align-items: center;
  gap: 1rem;
  background: white;
  border-radius: 12px;
  padding: 1.5rem;
  box-shadow: 0 2px 8px rgba(0,0,0,0.08);
  transition: transform 0.2s, box-shadow 0.2s;
}

.stat-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0,0,0,0.12);
}

.stat-icon {
  width: 48px;
  height: 48px;
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.5rem;
}

.stat-icon.nodes-total {
  background: #e3f2fd;
  color: #1976d2;
}

.stat-icon.nodes-online {
  background: #e8f5e9;
  color: #388e3c;
}

.stat-icon.nodes-offline {
  background: #fff3e0;
  color: #f57c00;
}

.stat-icon.root-node {
  background: #f3e5f5;
  color: #7b1fa2;
}

.stat-details {
  display: flex;
  flex-direction: column;
}

.stat-label {
  font-size: 0.875rem;
  color: var(--text-color-secondary);
  margin-bottom: 0.25rem;
}

.stat-value {
  font-size: 1.5rem;
  font-weight: bold;
  color: var(--text-color);
}

.stat-value.success {
  color: #10b981;
}

.stat-value.warning {
  color: #f59e0b;
}

.nodes-by-type {
  background: white;
  border-radius: 12px;
  padding: 1.5rem;
  box-shadow: 0 2px 8px rgba(0,0,0,0.08);
  margin-bottom: 1rem;
}

.nodes-by-type h3 {
  margin: 0 0 1rem 0;
  color: var(--text-color);
}

.type-list {
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.type-item {
  display: flex;
  justify-content: space-between;
  padding: 0.75rem;
  background: var(--surface-50);
  border-radius: 8px;
}

.type-name {
  font-weight: 500;
  color: var(--text-color);
}

.type-count {
  color: var(--text-color-secondary);
}

.last-activity {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 1rem;
  background: var(--surface-50);
  border-radius: 8px;
  color: var(--text-color-secondary);
  font-size: 0.875rem;
}
</style>
```

---

## 6. Обновление NodesList (фильтрация)

**Добавить в существующий `NodesList.vue`:**

```vue
<template>
  <div class="nodes-list">
    <!-- Добавить селектор зоны -->
    <zone-selector />
    
    <div class="nodes-grid">
      <node-card 
        v-for="node in filteredNodes" 
        :key="node.node_id"
        :node="node"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useNodesStore } from '@/store/nodes';
import { useZonesStore } from '@/store/zones';
import ZoneSelector from '@/components/zones/ZoneSelector.vue';
import NodeCard from '@/components/NodeCard.vue';

const nodesStore = useNodesStore();
const zonesStore = useZonesStore();

// Фильтрация узлов по текущей зоне
const filteredNodes = computed(() => {
  const currentZone = zonesStore.currentZone;
  if (!currentZone) return nodesStore.nodes;
  
  return nodesStore.nodes.filter(node => node.zone === currentZone);
});
</script>
```

---

## 7. Обновление NodeCard (отображение зоны)

**Добавить в существующий `NodeCard.vue`:**

```vue
<template>
  <div class="node-card">
    <div class="node-header">
      <h3>{{ node.node_id }}</h3>
      <span class="zone-badge" :title="`Zone: ${node.zone}`">
        <i class="pi pi-map-marker"></i>
        {{ formatZone(node.zone) }}
      </span>
    </div>
    <!-- остальной контент карточки -->
  </div>
</template>

<script setup lang="ts">
const formatZone = (zone: string): string => {
  if (zone === 'UNCONFIGURED') return 'Не настроен';
  return zone.replace(/_/g, ' ').replace(/zone /i, '');
};
</script>

<style scoped>
.node-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 1rem;
}

.zone-badge {
  display: flex;
  align-items: center;
  gap: 0.25rem;
  background: var(--primary-color);
  color: white;
  padding: 0.25rem 0.75rem;
  border-radius: 12px;
  font-size: 0.75rem;
  font-weight: 600;
}
</style>
```

---

## 8. Добавление роутов

**Обновить `server/frontend/src/router/index.ts`:**

```typescript
import { createRouter, createWebHistory } from 'vue-router';

const router = createRouter({
  history: createWebHistory(),
  routes: [
    // Существующие роуты...
    
    // Зоны
    {
      path: '/zones',
      name: 'Zones',
      component: () => import('@/views/Zones.vue'),
      meta: { title: 'Зоны' }
    },
    {
      path: '/zones/:zone',
      name: 'ZoneDetails',
      component: () => import('@/views/ZoneDetails.vue'),
      meta: { title: 'Детали зоны' }
    }
  ]
});

export default router;
```

---

## Чек-лист реализации Frontend

- [ ] Создать `src/api/zones.ts`
- [ ] Создать `src/store/zones.ts`
- [ ] Создать `src/services/websocket.ts`
- [ ] Создать `src/components/zones/ZoneSelector.vue`
- [ ] Создать `src/components/zones/ZoneDashboard.vue`
- [ ] Обновить `src/components/NodesList.vue` (фильтрация)
- [ ] Обновить `src/components/NodeCard.vue` (badge зоны)
- [ ] Добавить роуты в `src/router/index.ts`
- [ ] Добавить `date-fns` для форматирования дат: `npm install date-fns`
- [ ] Настроить WebSocket (`.env` файл)
- [ ] Протестировать переключение зон
- [ ] Протестировать WebSocket подписки
- [ ] Протестировать фильтрацию узлов

---

## Переменные окружения

**Файл:** `server/frontend/.env`

```env
# API endpoint
VITE_API_URL=http://localhost:8000/api

# WebSocket
VITE_WEBSOCKET_KEY=hydro-key
VITE_WEBSOCKET_HOST=localhost
VITE_WEBSOCKET_PORT=6001
VITE_WEBSOCKET_FORCE_TLS=false
```

---

## Тестирование

### Сценарий 1: Переключение зон
1. Открыть UI
2. Выбрать Zone 1 из селектора
3. Проверить что отображаются только узлы Zone 1
4. Переключить на Zone 2
5. Проверить что отображаются только узлы Zone 2

### Сценарий 2: Health monitoring
1. Открыть UI
2. Проверить health badge зоны
3. Отключить Root Node
4. Через 30 секунд проверить что badge стал "Critical"

### Сценарий 3: Real-time updates
1. Открыть UI с выбранной зоной
2. Включить новый узел в этой зоне
3. Проверить что узел появился в UI без перезагрузки
4. Проверить что статистика обновилась

---

**Завершение:** Все шаблоны готовы для реализации мультизонного frontend!


