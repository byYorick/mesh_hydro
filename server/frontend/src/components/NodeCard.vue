<template>
  <v-card
    :color="statusColor"
    variant="tonal"
    class="node-card"
  >
    <v-card-title class="d-flex align-center">
      <v-icon :icon="nodeIcon" class="mr-2"></v-icon>
      <span>{{ node.node_id }}</span>
      
      <v-spacer></v-spacer>
      
      <v-chip
        :color="statusColor"
        size="small"
        variant="flat"
      >
        <v-icon :icon="statusIcon" start></v-icon>
        {{ statusText }}
      </v-chip>
    </v-card-title>

    <v-card-subtitle>
      {{ nodeTypeText }} • {{ node.zone || 'Без зоны' }}
    </v-card-subtitle>

    <v-card-text>
      <!-- Node-specific data -->
      <div v-if="node.node_type === 'ph_ec' && lastData">
        <v-row dense>
          <v-col cols="6">
            <div class="text-h4 font-weight-bold">
              {{ lastData.ph?.toFixed(2) || '-' }}
            </div>
            <div class="text-caption">pH</div>
          </v-col>
          <v-col cols="6">
            <div class="text-h4 font-weight-bold">
              {{ lastData.ec?.toFixed(2) || '-' }}
            </div>
            <div class="text-caption">EC (mS/cm)</div>
          </v-col>
        </v-row>
      </div>

      <div v-else-if="node.node_type === 'climate' && lastData">
        <v-row dense>
          <v-col cols="4">
            <div class="text-h5">{{ lastData.temp?.toFixed(1) || '-' }}°C</div>
            <div class="text-caption">Температура</div>
          </v-col>
          <v-col cols="4">
            <div class="text-h5">{{ lastData.humidity?.toFixed(0) || '-' }}%</div>
            <div class="text-caption">Влажность</div>
          </v-col>
          <v-col cols="4">
            <div class="text-h5">{{ lastData.co2 || '-' }}</div>
            <div class="text-caption">CO₂ ppm</div>
          </v-col>
        </v-row>
      </div>

      <div v-else-if="node.node_type === 'water' && lastData">
        <v-row dense>
          <v-col cols="6">
            <div class="text-h5">{{ lastData.level?.toFixed(0) || '-' }}%</div>
            <div class="text-caption">Уровень</div>
          </v-col>
          <v-col cols="6">
            <div class="text-h5">{{ lastData.temp?.toFixed(1) || '-' }}°C</div>
            <div class="text-caption">Температура</div>
          </v-col>
        </v-row>
      </div>

      <div v-else class="text-caption text-disabled">
        Нет данных
      </div>

      <!-- Memory Usage (compact) -->
      <v-divider class="my-2"></v-divider>
      <div v-if="hasMemoryInfo" class="mb-2">
        <div class="d-flex justify-space-between mb-1">
          <span class="text-caption">
            <v-icon icon="mdi-memory" size="x-small" class="mr-1"></v-icon>
            RAM
          </span>
          <span class="text-caption">{{ heapPercent.toFixed(0) }}%</span>
        </div>
        <v-progress-linear
          :model-value="heapPercent"
          :color="getMemoryColor(heapPercent)"
          height="6"
          rounded
        ></v-progress-linear>
      </div>

      <!-- Last seen -->
      <div class="text-caption text-disabled">
        <v-icon icon="mdi-clock-outline" size="small" class="mr-1"></v-icon>
        {{ lastSeenText }}
      </div>
    </v-card-text>

    <v-card-actions>
      <v-btn
        size="small"
        variant="text"
        :to="{ name: 'NodeDetail', params: { nodeId: node.node_id } }"
        prepend-icon="mdi-eye"
      >
        Детали
      </v-btn>

      <v-spacer></v-spacer>

      <!-- Quick actions based on node type -->
      <template v-if="isOnline">
        <!-- pH/EC Quick Actions -->
        <v-menu v-if="node.node_type === 'ph_ec'">
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
          size="small"
          color="primary"
          prepend-icon="mdi-window-open"
          @click="$emit('command', { command: 'open_all', params: {} })"
        >
          Открыть
        </v-btn>

        <!-- Climate Quick Action -->
        <v-btn
          v-if="node.node_type === 'climate'"
          size="small"
          color="primary"
          prepend-icon="mdi-refresh"
          @click="$emit('command', { command: 'update_sensors', params: {} })"
        >
          Обновить
        </v-btn>

        <!-- Command Dialog (all nodes) -->
        <CommandDialog :node="node" @command-sent="handleCommand">
          <template v-slot:activator="{ props: dialogProps }">
            <v-btn
              size="small"
              color="secondary"
              v-bind="dialogProps"
              prepend-icon="mdi-send"
            >
              Команды
            </v-btn>
          </template>
        </CommandDialog>
      </template>

      <v-chip v-else size="small" color="error">
        <v-icon icon="mdi-lan-disconnect" start size="small"></v-icon>
        Offline
      </v-chip>
    </v-card-actions>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'
import { formatDistanceToNow } from '@/utils/time'
import CommandDialog from './CommandDialog.vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['command'])

function quickPump(pump) {
  emit('command', {
    command: 'run_pump',
    params: { pump, duration: 5 },
  })
}

function handleCommand({ command, params }) {
  emit('command', { command, params })
}

// Node online status
const isOnline = computed(() => {
  return props.node.online || props.node.is_online || false
})

// Status color
const statusColor = computed(() => {
  if (!props.node.last_seen_at) return 'grey'
  
  const lastSeen = new Date(props.node.last_seen_at)
  const seconds = (Date.now() - lastSeen.getTime()) / 1000
  
  if (seconds < 30) return 'success'
  if (seconds < 60) return 'warning'
  return 'error'
})

// Status icon
const statusIcon = computed(() => {
  return isOnline.value ? 'mdi-check-circle' : 'mdi-close-circle'
})

// Status text
const statusText = computed(() => {
  return isOnline.value ? 'Online' : 'Offline'
})

// Node type icon
const nodeIcon = computed(() => {
  const icons = {
    'ph_ec': 'mdi-flask',
    'climate': 'mdi-thermometer',
    'relay': 'mdi-electric-switch',
    'water': 'mdi-water',
    'display': 'mdi-monitor',
    'root': 'mdi-server-network',
  }
  return icons[props.node.node_type] || 'mdi-chip'
})

// Node type text
const nodeTypeText = computed(() => {
  const types = {
    'ph_ec': 'pH/EC Сенсор',
    'climate': 'Климат Сенсор',
    'relay': 'Реле',
    'water': 'Уровень Воды',
    'display': 'Дисплей',
    'root': 'Root Узел',
  }
  return types[props.node.node_type] || props.node.node_type
})

// Last telemetry data
const lastData = computed(() => {
  return props.node.last_telemetry?.data || props.node.last_data || null
})

// Last seen text
const lastSeenText = computed(() => {
  if (!props.node.last_seen_at) return 'Никогда'
  return formatDistanceToNow(props.node.last_seen_at)
})

// Memory info
const metadata = computed(() => props.node.metadata || {})

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
</script>

<style scoped>
.node-card {
  height: 100%;
}
</style>

