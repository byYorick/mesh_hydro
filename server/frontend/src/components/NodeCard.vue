<template>
  <v-card
    :class="['node-card-modern', { 'online': isOnline, 'offline': !isOnline }]"
    :elevation="isOnline ? 2 : 0"
    hover
    style="border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity)); transition: all 0.2s ease;"
  >
    <!-- Status indicator bar -->
    <div 
      :class="['status-bar', statusColor]"
      :style="{ height: '4px', width: '100%' }"
    ></div>

    <!-- Header -->
    <v-card-title class="d-flex align-center justify-space-between pa-4 pb-2">
      <div class="d-flex align-center flex-grow-1">
        <div>
          <div class="text-h6 font-weight-medium">{{ node.node_id }}</div>
          <div class="d-flex align-center mt-1">
            <div class="text-caption text-medium-emphasis mr-2">{{ nodeTypeText }}</div>
            <v-chip
              size="x-small"
              :color="statusColor"
              variant="flat"
              class="text-none"
            >
              {{ statusText }}
            </v-chip>
            <v-chip
              v-if="isDeprecatedCombinedNode"
              size="x-small"
              color="warning"
              variant="outlined"
              class="text-none ml-2"
            >
              Устарело
            </v-chip>
          </div>
        </div>
      </div>
      
      <!-- Error indicator -->
      <v-badge
        v-if="errorCount > 0"
        :content="errorCount"
        :color="errorCount > 5 ? 'error' : 'warning'"
        overlap
        offset-x="8"
        offset-y="8"
      >
        <v-btn
          icon
          size="small"
          variant="text"
          :color="errorCount > 5 ? 'error' : 'warning'"
        >
          <v-icon>mdi-alert-circle</v-icon>
        </v-btn>
      </v-badge>
    </v-card-title>

    <v-card-text class="pa-4 pt-2">
      <!-- Main Metrics -->
      <div v-if="lastData" class="main-metrics mb-4">
        <!-- pH/EC Node Metrics -->
        <div v-if="(['ph_ec', 'deprecated_ph_ec', 'ph'].includes(node.node_type)) && lastData" class="metrics-layout">
          <div class="primary-metric">
            <div class="metric-header">
              <v-icon icon="mdi-flask" size="20" class="mr-2" color="primary"></v-icon>
              <span class="text-caption text-medium-emphasis">pH</span>
            </div>
            <div class="metric-value-large">{{ lastData.ph?.toFixed(2) || '-' }}</div>
            <div v-if="lastData.ph_target" class="metric-target text-caption">
              Цель: {{ lastData.ph_target.toFixed(2) }}
            </div>
          </div>
          <div class="secondary-metric">
            <v-icon icon="mdi-thermometer" size="18" class="mr-1" color="orange"></v-icon>
            <span class="text-h6 font-weight-medium">{{ lastData.temp?.toFixed(1) || '-' }}°</span>
          </div>
        </div>

        <!-- EC Node Metrics -->
        <div v-else-if="node.node_type === 'ec' && lastData" class="metrics-layout">
          <div class="primary-metric">
            <div class="metric-header">
              <v-icon icon="mdi-flash" size="20" class="mr-2" color="warning"></v-icon>
              <span class="text-caption text-medium-emphasis">EC</span>
            </div>
            <div class="metric-value-large">{{ lastData.ec?.toFixed(2) || '-' }}</div>
            <div class="text-caption">mS/cm</div>
          </div>
          <div class="secondary-metric">
            <v-icon icon="mdi-thermometer" size="18" class="mr-1" color="orange"></v-icon>
            <span class="text-h6 font-weight-medium">{{ lastData.temp?.toFixed(1) || '-' }}°</span>
          </div>
        </div>

        <!-- Climate Node Metrics -->
        <div v-else-if="node.node_type === 'climate' && lastData" class="metrics-layout-climate">
          <div class="metric-item">
            <v-icon icon="mdi-thermometer" size="18" color="orange"></v-icon>
            <div class="metric-value-small">{{ lastData.temperature?.toFixed(1) || '-' }}°</div>
            <div class="text-caption">Темп</div>
          </div>
          <div class="metric-item">
            <v-icon icon="mdi-water-percent" size="18" color="blue"></v-icon>
            <div class="metric-value-small">{{ lastData.humidity?.toFixed(0) || '-' }}%</div>
            <div class="text-caption">Влаж</div>
          </div>
          <div class="metric-item">
            <v-icon icon="mdi-molecule-co2" size="18" color="green"></v-icon>
            <div class="metric-value-small">{{ lastData.co2 || '-' }}</div>
            <div class="text-caption">CO₂</div>
          </div>
        </div>

        <!-- Water Node Metrics -->
        <div v-else-if="node.node_type === 'water' && lastData" class="metrics-layout">
          <div class="primary-metric">
            <div class="metric-header">
              <v-icon icon="mdi-waves" size="20" class="mr-2" color="blue"></v-icon>
              <span class="text-caption text-medium-emphasis">Уровень</span>
            </div>
            <div class="metric-value-large">{{ lastData.level?.toFixed(0) || '-' }}%</div>
          </div>
          <div class="secondary-metric">
            <v-icon icon="mdi-thermometer" size="18" class="mr-1" color="orange"></v-icon>
            <span class="text-h6 font-weight-medium">{{ lastData.temp?.toFixed(1) || '-' }}°</span>
          </div>
        </div>

        <!-- Mobile: Horizontal scroll -->
        <div v-else-if="mobileLayout && visibleMetrics.length > 0" class="metrics-scroll">
          <v-chip
            v-for="metric in visibleMetrics"
            :key="metric.key"
            class="metric-chip mr-2"
            size="small"
            variant="tonal"
          >
            <v-icon :icon="metric.icon" start size="small"></v-icon>
            {{ metric.value }}
          </v-chip>
        </div>
      </div>

      <!-- No data -->
      <div v-else class="no-data text-center py-6">
        <v-icon icon="mdi-database-off" size="40" color="grey-lighten-1" class="mb-2"></v-icon>
        <div class="text-caption text-disabled">Нет данных</div>
      </div>

      <!-- Footer Info -->
      <v-divider class="my-3"></v-divider>
      
      <div class="footer-info">
        <!-- System stats inline -->
        <div class="d-flex align-center justify-space-between mb-2">
          <div v-if="hasMemoryInfo" class="d-flex align-center">
            <v-icon icon="mdi-memory" size="14" :color="getMemoryColor(heapPercent)" class="mr-1"></v-icon>
            <span class="text-caption">{{ heapPercent.toFixed(0) }}%</span>
          </div>
          
          <div v-if="hasRssi" class="d-flex align-center ml-3">
            <v-icon :icon="rssiIcon" size="14" :color="getRssiColor(rssiPercent)" class="mr-1"></v-icon>
            <span class="text-caption">{{ rssiValue }} dBm</span>
          </div>
          
          <v-spacer></v-spacer>
          
          <div class="text-caption text-disabled">
            <v-icon icon="mdi-clock-outline" size="14" class="mr-1"></v-icon>
            {{ lastSeenText }}
          </div>
        </div>
        
        <!-- Zone -->
        <div v-if="node.zone" class="text-caption text-medium-emphasis">
          <v-icon icon="mdi-map-marker-outline" size="14" class="mr-1"></v-icon>
          {{ node.zone }}
        </div>
      </div>
    </v-card-text>

    <v-card-actions class="pa-3 pt-0">
      <v-btn
        size="small"
        variant="text"
        :to="{ name: 'NodeDetail', params: { nodeId: node.node_id } }"
        class="text-none"
      >
        Детали
        <v-icon icon="mdi-chevron-right" size="small" class="ml-1"></v-icon>
      </v-btn>

      <v-spacer></v-spacer>

      <!-- Quick actions based on node type -->
      <template v-if="isOnline">
        <!-- pH/EC Quick Actions -->
        <v-menu v-if="['ph_ec', 'deprecated_ph_ec', 'ph'].includes(node.node_type)">
          <template v-slot:activator="{ props }">
            <v-btn
              size="small"
              color="primary"
              v-bind="props"
              prepend-icon="mdi-pump"
            >
              Насосы
            </v-btn>
          </template>
          <v-list density="compact">
            <v-list-item @click="quickPump('ph_up')">
              <v-list-item-title>
                <v-icon icon="mdi-arrow-up" size="small" class="mr-1"></v-icon>
                pH Up (5 сек)
              </v-list-item-title>
            </v-list-item>
            <v-list-item @click="quickPump('ph_down')">
              <v-list-item-title>
                <v-icon icon="mdi-arrow-down" size="small" class="mr-1"></v-icon>
                pH Down (5 сек)
              </v-list-item-title>
            </v-list-item>
            <v-list-item @click="quickPump('ec_up')">
              <v-list-item-title>
                <v-icon icon="mdi-flash" size="small" class="mr-1"></v-icon>
                EC Up (5 сек)
              </v-list-item-title>
            </v-list-item>
          </v-list>
        </v-menu>

        <!-- Relay Quick Actions -->
        <v-btn
          v-if="node.node_type === 'relay'"
          size="large"
          color="primary"
          variant="elevated"
          prepend-icon="mdi-window-open"
          @click="$emit('command', { command: 'open_all', params: {} })"
          class="text-none font-weight-medium"
        >
          <div class="d-flex flex-column align-center">
            <span>Открыть</span>
            <span class="text-caption">Все окна</span>
          </div>
        </v-btn>

        <!-- Climate Quick Action -->
        <v-btn
          v-if="node.node_type === 'climate'"
          size="large"
          color="primary"
          variant="elevated"
          prepend-icon="mdi-refresh"
          @click="$emit('command', { command: 'update_sensors', params: {} })"
          class="text-none font-weight-medium"
        >
          <div class="d-flex flex-column align-center">
            <span>Обновить</span>
            <span class="text-caption">Датчики</span>
          </div>
        </v-btn>

        <!-- Get Config Button -->
        <ConfigViewDialog 
          v-model="showConfigDialog" 
          :node-id="node.node_id"
        >
          <template v-slot:activator="{ props: dialogProps }">
            <v-btn
              size="large"
              color="info"
              variant="elevated"
              v-bind="dialogProps"
              prepend-icon="mdi-download"
              class="text-none font-weight-medium"
            >
              <div class="d-flex flex-column align-center">
                <span>Конфиг</span>
                <span class="text-caption">Получить настройки</span>
              </div>
            </v-btn>
          </template>
        </ConfigViewDialog>

      </template>

      <v-chip v-else size="small" color="error">
        <v-icon icon="mdi-lan-disconnect" start size="small"></v-icon>
        Offline
      </v-chip>
    </v-card-actions>
  </v-card>
