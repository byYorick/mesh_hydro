<template>
  <v-card>
    <v-card-title>
      <v-icon icon="mdi-memory" class="mr-2"></v-icon>
      Память и ресурсы
    </v-card-title>

    <v-card-text>
      <!-- RAM (Heap) -->
      <div class="mb-4">
        <div class="d-flex justify-space-between mb-1">
          <span class="text-subtitle-2">
            <v-icon icon="mdi-chip" size="small" class="mr-1"></v-icon>
            Heap (RAM)
          </span>
          <span class="text-caption">
            {{ formatBytes(heapUsed) }} / {{ formatBytes(heapTotal) }}
          </span>
        </div>
        <v-progress-linear
          :model-value="heapPercent"
          :color="getMemoryColor(heapPercent)"
          height="20"
          rounded
        >
          <template v-slot:default>
            <strong class="text-white">{{ heapPercent.toFixed(1) }}%</strong>
          </template>
        </v-progress-linear>
      </div>

      <!-- Flash Memory -->
      <div class="mb-4" v-if="flashTotal">
        <div class="d-flex justify-space-between mb-1">
          <span class="text-subtitle-2">
            <v-icon icon="mdi-sd" size="small" class="mr-1"></v-icon>
            Flash
          </span>
          <span class="text-caption">
            {{ formatBytes(flashUsed) }} / {{ formatBytes(flashTotal) }}
          </span>
        </div>
        <v-progress-linear
          :model-value="flashPercent"
          :color="getMemoryColor(flashPercent)"
          height="20"
          rounded
        >
          <template v-slot:default>
            <strong class="text-white">{{ flashPercent.toFixed(1) }}%</strong>
          </template>
        </v-progress-linear>
      </div>

      <!-- PSRAM (если есть) -->
      <div class="mb-4" v-if="psramTotal">
        <div class="d-flex justify-space-between mb-1">
          <span class="text-subtitle-2">
            <v-icon icon="mdi-expansion-card" size="small" class="mr-1"></v-icon>
            PSRAM
          </span>
          <span class="text-caption">
            {{ formatBytes(psramUsed) }} / {{ formatBytes(psramTotal) }}
          </span>
        </div>
        <v-progress-linear
          :model-value="psramPercent"
          :color="getMemoryColor(psramPercent)"
          height="20"
          rounded
        >
          <template v-slot:default>
            <strong class="text-white">{{ psramPercent.toFixed(1) }}%</strong>
          </template>
        </v-progress-linear>
      </div>

      <v-divider class="my-3"></v-divider>

      <!-- Additional stats -->
      <v-list density="compact">
        <v-list-item v-if="uptime">
          <template v-slot:prepend>
            <v-icon icon="mdi-clock-outline" size="small"></v-icon>
          </template>
          <v-list-item-title>Uptime</v-list-item-title>
          <v-list-item-subtitle>{{ formatUptime(uptime) }}</v-list-item-subtitle>
        </v-list-item>

        <v-list-item v-if="freeHeap">
          <template v-slot:prepend>
            <v-icon icon="mdi-memory" size="small"></v-icon>
          </template>
          <v-list-item-title>Свободная память</v-list-item-title>
          <v-list-item-subtitle>{{ formatBytes(freeHeap) }}</v-list-item-subtitle>
        </v-list-item>

        <v-list-item v-if="cpuFreq">
          <template v-slot:prepend>
            <v-icon icon="mdi-speedometer" size="small"></v-icon>
          </template>
          <v-list-item-title>Частота CPU</v-list-item-title>
          <v-list-item-subtitle>{{ cpuFreq }} MHz</v-list-item-subtitle>
        </v-list-item>

        <v-list-item v-if="wifiRssi">
          <template v-slot:prepend>
            <v-icon :icon="getWifiIcon(wifiRssi)" :color="getWifiColor(wifiRssi)" size="small"></v-icon>
          </template>
          <v-list-item-title>WiFi сигнал</v-list-item-title>
          <v-list-item-subtitle>{{ wifiRssi }} dBm ({{ getWifiQuality(wifiRssi) }})</v-list-item-subtitle>
        </v-list-item>
      </v-list>

      <!-- Memory warning -->
      <v-alert
        v-if="heapPercent > 80"
        type="warning"
        variant="tonal"
        density="compact"
        class="mt-3"
      >
        <small>⚠️ Высокое использование памяти! Рассмотрите перезагрузку узла.</small>
      </v-alert>
    </v-card-text>

    <v-card-actions>
      <v-btn
        size="small"
        variant="text"
        prepend-icon="mdi-refresh"
        @click="$emit('refresh')"
        :loading="loading"
      >
        Обновить
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
  loading: {
    type: Boolean,
    default: false,
  },
})

