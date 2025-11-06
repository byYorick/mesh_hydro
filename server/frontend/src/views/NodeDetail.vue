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

    <!-- Node Info and Node-specific Details -->
    <v-row>
      <v-col cols="12" md="4">
        <NodeManagementCard
          :node="node"
          @node-updated="handleNodeUpdate"
          @node-deleted="handleNodeDelete"
        />
      </v-col>

      <!-- Node-specific Details Panel -->
      <v-col cols="12" md="8">
        <!-- pH Node Detail -->
        <PhDetail
          v-if="node.node_type === 'ph'"
          :node="node"
          :telemetry-data="telemetryData"
          :node-errors="nodeErrors"
        />
        
        <!-- EC Node Detail -->
        <EcNode v-else-if="node.node_type === 'ec'" :node="node" />
        
        <!-- Root Node Detail -->
        <RootDetail v-else-if="node.node_type === 'root'" :node="node" />
        
        <!-- Fallback for other types -->
        <v-card v-else>
          <v-card-text>
            <v-alert type="warning">
              Детальный компонент для типа "{{ node.node_type }}" еще не реализован
            </v-alert>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Memory, Metadata and Health -->
    <v-row>
      <v-col cols="12" md="4">
        <NodeMemoryCard
          :node="node"
          :loading="loading"
          @refresh="loadTelemetry"
        />
      </v-col>
      <v-col cols="12" md="4">
        <NodeHealthIndicator
          :node="node"
          :error-count="nodeErrors.length"
        />
      </v-col>
      <v-col cols="12" md="4">
        <NodeMetadataCard :node="node" />
      </v-col>
    </v-row>

    <!-- Advanced Chart -->
    <v-row>
      <v-col cols="12">
        <AdvancedChart
          :title="`Телеметрия: ${node.node_id}`"
          :icon="getNodeIcon(node.node_type)"
          :data="telemetryData"
          :fields="telemetryFields"
          :loading="loading"
          @range-changed="onRangeChanged"
          @refresh="loadTelemetry"
        />
      </v-col>
    </v-row>

    <!-- Errors Timeline -->
    <v-row>
      <v-col cols="12">
        <ErrorTimeline
          :errors="nodeErrors"
          @view-details="viewErrorDetails"
          @resolve="resolveError"
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
              :items="nodeEvents"
              :items-per-page="10"
            >
              <template v-slot:item.level="{ item }">
                <v-chip :color="getLevelColor(item.level)" size="small">
                  {{ item.level }}
                </v-chip>
              </template>
              <template v-slot:item.message="{ item }">
                <div>
                  <div>{{ item.message }}</div>
                  <div v-if="item.data && hasEventMetadata(item.data)" class="mt-2">
                    <v-chip-group>
                      <!-- pH метаданные -->
                      <v-chip v-if="getPhValue(item.data) !== null" size="x-small" variant="outlined">
                        pH: {{ getPhValue(item.data).toFixed(2) }}
                        <span v-if="getTargetValue(item.data) !== null"> (target: {{ getTargetValue(item.data).toFixed(2) }})</span>
                      </v-chip>
                      <!-- Zone -->
                      <v-chip v-if="item.data.zone || item.data.pid_data?.zone" size="x-small" variant="outlined">
                        Zone: {{ item.data.zone || item.data.pid_data?.zone || 'N/A' }}
                      </v-chip>
                      <!-- Error -->
                      <v-chip v-if="getErrorValue(item.data) !== null" size="x-small" variant="outlined">
                        Error: {{ getErrorValue(item.data).toFixed(3) }}
                      </v-chip>
                      <!-- PID параметры -->
                      <v-chip v-if="item.data.pid_up || item.data.pid_down || item.data.pid_data" size="x-small" variant="outlined">
                        PID: {{ formatPidParams(item.data) }}
                      </v-chip>
                      <!-- Emergency mode -->
                      <v-chip v-if="item.data.emergency_mode" size="x-small" color="error" variant="outlined">
                        Emergency
                      </v-chip>
                    </v-chip-group>
                  </div>
                </div>
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

    <!-- Error Details Dialog -->
    <ErrorDetailsDialog
      v-model="showErrorDialog"
      :error="selectedError"
      @error-resolved="handleErrorResolved"
    />
  </div>
  
  <div v-else class="text-center pa-8">
    <v-progress-circular indeterminate></v-progress-circular>
    <div class="mt-4">Загрузка данных узла...</div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useNodesStore } from '@/stores/nodes'