</template>

<script setup>
import { ref, computed } from 'vue'
import { useNodeStatusV2 } from '@/composables/useNodeStatusV2'
import ConfigViewDialog from './ConfigViewDialog.vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
  mobileLayout: {
    type: Boolean,
    default: false,
  },
  errorCount: {
    type: Number,
    default: 0,
  },
})

const emit = defineEmits(['command'])

function quickPump(pump) {
  emit('command', {
    command: 'run_pump',
    params: { pump, duration: 5 },
  })
}

// Централизованная система статусов
const nodeRef = computed(() => props.node)
const {
  isOnline,
  isPumpRunning,
  statusColor,
  statusIcon,
  statusText,
  lastSeenText,
  canPerformActions,
  canRunPumps
} = useNodeStatusV2(nodeRef)

// Удалены дублирующиеся computed свойства - теперь используются из useNodeStatus

// Node type icon
const normalizedNodeType = computed(() => {
  const type = props.node?.node_type
  if (type === 'deprecated_ph_ec') {
    return 'ph_ec'
  }
  return type
})

const isDeprecatedCombinedNode = computed(() => props.node?.node_type === 'deprecated_ph_ec')

const nodeIcon = computed(() => {
  if (isDeprecatedCombinedNode.value) {
    return 'mdi-flask-off'
  }
  const icons = {
    'ph_ec': 'mdi-flask',
    'ph': 'mdi-flask-outline',
    'ec': 'mdi-flash',
    'climate': 'mdi-thermometer',
    'relay': 'mdi-electric-switch',
    'water': 'mdi-water',
    'display': 'mdi-monitor',
    'root': 'mdi-server-network',
  }
  return icons[normalizedNodeType.value] || 'mdi-chip'
})

