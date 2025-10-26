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
        <NodeManagementCard
          :node="node"
          @node-updated="handleNodeUpdate"
          @node-deleted="handleNodeDelete"
        />
      </v-col>

    <!-- Actions Panel -->
    <v-col cols="12" md="8">
      <!-- pH Node Component -->
      <PhNode v-if="node.node_type === 'ph'" :node="node" />
      
      <!-- EC Node Component -->
      <EcNode v-else-if="node.node_type === 'ec'" :node="node" />
      
      <!-- pH/EC Node Actions (includes manual pump control) -->
      <NodeActions
        v-else-if="node.node_type === 'ph_ec' || node.node_type === 'ph'"
        :node="node"
        @command="sendCommand"
        @config-update="updateConfig"
      />
      
      <!-- Generic Node Actions -->
      <NodeActions
        v-else
        :node="node"
        @command="sendCommand"
        @config-update="updateConfig"
      />
    </v-col>
  </v-row>

    <!-- Configuration Management Section -->
    <v-row v-if="node.online && (node.node_type === 'ph_ec' || node.node_type === 'ph' || node.node_type === 'ec')">
      <v-col cols="12">
        <v-card>
          <v-card-title>
            <v-icon icon="mdi-cog" class="mr-2"></v-icon>
            Управление конфигурацией
          </v-card-title>
          <v-card-text>
            <v-tabs v-model="configTab" bg-color="primary">
              <v-tab value="manual">
                <v-icon icon="mdi-pump" start></v-icon>
                Ручное управление
              </v-tab>
              <v-tab value="calibration">
                <v-icon icon="mdi-medical-bag" start></v-icon>
                Калибровка
              </v-tab>
            </v-tabs>

            <v-window v-model="configTab" class="mt-4">
              <!-- Manual Pump Control -->
              <v-window-item value="manual">
                <ManualPumpControl
                  :node-id="node.node_id"
                  :is-online="node.online"
                  @pump-started="onPumpStart"
                  @pump-stopped="onPumpStop"
                />
              </v-window-item>

              <!-- Calibration -->
              <v-window-item value="calibration">
                <v-row>
                  <v-col cols="12" md="6">
                    <v-card variant="outlined">
                      <v-card-title class="text-subtitle-1">
                        Калибровка насосов
                      </v-card-title>
                      <v-card-text>
                        <p class="mb-4">
                          Калибровка насосов необходима для точного дозирования реагентов.
                          Введите объем жидкости и длительность работы насоса.
                        </p>
                        <v-btn
                          color="primary"
                          block
                          size="large"
                          @click="calibrationDialog = true"
                        >
                          <v-icon icon="mdi-medical-bag" start></v-icon>
                          Открыть калибровку
                        </v-btn>
                      </v-card-text>
                    </v-card>
                  </v-col>
                  <v-col cols="12" md="6">
                    <v-card variant="outlined">
                      <v-card-title class="text-subtitle-1">
                        Информация
                      </v-card-title>
                      <v-card-text>
                        <v-alert type="info" variant="tonal" density="compact">
                          <strong>Советы по калибровке:</strong>
                          <ul class="mt-2 mb-0">
                            <li>Используйте известный объем (10-50 мл)</li>
                            <li>Измерьте время работы насоса (5-15 сек)</li>
                            <li>Соберите жидкость в мерный стакан</li>
                            <li>Введите фактический объем для расчета производительности</li>
                          </ul>
                        </v-alert>
                      </v-card-text>
                    </v-card>
                  </v-col>
                </v-row>
              </v-window-item>
            </v-window>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Calibration Dialog -->
    <PumpCalibrationDialog
      v-model="calibrationDialog"
      :node-id="node.node_id"
      @calibrated="onCalibrated"
    />

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
import { useAppStore } from '@/stores/app'
import { useNodeStatus } from '@/composables/useNodeStatus'
import AdvancedChart from '@/components/AdvancedChart.vue'
import NodeActions from '@/components/NodeActions.vue'
import NodeManagementCard from '@/components/NodeManagementCard.vue'
import NodeMemoryCard from '@/components/NodeMemoryCard.vue'
import NodeMetadataCard from '@/components/NodeMetadataCard.vue'
import NodeHealthIndicator from '@/components/NodeHealthIndicator.vue'
import ErrorTimeline from '@/components/ErrorTimeline.vue'
import ErrorDetailsDialog from '@/components/ErrorDetailsDialog.vue'
import PhNode from '@/components/PhNode.vue'
import EcNode from '@/components/EcNode.vue'
import ManualPumpControl from '@/components/node-config/ManualPumpControl.vue'
import PumpCalibrationDialog from '@/components/node-config/PumpCalibrationDialog.vue'
import { formatDateTime } from '@/utils/time'
import api from '@/services/api'

const route = useRoute()
const router = useRouter()
const nodesStore = useNodesStore()
const telemetryStore = useTelemetryStore()
const errorsStore = useErrorsStore()
const appStore = useAppStore()

const node = ref(null)
const telemetryData = ref([])
const nodeErrors = ref([])
const selectedRange = ref('24h')
const loading = ref(false)
const selectedError = ref(null)
const showErrorDialog = ref(false)
const configTab = ref('manual')
const calibrationDialog = ref(false)

const eventHeaders = [
  { title: 'Уровень', key: 'level' },
  { title: 'Сообщение', key: 'message' },
  { title: 'Создано', key: 'created_at' },
  { title: 'Решено', key: 'resolved_at' },
]

// Централизованная система статусов
const {
  isOnline,
  isPumpRunning,
  statusColor,
  statusIcon,
  statusText,
  lastSeenText,
  canPerformActions,
  canRunPumps
} = useNodeStatus(node)

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
  await loadNodeErrors()
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
    // Специальная обработка для команды run_pump
    if (command === 'run_pump') {
      // Определяем pump_id на основе типа насоса
      const pumpIdMap = {
        'ph_up': 0,
        'ph_down': 1,
        'ec_up': 2,
        'ec_down': 3,
        'water': 4
      }
      
      const pumpId = pumpIdMap[params.pump] || 0
      
      // Отправляем запрос на запуск насоса
      await api.post(`/nodes/${node.value.node_id}/pump/run`, {
        pump_id: pumpId,
        duration_sec: params.duration
      })
      
      appStore.showSnackbar(`Насос ${params.pump} запущен на ${params.duration} сек`, 'success')
    } else {
      // Обычные команды через стандартный API
      await nodesStore.sendCommand(node.value.node_id, command, params)
      appStore.showSnackbar(`Команда "${command}" отправлена`, 'success')
    }
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

async function handleNodeUpdate(updateData) {
  try {
    // Update node via API
    await api.updateNodeConfig(node.value.node_id, {
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
function onPumpStart(event) {
  console.log('Pump started:', event)
  appStore.showSnackbar(`Насос ${event.pump_id} запущен на ${event.duration} сек`, 'success')
}

function onPumpStop() {
  console.log('Pump stopped')
  appStore.showSnackbar('Насос остановлен', 'info')
}

// Calibration event handler
function onCalibrated(result) {
  console.log('Pump calibrated:', result)
  appStore.showSnackbar(`Насос откалиброван: ${result.ml_per_second?.toFixed(2)} мл/сек`, 'success')
}
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