import { useTelemetryStore } from '@/stores/telemetry'
import { useErrorsStore } from '@/stores/errors'
import { useEventsStore } from '@/stores/events'
import { useAppStore } from '@/stores/app'
import { useNodeStatusV2 } from '@/composables/useNodeStatusV2'
import AdvancedChart from '@/components/AdvancedChart.vue'
import NodeManagementCard from '@/components/NodeManagementCard.vue'
import NodeMemoryCard from '@/components/NodeMemoryCard.vue'
import NodeMetadataCard from '@/components/NodeMetadataCard.vue'
import NodeHealthIndicator from '@/components/NodeHealthIndicator.vue'
import ErrorTimeline from '@/components/ErrorTimeline.vue'
import ErrorDetailsDialog from '@/components/ErrorDetailsDialog.vue'
import PhNode from '@/components/PhNode.vue'
import EcNode from '@/components/EcNode.vue'
import PhDetail from '@/components/detail/PhDetail.vue'
import RootDetail from '@/components/detail/RootDetail.vue'
import { formatDateTime } from '@/utils/time'
import api from '@/services/api'

const route = useRoute()
const router = useRouter()
const nodesStore = useNodesStore()
const telemetryStore = useTelemetryStore()
const errorsStore = useErrorsStore()
const eventsStore = useEventsStore()
const appStore = useAppStore()

const node = ref(null)
const telemetryData = ref([])
const nodeErrors = ref([])
const selectedRange = ref('24h')
const loading = ref(false)
const selectedError = ref(null)
const showErrorDialog = ref(false)

const eventHeaders = [
  { title: 'Уровень', key: 'level' },
  { title: 'Сообщение', key: 'message' },
  { title: 'Создано', key: 'created_at' },
  { title: 'Решено', key: 'resolved_at' },
]

// Централизованная система статусов
const nodeRef = computed(() => node.value)
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

const telemetryFields = computed(() => {
  if (!node.value) return []
  
  switch (node.value.node_type) {
    case 'ph_ec':
      return ['ph', 'ec', 'temp']
    case 'ph':
      return ['ph', 'temp']
    case 'ec':
      return ['ec', 'temp']
    case 'climate':
      return ['temp', 'humidity', 'co2']
    case 'water':
      return ['level', 'temp']
    default:
      return []
  }
})

// События узла из eventsStore (обновляются в реальном времени через WebSocket)
const nodeEvents = computed(() => {
  if (!node.value?.node_id) return []
  
  const nodeId = node.value.node_id
  
  // ВАЖНО: Обращаемся напрямую к eventsStore.events для реактивности
  // Computed property автоматически отслеживает изменения в eventsStore.events
  const allEvents = eventsStore.events
    .filter(e => e && e.node_id === nodeId)
    .slice() // Создаем копию для сортировки
  
  // Сортируем по дате создания (новые сверху)
  allEvents.sort((a, b) => {
    const dateA = new Date(a.created_at || 0).getTime()
    const dateB = new Date(b.created_at || 0).getTime()
    return dateB - dateA
  })
  
  return allEvents
})

async function loadNodeErrors() {
  if (!node.value?.node_id) {
    console.warn('Node not loaded, skipping error loading')
    return
  }
  
  try {
    const result = await errorsStore.fetchNodeErrors(node.value.node_id, {
      hours: 168, // Last 7 days
    })
    nodeErrors.value = Array.isArray(result) ? result : []
  } catch (error) {
    console.error('Error loading node errors:', error)
    nodeErrors.value = []
  }
}

function viewErrorDetails(error) {
  selectedError.value = error
  showErrorDialog.value = true
}

async function resolveError(errorId) {
  try {
    await errorsStore.resolveError(errorId, 'manual')
    appStore.showSnackbar('Ошибка решена', 'success')
    await loadNodeErrors()
  } catch (error) {
    appStore.showSnackbar('Ошибка при резолвении', 'error')
  }
}

async function handleErrorResolved() {
  await loadNodeErrors()
  showErrorDialog.value = false
}

async function loadTelemetry() {
  if (!node.value?.node_id) {
    console.warn('Node not loaded, skipping telemetry loading')
    return
  }
  
  loading.value = true
  try {
    const hours = getHoursFromRange(selectedRange.value)
    telemetryData.value = await telemetryStore.fetchTelemetry({
      node_id: node.value.node_id,
      hours,
    })
  } finally {
    loading.value = false
  }
}