// Node type text
const nodeTypeText = computed(() => {
  if (isDeprecatedCombinedNode.value) {
    return 'pH/EC (устарело)'
  }
  const types = {
    'ph_ec': 'pH/EC Сенсор',
    'ph': 'pH Контроллер',
    'ec': 'EC Контроллер',
    'climate': 'Климат Сенсор',
    'relay': 'Реле',
    'water': 'Уровень Воды',
    'display': 'Дисплей',
    'root': 'Root Узел',
  }
  return types[normalizedNodeType.value] || normalizedNodeType.value
})

// Last telemetry data
const lastData = computed(() => {
  if (!props.node) return null
  return props.node.last_telemetry?.data || props.node.last_data || null
})

// Удален дублирующийся lastSeenText - теперь используется из useNodeStatus

// Memory info
const metadata = computed(() => {
  if (!props.node) return {}
  return props.node.metadata || {}
})

const hasMemoryInfo = computed(() => {
  return metadata.value.heap_total || metadata.value.total_heap || metadata.value.heap_used || metadata.value.heap_free
})

const heapFree = computed(() => metadata.value.heap_free || metadata.value.free_heap || 0)
const heapTotal = computed(() => {
  if (metadata.value.heap_total || metadata.value.total_heap) {
    return metadata.value.heap_total || metadata.value.total_heap
  }
  // Оцениваем total по free (предполагаем free ~60%)
  if (heapFree.value > 0) {
    return Math.round(heapFree.value / 0.6)
  }
  return 320000
})
const heapUsed = computed(() => {
  if (metadata.value.heap_used) return metadata.value.heap_used
  return heapTotal.value - heapFree.value
})
const heapPercent = computed(() => (heapUsed.value / heapTotal.value) * 100)

