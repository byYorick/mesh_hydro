<template>
  <v-card>
    <v-card-title>
      <v-icon icon="mdi-information-outline" class="mr-2"></v-icon>
      Метаданные узла
    </v-card-title>

    <v-card-text>
      <v-list density="compact">
        <!-- Firmware -->
        <v-list-item v-if="metadata.firmware">
          <template v-slot:prepend>
            <v-icon icon="mdi-chip" color="primary"></v-icon>
          </template>
          <v-list-item-title>Прошивка</v-list-item-title>
          <v-list-item-subtitle>
            {{ metadata.firmware }}
            <v-chip
              v-if="hasUpdate"
              size="x-small"
              color="success"
              class="ml-2"
            >
              Обновление доступно
            </v-chip>
          </v-list-item-subtitle>
        </v-list-item>

        <!-- Hardware -->
        <v-list-item v-if="metadata.hardware">
          <template v-slot:prepend>
            <v-icon icon="mdi-cpu-64-bit" color="info"></v-icon>
          </template>
          <v-list-item-title>Аппаратура</v-list-item-title>
          <v-list-item-subtitle>{{ metadata.hardware }}</v-list-item-subtitle>
        </v-list-item>

        <!-- MAC Address -->
        <v-list-item v-if="node.mac_address || metadata.mac_address || metadata.mac">
          <template v-slot:prepend>
            <v-icon icon="mdi-network-outline" color="primary"></v-icon>
          </template>
          <v-list-item-title>MAC адрес</v-list-item-title>
          <v-list-item-subtitle>
            {{ formatMac(node.mac_address || metadata.mac_address || metadata.mac) }}
          </v-list-item-subtitle>
        </v-list-item>

        <!-- IP Address -->
        <v-list-item v-if="metadata.ip_address">
          <template v-slot:prepend>
            <v-icon icon="mdi-ip-network" color="secondary"></v-icon>
          </template>
          <v-list-item-title>IP адрес</v-list-item-title>
          <v-list-item-subtitle>{{ metadata.ip_address }}</v-list-item-subtitle>
        </v-list-item>

        <!-- WiFi SSID -->
        <v-list-item v-if="metadata.wifi_ssid">
          <template v-slot:prepend>
            <v-icon icon="mdi-wifi" color="info"></v-icon>
          </template>
          <v-list-item-title>WiFi сеть</v-list-item-title>
          <v-list-item-subtitle>{{ metadata.wifi_ssid }}</v-list-item-subtitle>
        </v-list-item>

        <!-- Boot Count -->
        <v-list-item v-if="metadata.boot_count">
          <template v-slot:prepend>
            <v-icon icon="mdi-restart" color="warning"></v-icon>
          </template>
          <v-list-item-title>Перезагрузок</v-list-item-title>
          <v-list-item-subtitle>{{ metadata.boot_count }}</v-list-item-subtitle>
        </v-list-item>

        <!-- Last Calibration (для pH/EC) -->
        <v-list-item v-if="metadata.last_calibration && node.node_type === 'ph_ec'">
          <template v-slot:prepend>
            <v-icon icon="mdi-flask" color="success"></v-icon>
          </template>
          <v-list-item-title>Последняя калибровка</v-list-item-title>
          <v-list-item-subtitle>
            {{ formatDate(metadata.last_calibration) }}
            <v-chip
              v-if="needsCalibration"
              size="x-small"
              color="warning"
              class="ml-2"
            >
              Требуется калибровка
            </v-chip>
          </v-list-item-subtitle>
        </v-list-item>

        <!-- Sensors (для Climate) -->
        <v-list-item v-if="metadata.sensors && Array.isArray(metadata.sensors)">
          <template v-slot:prepend>
            <v-icon icon="mdi-thermometer" color="info"></v-icon>
          </template>
          <v-list-item-title>Датчики</v-list-item-title>
          <v-list-item-subtitle>
            <v-chip
              v-for="sensor in metadata.sensors"
              :key="sensor"
              size="x-small"
              class="mr-1 mt-1"
            >
              {{ sensor }}
            </v-chip>
          </v-list-item-subtitle>
        </v-list-item>

        <!-- Created via -->
        <v-list-item v-if="metadata.created_via">
          <template v-slot:prepend>
            <v-icon icon="mdi-creation" color="grey"></v-icon>
          </template>
          <v-list-item-title>Создан через</v-list-item-title>
          <v-list-item-subtitle>
            {{ formatCreatedVia(metadata.created_via) }}
          </v-list-item-subtitle>
        </v-list-item>

        <!-- Created at -->
        <v-list-item v-if="metadata.created_at">
          <template v-slot:prepend>
            <v-icon icon="mdi-calendar" color="grey"></v-icon>
          </template>
          <v-list-item-title>Дата создания</v-list-item-title>
          <v-list-item-subtitle>{{ formatDateTime(metadata.created_at) }}</v-list-item-subtitle>
        </v-list-item>
      </v-list>

      <!-- Additional metadata (если есть кастомные поля) -->
      <v-expansion-panels v-if="hasAdditionalMetadata" class="mt-3">
        <v-expansion-panel>
          <v-expansion-panel-title>
            <v-icon icon="mdi-code-json" size="small" class="mr-2"></v-icon>
            <span class="text-caption">Дополнительные метаданные</span>
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <pre class="text-caption">{{ JSON.stringify(additionalMetadata, null, 2) }}</pre>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'
import { formatDateTime, formatDate } from '@/utils/time'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const metadata = computed(() => {
  // Создаём копию metadata чтобы не мутировать исходный объект
  const sourceMeta = props.node.metadata || {}
  const meta = { ...sourceMeta }
  
  // Исправляем undefined sensors и capabilities
  if (!Array.isArray(meta.sensors)) {
    meta.sensors = []
  }
  if (!Array.isArray(meta.capabilities)) {
    meta.capabilities = []
  }
  
  return meta
})