async function sendCommand({ command, params }) {
  if (!node.value?.node_id) {
    appStore.showSnackbar('Узел не загружен', 'error')
    return
  }
  
  try {
    await nodesStore.sendCommand(node.value.node_id, command, params)
    appStore.showSnackbar(`Команда "${command}" отправлена`, 'success')
  } catch (error) {
    console.error('Error sending command:', error)
    appStore.showSnackbar('Ошибка отправки команды', 'error')
  }
}

async function updateConfig(config) {
  try {
    await nodesStore.updateConfig(node.value.node_id, config)
    node.value.config = config
    appStore.showSnackbar('Конфигурация обновлена', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка обновления конфигурации', 'error')
  }
}

function getNodeIcon(type) {
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
  return icons[type] || 'mdi-chip'
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

function formatPid(pid) {
  if (!pid || typeof pid !== 'object') return 'N/A'
  const setpoint = pid.setpoint !== undefined ? pid.setpoint.toFixed(2) : 'N/A'
  const integral = pid.integral !== undefined ? pid.integral.toFixed(2) : 'N/A'
  const zone = pid.zone || 'N/A'
  return `${setpoint} (${integral}, ${zone})`
}

function hasEventMetadata(data) {
  return data && (
    data.ph !== undefined || 
    data.current_ph !== undefined ||
    data.target !== undefined ||
    data.ph_target !== undefined ||
    data.pid_up !== undefined ||
    data.pid_down !== undefined ||
    data.pid_data !== undefined ||
    data.zone !== undefined ||
    data.error !== undefined ||
    data.emergency_mode !== undefined
  )
}

function getPhValue(data) {
  return data?.ph ?? data?.current_ph ?? null
}

function getTargetValue(data) {
  return data?.target ?? data?.ph_target ?? null
}

function getErrorValue(data) {
  return data?.error ?? data?.pid_data?.error ?? null
}

function formatPidParams(data) {
  const pidData = data?.pid_data || data?.pid_up || data?.pid_down
  if (!pidData || typeof pidData !== 'object') return 'N/A'
  
  const kp = pidData.kp !== undefined ? pidData.kp.toFixed(4) : 'N/A'
  const ki = pidData.ki !== undefined ? pidData.ki.toFixed(4) : 'N/A'
  const kd = pidData.kd !== undefined ? pidData.kd.toFixed(4) : 'N/A'
  const setpoint = pidData.setpoint !== undefined ? pidData.setpoint.toFixed(2) : 'N/A'
  const output = pidData.output !== undefined ? pidData.output.toFixed(2) : 'N/A'
  
  return `P:${kp} I:${ki} D:${kd} SP:${setpoint} Out:${output}`
}

async function handleNodeUpdate(updateData) {
  try {
    // Update node via nodes store
    await nodesStore.updateConfig(node.value.node_id, {
      ...node.value.config,
      ...updateData.config,
    })
    
    // Update local data
    Object.assign(node.value, updateData)
    
    appStore.showSnackbar('Узел обновлен', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка обновления узла', 'error')
  }
}

async function handleNodeDelete(nodeId) {
  try {
    await nodesStore.deleteNode(nodeId)
    appStore.showSnackbar(`Узел ${nodeId} удален`, 'success')
    
    // Redirect to nodes list
    router.push({ name: 'Nodes' })
  } catch (error) {
    appStore.showSnackbar('Ошибка удаления узла', 'error')
  }
}

// Pump control event handlers
onMounted(async () => {
  const nodeId = route.params.nodeId
  node.value = await nodesStore.fetchNode(nodeId)
  await loadTelemetry()
  await loadNodeErrors()
  
  // Загружаем события для узла, если их еще нет в store
  try {
    // Проверяем, есть ли события для этого узла в store
    const existingEvents = eventsStore.events.filter(e => e.node_id === nodeId)
    if (existingEvents.length === 0) {
      // Загружаем события для конкретного узла через API
      const newEvents = await api.getEvents({ node_id: nodeId })
      
      // Добавляем новые события в store, избегая дубликатов
      const eventsMap = new Map(eventsStore.events.map(e => [e.id, e]))
      newEvents.forEach(e => {
        if (e.id && !eventsMap.has(e.id)) {
          eventsStore.events.push(e)
        }
      })
    }
  } catch (error) {
    console.warn('Failed to load events for node:', error)
  }
})
</script>

<style scoped>
/* Фиксированная высота для вкладок */
.v-tabs {
  border-bottom: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
}

/* Контент вкладок */
.v-window {
  min-height: 400px;
}

/* Карточки внутри вкладок */
.v-card {
  overflow: visible;
}

/* Кнопки управления */
.v-btn.size-large {
  min-height: 56px;
  font-size: 16px;
}


</style>

