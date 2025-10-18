<template>
  <div>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3 mb-4">Телеметрия</h1>
      </v-col>
    </v-row>

    <!-- Filters -->
    <v-row>
      <v-col cols="12" md="4">
        <v-select
          v-model="selectedNode"
          :items="nodesStore.nodes"
          item-title="node_id"
          item-value="node_id"
          label="Выберите узел"
          variant="outlined"
          density="compact"
        ></v-select>
      </v-col>

      <v-col cols="12" md="4">
        <v-select
          v-model="selectedField"
          :items="availableFields"
          label="Поле данных"
          variant="outlined"
          density="compact"
        ></v-select>
      </v-col>

      <v-col cols="12" md="4">
        <v-btn
          block
          color="primary"
          prepend-icon="mdi-download"
          @click="exportData"
        >
          Экспортировать CSV
        </v-btn>
      </v-col>
    </v-row>

    <!-- Chart -->
    <v-row v-if="selectedNode">
      <v-col cols="12">
        <TelemetryChart
          :title="`${selectedNode} - ${selectedField}`"
          :data="chartData"
          :fields="[selectedField]"
          @range-changed="onRangeChanged"
        />
      </v-col>
    </v-row>

    <!-- Data Table -->
    <v-row v-if="selectedNode">
      <v-col cols="12">
        <v-card>
          <v-card-title>Данные телеметрии</v-card-title>
          <v-card-text>
            <v-data-table
              :headers="tableHeaders"
              :items="telemetryStore.telemetry"
              :items-per-page="20"
            >
              <template v-slot:item.data="{ item }">
                {{ formatData(item.data) }}
              </template>
              <template v-slot:item.received_at="{ item }">
                {{ formatDateTime(item.received_at) }}
              </template>
            </v-data-table>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Empty state -->
    <v-row v-else>
      <v-col cols="12">
        <v-card>
          <v-card-text class="text-center pa-8">
            <v-icon icon="mdi-chart-box-outline" size="64" color="grey"></v-icon>
            <div class="mt-4 text-h6 text-disabled">
              Выберите узел для просмотра телеметрии
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { ref, computed, watch } from 'vue'
import { useNodesStore } from '@/stores/nodes'
import { useTelemetryStore } from '@/stores/telemetry'
import { useAppStore } from '@/stores/app'
import TelemetryChart from '@/components/TelemetryChart.vue'
import { formatDateTime } from '@/utils/time'

const nodesStore = useNodesStore()
const telemetryStore = useTelemetryStore()
const appStore = useAppStore()

const selectedNode = ref(null)
const selectedField = ref('value')
const selectedRange = ref('24h')
const chartData = ref([])

const tableHeaders = [
  { title: 'Node ID', key: 'node_id' },
  { title: 'Данные', key: 'data' },
  { title: 'Получено', key: 'received_at' },
]

const availableFields = computed(() => {
  if (!selectedNode.value) return []
  
  const node = nodesStore.getNodeById(selectedNode.value)
  if (!node) return []
  
  switch (node.node_type) {
    case 'ph_ec':
      return ['ph', 'ec', 'temp']
    case 'climate':
      return ['temp', 'humidity', 'co2', 'light']
    case 'water':
      return ['level', 'flow', 'temp']
    default:
      return ['value']
  }
})

watch(selectedNode, async (newNode) => {
  if (newNode) {
    selectedField.value = availableFields.value[0] || 'value'
    await loadTelemetry()
  }
})

watch(selectedField, async () => {
  if (selectedNode.value) {
    await loadTelemetry()
  }
})

async function loadTelemetry() {
  const hours = getHoursFromRange(selectedRange.value)
  
  const data = await telemetryStore.fetchTelemetry({
    node_id: selectedNode.value,
    hours,
  })
  
  chartData.value = data
}

async function onRangeChanged(range) {
  selectedRange.value = range
  await loadTelemetry()
}

function getHoursFromRange(range) {
  const map = {
    '1h': 1,
    '6h': 6,
    '24h': 24,
    '7d': 168,
  }
  return map[range] || 24
}

function formatData(data) {
  if (!data) return 'N/A'
  return Object.entries(data)
    .map(([k, v]) => `${k}: ${typeof v === 'number' ? v.toFixed(2) : v}`)
    .join(', ')
}

async function exportData() {
  try {
    await telemetryStore.exportTelemetry({
      node_id: selectedNode.value,
      hours: getHoursFromRange(selectedRange.value),
    })
    appStore.showSnackbar('Данные экспортированы', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка экспорта', 'error')
  }
}
</script>

