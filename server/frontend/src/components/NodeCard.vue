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

      <!-- Last seen -->
      <v-divider class="my-2"></v-divider>
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
      >
        Детали
      </v-btn>

      <v-spacer></v-spacer>

      <!-- Quick actions based on node type -->
      <v-btn
        v-if="node.node_type === 'relay'"
        size="small"
        color="primary"
        @click="$emit('command', 'toggle_relay', { relay: 1 })"
        :disabled="!isOnline"
      >
        <v-icon icon="mdi-power" start></v-icon>
        Переключить
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'
import { formatDistanceToNow } from '@/utils/time'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

defineEmits(['command'])

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
</script>

<style scoped>
.node-card {
  height: 100%;
}
</style>

