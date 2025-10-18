<template>
  <div v-if="node">
    <v-row>
      <v-col cols="12">
        <v-btn
          icon="mdi-arrow-left"
          variant="text"
          @click="$router.back()"
        ></v-btn>
        <h1 class="text-h3 d-inline-block ml-4">{{ node.node_id }}</h1>
        <v-chip
          :color="statusColor"
          class="ml-4"
        >
          {{ node.online ? 'Online' : 'Offline' }}
        </v-chip>
      </v-col>
    </v-row>

    <!-- Node Info -->
    <v-row>
      <v-col cols="12" md="4">
        <v-card>
          <v-card-title>Информация об узле</v-card-title>
          <v-card-text>
            <v-list density="compact">
              <v-list-item>
                <v-list-item-title>Тип</v-list-item-title>
                <v-list-item-subtitle>{{ node.node_type }}</v-list-item-subtitle>
              </v-list-item>
              <v-list-item>
                <v-list-item-title>Зона</v-list-item-title>
                <v-list-item-subtitle>{{ node.zone || 'Не указана' }}</v-list-item-subtitle>
              </v-list-item>
              <v-list-item>
                <v-list-item-title>MAC адрес</v-list-item-title>
                <v-list-item-subtitle>{{ node.mac_address || 'Не указан' }}</v-list-item-subtitle>
              </v-list-item>
              <v-list-item>
                <v-list-item-title>Последняя связь</v-list-item-title>
                <v-list-item-subtitle>{{ formatDateTime(node.last_seen_at) }}</v-list-item-subtitle>
              </v-list-item>
            </v-list>
          </v-card-text>
        </v-card>
      </v-col>

      <!-- Latest Telemetry -->
      <v-col cols="12" md="8">
        <TelemetryChart
          :title="`Телеметрия: ${node.node_id}`"
          :data="telemetryData"
          :fields="telemetryFields"
          @range-changed="onRangeChanged"
        />
      </v-col>
    </v-row>

    <!-- Events for this node -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title>
            <v-icon icon="mdi-alert-circle" class="mr-2"></v-icon>
            События узла
          </v-card-title>
          <v-card-text>
            <v-data-table
              :headers="eventHeaders"
              :items="node.events || []"
              :items-per-page="10"
            >
              <template v-slot:item.level="{ item }">
                <v-chip :color="getLevelColor(item.level)" size="small">
                  {{ item.level }}
                </v-chip>
              </template>
              <template v-slot:item.created_at="{ item }">
                {{ formatDateTime(item.created_at) }}
              </template>
              <template v-slot:item.resolved_at="{ item }">
                <span v-if="item.resolved_at">
                  {{ formatDateTime(item.resolved_at) }}
                </span>
                <v-chip v-else color="warning" size="small">Активно</v-chip>
              </template>
            </v-data-table>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
  
  <div v-else class="text-center pa-8">
    <v-progress-circular indeterminate></v-progress-circular>
    <div class="mt-4">Загрузка данных узла...</div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { useNodesStore } from '@/stores/nodes'
import { useTelemetryStore } from '@/stores/telemetry'
import TelemetryChart from '@/components/TelemetryChart.vue'
import { formatDateTime } from '@/utils/time'

const route = useRoute()
const nodesStore = useNodesStore()
const telemetryStore = useTelemetryStore()

const node = ref(null)
const telemetryData = ref([])
const selectedRange = ref('24h')

const eventHeaders = [
  { title: 'Уровень', key: 'level' },
  { title: 'Сообщение', key: 'message' },
  { title: 'Создано', key: 'created_at' },
  { title: 'Решено', key: 'resolved_at' },
]

const statusColor = computed(() => {
  return node.value?.online ? 'success' : 'error'
})

const telemetryFields = computed(() => {
  if (!node.value) return []
  
  switch (node.value.node_type) {
    case 'ph_ec':
      return ['ph', 'ec', 'temp']
    case 'climate':
      return ['temp', 'humidity', 'co2']
    case 'water':
      return ['level', 'temp']
    default:
      return []
  }
})

onMounted(async () => {
  const nodeId = route.params.nodeId
  node.value = await nodesStore.fetchNode(nodeId)
  await loadTelemetry()
})

async function loadTelemetry() {
  const hours = getHoursFromRange(selectedRange.value)
  telemetryData.value = await telemetryStore.fetchTelemetry({
    node_id: node.value.node_id,
    hours,
  })
}

function onRangeChanged(range) {
  selectedRange.value = range
  loadTelemetry()
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

function getLevelColor(level) {
  const colors = {
    info: 'blue',
    warning: 'orange',
    critical: 'red',
    emergency: 'purple',
  }
  return colors[level] || 'grey'
}
</script>