function getMemoryColor(percent) {
  if (percent < 50) return 'success'
  if (percent < 75) return 'warning'
  return 'error'
}

// WiFi Signal (RSSI)
const hasRssi = computed(() => {
  return metadata.value.rssi_to_parent != null || metadata.value.wifi_rssi != null
})

const rssiValue = computed(() => {
  return metadata.value.rssi_to_parent || metadata.value.wifi_rssi || 0
})

// Преобразование RSSI (-100 до -30 dBm) в проценты (0-100%)
const rssiPercent = computed(() => {
  const rssi = rssiValue.value
  if (rssi === 0) return 0
  // -30 dBm = отлично (100%), -90 dBm = плохо (0%)
  const percent = Math.min(100, Math.max(0, (rssi + 90) * (100 / 60)))
  return percent
})

const rssiIcon = computed(() => {
  const percent = rssiPercent.value
  if (percent > 75) return 'mdi-wifi-strength-4'
  if (percent > 50) return 'mdi-wifi-strength-3'
  if (percent > 25) return 'mdi-wifi-strength-2'
  return 'mdi-wifi-strength-1'
})

function getRssiColor(percent) {
  if (percent > 60) return 'success'
  if (percent > 30) return 'warning'
  return 'error'
}

// Простая мемоизация для обработки метрик
const metricsCache = new Map()
const processNodeMetrics = (nodeType, data) => {
  const cacheKey = `${nodeType}-${JSON.stringify(data)}`
  if (metricsCache.has(cacheKey)) {
    return metricsCache.get(cacheKey)
  }
  
  const result = (() => {
    if (!data) return []
    
    const metrics = []
    
    // pH/EC metrics
    if (nodeType === 'ph_ec' || nodeType === 'ph') {
      if (data.ph != null) metrics.push({ key: 'ph', value: `pH ${data.ph.toFixed(2)}`, icon: 'mdi-flask' })
      if (data.temp != null) metrics.push({ key: 'temp', value: `${data.temp.toFixed(1)}°C`, icon: 'mdi-thermometer' })
    }
    
    if (nodeType === 'ec') {
      if (data.ec != null) metrics.push({ key: 'ec', value: `EC ${data.ec.toFixed(2)}`, icon: 'mdi-flash' })
      if (data.temp != null) metrics.push({ key: 'temp', value: `${data.temp.toFixed(1)}°C`, icon: 'mdi-thermometer' })
    }
    
    // Climate metrics
    if (nodeType === 'climate') {
      if (data.temperature != null) metrics.push({ key: 'temp', value: `${data.temperature.toFixed(1)}°C`, icon: 'mdi-thermometer' })
      if (data.humidity != null) metrics.push({ key: 'hum', value: `${data.humidity.toFixed(0)}%`, icon: 'mdi-water-percent' })
      if (data.co2 != null) metrics.push({ key: 'co2', value: `${data.co2} ppm`, icon: 'mdi-molecule-co2' })
    }
    
    // Water metrics
    if (nodeType === 'water') {
      if (data.level != null) metrics.push({ key: 'level', value: `${data.level.toFixed(0)}%`, icon: 'mdi-waves' })
      if (data.temp != null) metrics.push({ key: 'temp', value: `${data.temp.toFixed(1)}°C`, icon: 'mdi-thermometer' })
    }
    
    return metrics
  })()
  
  metricsCache.set(cacheKey, result)
  return result
}