// Memory calculations
const heapTotal = computed(() => metadata.value.heap_total || metadata.value.total_heap || 320000)
const heapUsed = computed(() => metadata.value.heap_used || 150000)
const heapPercent = computed(() => (heapUsed.value / heapTotal.value) * 100)

const flashTotal = computed(() => metadata.value.flash_total || metadata.value.flash_size || null)
const flashUsed = computed(() => metadata.value.flash_used || 0)
const flashPercent = computed(() => {
  if (!flashTotal.value) return 0
  return (flashUsed.value / flashTotal.value) * 100
})

const psramTotal = computed(() => metadata.value.psram_total || null)
const psramUsed = computed(() => metadata.value.psram_used || 0)
const psramPercent = computed(() => {
  if (!psramTotal.value) return 0
  return (psramUsed.value / psramTotal.value) * 100
})

const uptime = computed(() => metadata.value.uptime || null)
const cpuFreq = computed(() => metadata.value.cpu_freq || metadata.value.cpu_frequency || null)
const wifiRssi = computed(() => metadata.value.wifi_rssi || metadata.value.rssi || null)

// Check if update available
const hasUpdate = computed(() => {
  const current = metadata.value.firmware
  const latest = metadata.value.latest_firmware
  return latest && current !== latest
})

// Check if calibration needed (30 days)
const needsCalibration = computed(() => {
  if (!metadata.value.last_calibration) return true
  
  const lastCal = new Date(metadata.value.last_calibration)
  const daysSince = (Date.now() - lastCal.getTime()) / (1000 * 60 * 60 * 24)
  
  return daysSince > 30
})

// Additional metadata (exclude known fields)
const knownFields = [
  'firmware', 'hardware', 'ip_address', 'wifi_ssid', 'wifi_rssi',
  'heap_total', 'heap_used', 'free_heap', 'total_heap',
  'flash_total', 'flash_used', 'flash_size',
  'psram_total', 'psram_used',
  'uptime', 'cpu_freq', 'cpu_frequency',
  'boot_count', 'last_calibration', 'sensors',
  'created_via', 'created_at', 'rssi',
]

