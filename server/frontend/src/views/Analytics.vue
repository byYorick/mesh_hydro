<template>
  <div>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3 mb-4">Аналитика и статистика</h1>
      </v-col>
    </v-row>

    <!-- Node selector -->
    <v-row>
      <v-col cols="12" md="6">
        <v-select
          v-model="selectedNode"
          :items="nodesStore.nodes"
          item-title="node_id"
          item-value="node_id"
          label="Выберите узел"
          variant="outlined"
          prepend-inner-icon="mdi-access-point"
        >
          <template v-slot:item="{ props, item }">
            <v-list-item v-bind="props">
              <template v-slot:prepend>
                <v-icon :icon="getNodeIcon(item.raw.node_type)"></v-icon>
              </template>
            </v-list-item>
          </template>
        </v-select>
      </v-col>

      <v-col cols="12" md="6">
        <v-select
          v-model="selectedPeriod"
          :items="periodOptions"
          label="Период"
          variant="outlined"
        ></v-select>
      </v-col>
    </v-row>

    <div v-if="selectedNode">
      <!-- Statistics Cards -->
      <v-row>
        <v-col cols="12" sm="6" md="3">
          <v-card color="primary" variant="tonal">
            <v-card-text class="text-center">
              <v-icon icon="mdi-counter" size="48"></v-icon>
              <div class="text-h4 font-weight-bold mt-2">
                {{ stats.dataPoints || 0 }}
              </div>
              <div class="text-subtitle-1">Точек данных</div>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" sm="6" md="3">
          <v-card color="success" variant="tonal">
            <v-card-text class="text-center">
              <v-icon icon="mdi-percent" size="48"></v-icon>
              <div class="text-h4 font-weight-bold mt-2">
                {{ stats.uptime || '0%' }}
              </div>
              <div class="text-subtitle-1">Uptime</div>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" sm="6" md="3">
          <v-card color="info" variant="tonal">
            <v-card-text class="text-center">
              <v-icon icon="mdi-clock-fast" size="48"></v-icon>
              <div class="text-h4 font-weight-bold mt-2">
                {{ stats.avgInterval || '-' }}
              </div>
              <div class="text-subtitle-1">Средний интервал (сек)</div>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" sm="6" md="3">
          <v-card color="warning" variant="tonal">
            <v-card-text class="text-center">
              <v-icon icon="mdi-alert" size="48"></v-icon>
              <div class="text-h4 font-weight-bold mt-2">
                {{ stats.anomalies || 0 }}
              </div>
              <div class="text-subtitle-1">Аномалий</div>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>

      <!-- Charts Grid -->
      <v-row>
        <v-col
          v-for="field in dataFields"
          :key="field.key"
          cols="12"
          md="6"
        >
          <AdvancedChart
            :title="`${field.label} - ${selectedNode}`"
            :icon="field.icon"
            :data="telemetryData"
            :fields="[field.key]"
            :y-axis-label="field.unit"
            :loading="loading"
            @refresh="loadData"
            @range-changed="onRangeChanged"
          />
        </v-col>
      </v-row>

      <!-- Correlation Matrix (if multiple fields) -->
      <v-row v-if="dataFields.length > 1">
        <v-col cols="12">
          <v-card>
            <v-card-title>
              <v-icon icon="mdi-grid" class="mr-2"></v-icon>
              Корреляция параметров
            </v-card-title>
            <v-card-text>
              <v-simple-table>
                <template v-slot:default>
                  <thead>
                    <tr>
                      <th></th>
                      <th v-for="field in dataFields" :key="field.key">
                        {{ field.label }}
                      </th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr v-for="field1 in dataFields" :key="field1.key">
                      <td><strong>{{ field1.label }}</strong></td>
                      <td v-for="field2 in dataFields" :key="field2.key">
                        <v-chip
                          :color="getCorrelationColor(field1.key, field2.key)"
                          size="small"
                        >
                          {{ calculateCorrelation(field1.key, field2.key) }}
                        </v-chip>
                      </td>
                    </tr>
                  </tbody>
                </template>
              </v-simple-table>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>

      <!-- Data Export -->
      <v-row>
        <v-col cols="12">
          <v-card>
            <v-card-title>
              <v-icon icon="mdi-download" class="mr-2"></v-icon>
              Экспорт данных
            </v-card-title>
            <v-card-text>
              <v-row>
                <v-col cols="12" md="6">
                  <v-btn
                    block
                    color="primary"
                    prepend-icon="mdi-file-delimited"
                    @click="exportCsv"
                    :loading="exporting"
                  >
                    Экспорт CSV
                  </v-btn>
                </v-col>
                <v-col cols="12" md="6">
                  <v-btn
                    block
                    color="secondary"
                    prepend-icon="mdi-code-json"
                    @click="exportJson"
                    :loading="exporting"
                  >
                    Экспорт JSON
                  </v-btn>
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>
    </div>

    <!-- Empty state -->
    <v-row v-else>
      <v-col cols="12">
        <v-card>
          <v-card-text class="text-center pa-12">
            <v-icon icon="mdi-chart-areaspline" size="80" color="grey"></v-icon>
            <div class="text-h5 mt-4 text-disabled">
              Выберите узел для просмотра аналитики
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { ref, computed, watch, onMounted } from 'vue'
import { useNodesStore } from '@/stores/nodes'
import { useTelemetryStore } from '@/stores/telemetry'
import { useAppStore } from '@/stores/app'
import AdvancedChart from '@/components/AdvancedChart.vue'

