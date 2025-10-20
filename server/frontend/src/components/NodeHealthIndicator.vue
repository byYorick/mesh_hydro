<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon :icon="healthIcon" :color="healthColor" class="mr-2"></v-icon>
      Health Score
      
      <v-spacer></v-spacer>
      
      <v-chip :color="healthColor" variant="flat">
        {{ healthScore }}/100
      </v-chip>
    </v-card-title>

    <v-card-text>
      <!-- Health Progress -->
      <v-progress-linear
        :model-value="healthScore"
        :color="healthColor"
        height="20"
        rounded
        class="mb-4"
      >
        <template v-slot:default>
          <strong>{{ healthScore }}%</strong>
        </template>
      </v-progress-linear>

      <!-- Health Factors -->
      <v-list density="compact">
        <v-list-item>
          <template v-slot:prepend>
            <v-icon :color="factorColors.errors" size="small">mdi-bug</v-icon>
          </template>
          <v-list-item-title>Ошибки (24ч)</v-list-item-title>
          <v-list-item-subtitle>
            {{ errorCount }} - {{ getFactorLabel(factorScores.errors) }}
          </v-list-item-subtitle>
        </v-list-item>

        <v-list-item>
          <template v-slot:prepend>
            <v-icon :color="factorColors.memory" size="small">mdi-memory</v-icon>
          </template>
          <v-list-item-title>Использование RAM</v-list-item-title>
          <v-list-item-subtitle>
            {{ memoryUsage.toFixed(0) }}% - {{ getFactorLabel(factorScores.memory) }}
          </v-list-item-subtitle>
        </v-list-item>

        <v-list-item>
          <template v-slot:prepend>
            <v-icon :color="factorColors.uptime" size="small">mdi-clock-check</v-icon>
          </template>
          <v-list-item-title>Uptime</v-list-item-title>
          <v-list-item-subtitle>
            {{ formatUptime(uptime) }} - {{ getFactorLabel(factorScores.uptime) }}
          </v-list-item-subtitle>
        </v-list-item>

        <v-list-item>
          <template v-slot:prepend>
            <v-icon :color="factorColors.signal" size="small">mdi-wifi</v-icon>
          </template>
          <v-list-item-title>WiFi сигнал</v-list-item-title>
          <v-list-item-subtitle>
            {{ wifiRssi }} dBm - {{ getFactorLabel(factorScores.signal) }}
          </v-list-item-subtitle>
        </v-list-item>
      </v-list>

      <!-- Overall Status -->
      <v-alert
        :type="healthAlertType"
        variant="tonal"
        class="mt-4"
        density="compact"
      >
        <div class="text-subtitle-2">{{ healthMessage }}</div>
      </v-alert>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
  errorCount: {
    type: Number,
    default: 0,
  },
})

// Calculate health score (0-100)
const healthScore = computed(() => {
  let score = 100

  // Errors impact (0-30 points)
  if (props.errorCount > 10) score -= 30
  else if (props.errorCount > 5) score -= 20
  else if (props.errorCount > 2) score -= 10
  else if (props.errorCount > 0) score -= 5

  // Memory impact (0-25 points)
  if (memoryUsage.value > 90) score -= 25
  else if (memoryUsage.value > 80) score -= 15
  else if (memoryUsage.value > 70) score -= 10

  // Uptime impact (0-20 points)
  if (uptime.value < 3600) score -= 20 // Less than 1 hour
  else if (uptime.value < 86400) score -= 10 // Less than 1 day

  // Signal impact (0-25 points)
  if (wifiRssi.value < -80) score -= 25
  else if (wifiRssi.value < -70) score -= 15
  else if (wifiRssi.value < -60) score -= 10

  return Math.max(0, Math.min(100, score))
})

// Individual factor scores
const factorScores = computed(() => ({
  errors: props.errorCount === 0 ? 100 : props.errorCount > 10 ? 0 : 100 - (props.errorCount * 10),
  memory: 100 - memoryUsage.value,
  uptime: uptime.value > 86400 ? 100 : (uptime.value / 86400) * 100,
  signal: ((wifiRssi.value + 90) / 60) * 100, // -30 to -90 dBm
}))

// Factor colors based on scores
const factorColors = computed(() => ({
  errors: factorScores.value.errors > 70 ? 'success' : factorScores.value.errors > 40 ? 'warning' : 'error',
  memory: factorScores.value.memory > 50 ? 'success' : factorScores.value.memory > 25 ? 'warning' : 'error',
  uptime: factorScores.value.uptime > 70 ? 'success' : factorScores.value.uptime > 40 ? 'warning' : 'error',
  signal: factorScores.value.signal > 60 ? 'success' : factorScores.value.signal > 30 ? 'warning' : 'error',
}))

const memoryUsage = computed(() => {
  const metadata = props.node.metadata || {}
  const heapFree = metadata.heap_free || metadata.free_heap || 0
  const heapTotal = metadata.heap_total || metadata.total_heap || 320000
  
  if (heapTotal === 0) return 0
  
  const heapUsed = heapTotal - heapFree
  return (heapUsed / heapTotal) * 100
})

const uptime = computed(() => {
  return props.node.metadata?.uptime || 0
})

const wifiRssi = computed(() => {
  return props.node.metadata?.wifi_rssi || props.node.metadata?.rssi_to_parent || -60
})

const healthColor = computed(() => {
  if (healthScore.value >= 80) return 'success'
  if (healthScore.value >= 60) return 'info'
  if (healthScore.value >= 40) return 'warning'
  return 'error'
})

const healthIcon = computed(() => {
  if (healthScore.value >= 80) return 'mdi-check-circle'
  if (healthScore.value >= 60) return 'mdi-information'
  if (healthScore.value >= 40) return 'mdi-alert'
  return 'mdi-alert-circle'
})

const healthAlertType = computed(() => {
  if (healthScore.value >= 80) return 'success'
  if (healthScore.value >= 60) return 'info'
  if (healthScore.value >= 40) return 'warning'
  return 'error'
})

const healthMessage = computed(() => {
  if (healthScore.value >= 80) return 'Узел работает отлично!'
  if (healthScore.value >= 60) return 'Узел работает нормально'
  if (healthScore.value >= 40) return 'Требуется внимание'
  return 'Критическое состояние узла!'
})

function getFactorLabel(score) {
  if (score >= 80) return 'Отлично'
  if (score >= 60) return 'Хорошо'
  if (score >= 40) return 'Удовлетворительно'
  return 'Плохо'
}

function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400)
  const hours = Math.floor((seconds % 86400) / 3600)
  
  if (days > 0) return `${days}д ${hours}ч`
  if (hours > 0) return `${hours}ч`
  return `${Math.floor(seconds / 60)}м`
}
</script>