const additionalMetadata = computed(() => {
  console.log('🔍 NodeMetadataCard: additionalMetadata computed called')
  console.log('🔍 NodeMetadataCard: metadata.value:', metadata.value)
  console.log('🔍 NodeMetadataCard: knownFields:', knownFields)
  
  const additional = {}
  if (!metadata.value || typeof metadata.value !== 'object') {
    console.log('NodeMetadataCard: metadata.value is not valid:', metadata.value)
    return additional
  }
  
  // Проверяем что knownFields определен и является массивом
  if (!knownFields || !Array.isArray(knownFields)) {
    console.warn('NodeMetadataCard: knownFields is not defined or not an array:', knownFields)
    return additional
  }
  
  try {
    console.log('🔍 NodeMetadataCard: Starting forEach loop')
    Object.keys(metadata.value).forEach((key, index) => {
      console.log(`🔍 NodeMetadataCard: Processing key ${index}:`, key)
      console.log(`🔍 NodeMetadataCard: knownFields type:`, typeof knownFields)
      console.log(`🔍 NodeMetadataCard: knownFields isArray:`, Array.isArray(knownFields))
      
      // Дополнительная проверка на undefined/null перед вызовом includes
      if (knownFields && Array.isArray(knownFields) && key && !knownFields.includes(key)) {
        console.log(`🔍 NodeMetadataCard: Adding key to additional:`, key)
        additional[key] = metadata.value[key]
      }
    })
    console.log('🔍 NodeMetadataCard: forEach loop completed')
  } catch (error) {
    console.error('NodeMetadataCard: Error in additionalMetadata computation:', error)
    console.error('NodeMetadataCard: knownFields:', knownFields)
    console.error('NodeMetadataCard: metadata.value:', metadata.value)
  }
  console.log('🔍 NodeMetadataCard: additionalMetadata result:', additional)
  return additional
})

const hasAdditionalMetadata = computed(() => {
  return Object.keys(additionalMetadata.value).length > 0
})

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

function formatCreatedVia(via) {
  const labels = {
    'web_ui': 'Веб-интерфейс',
    'quick_add': 'Быстрое добавление',
    'mqtt': 'MQTT автообнаружение',
    'api': 'API',
    'seeder': 'Database Seeder',
    'heartbeat': 'MQTT Heartbeat',
    'discovery_topic': 'MQTT Discovery',
  }
  return labels[via] || via
}

function formatMac(mac) {
  if (!mac || typeof mac !== 'string') return 'N/A'
  
  // Если уже форматировано с двоеточиями (00:4b:12:37:d5:a4)
  try {
    // Дополнительная проверка на undefined/null перед вызовом includes
    if (mac && typeof mac === 'string' && mac.includes && mac.includes(':')) {
      return mac.toUpperCase()
    }
  } catch (error) {
    console.error('NodeMetadataCard.vue: formatMac - Error in includes (colons):', error)
    console.error('NodeMetadataCard.vue: formatMac - mac:', mac, typeof mac)
  }
  
  // Если без разделителей (004b1237d5a4)
  if (mac.length === 12) {
    return mac.match(/.{1,2}/g).join(':').toUpperCase()
  }
  
  // Если с дефисами (00-4b-12-37-d5-a4)
  try {
    // Дополнительная проверка на undefined/null перед вызовом includes
    if (mac && typeof mac === 'string' && mac.includes && mac.includes('-')) {
      return mac.replace(/-/g, ':').toUpperCase()
    }
  } catch (error) {
    console.error('NodeMetadataCard.vue: formatMac - Error in includes (dashes):', error)
    console.error('NodeMetadataCard.vue: formatMac - mac:', mac, typeof mac)
  }
  
  return mac.toUpperCase()
}
</script>

<style scoped>
pre {
  white-space: pre-wrap;
  word-break: break-all;
  background-color: rgba(0, 0, 0, 0.05);
  padding: 8px;
  border-radius: 4px;
}
</style>