const nodesStore = useNodesStore()
const telemetryStore = useTelemetryStore()
const appStore = useAppStore()

const selectedNode = ref(null)
const selectedPeriod = ref('24h')
const telemetryData = ref([])
const stats = ref({})
const loading = ref(false)
const exporting = ref(false)

const periodOptions = [
  { title: '1 час', value: '1h' },
  { title: '6 часов', value: '6h' },
  { title: '24 часа', value: '24h' },
  { title: '7 дней', value: '7d' },
  { title: '30 дней', value: '30d' },
]

const dataFields = computed(() => {
  if (!selectedNode.value) return []
  
  const node = nodesStore.getNodeById(selectedNode.value)
  if (!node) return []
  
  switch (node.node_type) {
    case 'ph_ec':
      return [
        { key: 'ph', label: 'pH', unit: 'pH', icon: 'mdi-flask' },
        { key: 'ec', label: 'EC', unit: 'mS/cm', icon: 'mdi-flash' },
        { key: 'temp', label: 'Температура', unit: '°C', icon: 'mdi-thermometer' },
      ]
    case 'climate':
      return [
        { key: 'temp', label: 'Температура', unit: '°C', icon: 'mdi-thermometer' },
        { key: 'humidity', label: 'Влажность', unit: '%', icon: 'mdi-water-percent' },
        { key: 'co2', label: 'CO₂', unit: 'ppm', icon: 'mdi-molecule-co2' },
        { key: 'light', label: 'Освещенность', unit: 'lux', icon: 'mdi-white-balance-sunny' },
      ]
    case 'water':
      return [
        { key: 'level', label: 'Уровень', unit: '%', icon: 'mdi-waves' },
        { key: 'temp', label: 'Температура', unit: '°C', icon: 'mdi-thermometer' },
      ]
    default:
      return []
  }
})

watch([selectedNode, selectedPeriod], () => {
  if (selectedNode.value) {
    loadData()
  }
})

onMounted(() => {
  if (nodesStore.nodes.length > 0) {
    selectedNode.value = nodesStore.nodes[0].node_id
  }
})

async function loadData() {
  loading.value = true
  
  try {
    const hours = getHoursFromPeriod(selectedPeriod.value)
    
    const data = await telemetryStore.fetchTelemetry({
      node_id: selectedNode.value,
      hours,
    })
    
    telemetryData.value = data
    calculateStats()
  } catch (error) {
    appStore.showSnackbar('Ошибка загрузки данных', 'error')
  } finally {
    loading.value = false
  }
}

function calculateStats() {
  if (!telemetryData.value || telemetryData.value.length === 0) {
    stats.value = {}
    return
  }

  stats.value = {
    dataPoints: telemetryData.value.length,
    uptime: '98.5%', // TODO: calculate real uptime
    avgInterval: 30, // TODO: calculate from timestamps
    anomalies: 2, // TODO: detect anomalies
  }
}

function calculateCorrelation(field1, field2) {
  if (field1 === field2) return '1.00'
  // TODO: implement real correlation calculation
  return '0.75'
}

function getCorrelationColor(field1, field2) {
  const corr = parseFloat(calculateCorrelation(field1, field2))
  if (corr > 0.8) return 'success'
  if (corr > 0.5) return 'info'
  if (corr > 0.2) return 'warning'
  return 'grey'
}

function getHoursFromPeriod(period) {
  const map = {
    '1h': 1,
    '6h': 6,
    '24h': 24,
    '7d': 168,
    '30d': 720,
  }
  return map[period] || 24
}

function onRangeChanged(range) {
  selectedPeriod.value = range
}

function getNodeIcon(type) {
  const icons = {
    'ph_ec': 'mdi-flask',
    'climate': 'mdi-thermometer',
    'relay': 'mdi-electric-switch',
    'water': 'mdi-water',
    'display': 'mdi-monitor',
    'root': 'mdi-server-network',
  }
  return icons[type] || 'mdi-chip'
}

async function exportCsv() {
  exporting.value = true
  try {
    await telemetryStore.exportTelemetry({
      node_id: selectedNode.value,
      hours: getHoursFromPeriod(selectedPeriod.value),
    })
    appStore.showSnackbar('CSV файл загружен', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка экспорта', 'error')
  } finally {
    exporting.value = false
  }
}

async function exportJson() {
  exporting.value = true
  try {
    const data = {
      node_id: selectedNode.value,
      period: selectedPeriod.value,
      exported_at: new Date().toISOString(),
      data: telemetryData.value,
      statistics: stats.value,
    }
    
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' })
    const url = window.URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = url
    link.download = `analytics_${selectedNode.value}_${Date.now()}.json`
    link.click()
    window.URL.revokeObjectURL(url)
    
    appStore.showSnackbar('JSON файл загружен', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка экспорта', 'error')
  } finally {
    exporting.value = false
  }
}
</script>