defineEmits(['refresh'])

// Extract memory data from metadata
const metadata = computed(() => props.node.metadata || {})

// Heap (RAM)
// Поддержка разных форматов данных от ESP32
const heapFree = computed(() => metadata.value.heap_free || metadata.value.free_heap || 0)
const heapMin = computed(() => metadata.value.heap_min || 0)
const heapTotal = computed(() => {
  // Если есть явно указанный total - используем его
  if (metadata.value.heap_total || metadata.value.total_heap) {
    return metadata.value.heap_total || metadata.value.total_heap
  }
  // Иначе оцениваем по heap_free (предполагаем что свободно ~60%)
  if (heapFree.value > 0) {
    return Math.round(heapFree.value / 0.6) // Если free ~60%, total примерно в 1.67 раз больше
  }
  return 320000 // По умолчанию для ESP32
})
const heapUsed = computed(() => {
  if (metadata.value.heap_used) return metadata.value.heap_used
  // Вычисляем: used = total - free
  return heapTotal.value - heapFree.value
})
const freeHeap = computed(() => heapFree.value)
const heapPercent = computed(() => {
  if (!heapTotal.value) return 0
  return ((heapUsed.value / heapTotal.value) * 100)
})

// Flash
const flashTotal = computed(() => metadata.value.flash_total || metadata.value.flash_size || 4194304)
const flashUsed = computed(() => metadata.value.flash_used || 0)
const flashPercent = computed(() => {
  if (!flashTotal.value) return 0
  return ((flashUsed.value / flashTotal.value) * 100)
})

// PSRAM
const psramTotal = computed(() => metadata.value.psram_total || 0)
const psramUsed = computed(() => metadata.value.psram_used || 0)
const psramPercent = computed(() => {
  if (!psramTotal.value) return 0
  return ((psramUsed.value / psramTotal.value) * 100)
})

// Other stats
const uptime = computed(() => metadata.value.uptime || null)
const cpuFreq = computed(() => metadata.value.cpu_freq || metadata.value.cpu_frequency || null)
const wifiRssi = computed(() => metadata.value.wifi_rssi || metadata.value.rssi || null)

function formatBytes(bytes) {
  if (!bytes || bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i]
}

function formatUptime(seconds) {
  if (!seconds) return 'N/A'
  
  const days = Math.floor(seconds / 86400)
  const hours = Math.floor((seconds % 86400) / 3600)
  const minutes = Math.floor((seconds % 3600) / 60)
  
  if (days > 0) return `${days}д ${hours}ч ${minutes}м`
  if (hours > 0) return `${hours}ч ${minutes}м`
  return `${minutes}м`
}

function getMemoryColor(percent) {
  if (percent < 50) return 'success'
  if (percent < 75) return 'warning'
  return 'error'
}

function getWifiIcon(rssi) {
  if (rssi > -50) return 'mdi-wifi-strength-4'
  if (rssi > -60) return 'mdi-wifi-strength-3'
  if (rssi > -70) return 'mdi-wifi-strength-2'
  return 'mdi-wifi-strength-1'
}

function getWifiColor(rssi) {
  if (rssi > -50) return 'success'
  if (rssi > -60) return 'info'
  if (rssi > -70) return 'warning'
  return 'error'
}

function getWifiQuality(rssi) {
  if (rssi > -50) return 'Отлично'
  if (rssi > -60) return 'Хорошо'
  if (rssi > -70) return 'Удовлетворительно'
  return 'Слабо'
}
</script>