// Visible metrics для mobile layout
const visibleMetrics = computed(() => {
  return processNodeMetrics(normalizedNodeType.value, lastData.value)
})

// Config dialog state
const showConfigDialog = ref(false)
</script>

<style scoped>
.node-card-modern {
  height: 100%;
  border-radius: 8px;
  overflow: hidden;
}

.node-card-modern:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

.status-bar {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  z-index: 1;
}

.status-bar.success {
  background: rgb(var(--v-theme-success));
}

.status-bar.error {
  background: rgb(var(--v-theme-error));
}

.status-bar.warning {
  background: rgb(var(--v-theme-warning));
}

.status-bar.grey {
  background: rgb(var(--v-theme-grey));
}

.status-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  flex-shrink: 0;
}

.status-dot.success {
  background: rgb(var(--v-theme-success));
  box-shadow: 0 0 0 0 rgba(var(--v-theme-success), 0.4);
}

.status-dot.error {
  background: rgb(var(--v-theme-error));
}

.status-dot.warning {
  background: rgb(var(--v-theme-warning));
}

.status-dot.grey {
  background: rgb(var(--v-theme-grey));
}

.status-dot.pulsing {
  animation: statusPulse 2s infinite;
}

@keyframes statusPulse {
  0% {
    box-shadow: 0 0 0 0 rgba(var(--v-theme-success), 0.4);
  }
  70% {
    box-shadow: 0 0 0 8px rgba(var(--v-theme-success), 0);
  }
  100% {
    box-shadow: 0 0 0 0 rgba(var(--v-theme-success), 0);
  }
}

.main-metrics {
  min-height: 100px;
}

.metrics-layout {
  display: flex;
  align-items: flex-start;
  gap: 16px;
}

.primary-metric {
  flex: 1;
}

.metric-header {
  display: flex;
  align-items: center;
  margin-bottom: 8px;
}

.metric-value-large {
  font-size: 2.5rem;
  font-weight: 600;
  line-height: 1;
  margin: 4px 0;
}

.metric-target {
  margin-top: 4px;
  opacity: 0.7;
}

.secondary-metric {
  display: flex;
  align-items: center;
  padding: 8px 12px;
  background: rgba(var(--v-theme-surface), 0.5);
  border-radius: 6px;
  margin-top: 8px;
}

.metrics-layout-climate {
  display: flex;
  gap: 12px;
  justify-content: space-between;
}

.metric-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 12px;
  background: rgba(var(--v-theme-surface), 0.3);
  border-radius: 6px;
}

.metric-value-small {
  font-size: 1.25rem;
  font-weight: 600;
  margin: 4px 0;
}

.metrics-scroll {
  display: flex;
  overflow-x: auto;
  padding: 4px 0;
  gap: 8px;
}

.metric-chip {
  flex-shrink: 0;
}

.no-data {
  opacity: 0.5;
}

.footer-info {
  padding-top: 4px;
}

@media (max-width: 600px) {
  .metrics-layout {
    flex-direction: column;
    gap: 12px;
  }
  
  .secondary-metric {
    margin-top: 0;
  }
  
  .metric-value-large {
    font-size: 2rem;
  }
}
</style>

