<template>
  <div>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3 mb-4">Dashboard</h1>
      </v-col>
    </v-row>

    <!-- System Status Cards -->
    <v-row>
      <v-col cols="12" sm="6" md="3">
        <v-card color="primary" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ summary?.nodes?.online || 0 }}
            </div>
            <div class="text-subtitle-1">Узлов Online</div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" sm="6" md="3">
        <v-card color="error" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ summary?.nodes?.offline || 0 }}
            </div>
            <div class="text-subtitle-1">Узлов Offline</div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" sm="6" md="3">
        <v-card color="warning" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ summary?.events?.active || 0 }}
            </div>
            <div class="text-subtitle-1">Активных событий</div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" sm="6" md="3">
        <v-card color="error" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ summary?.events?.critical || 0 }}
            </div>
            <div class="text-subtitle-1">Критичных событий</div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Nodes Grid -->
    <v-row>
      <v-col cols="12" md="6">
        <h2 class="text-h5 mb-3">Узлы системы</h2>
      </v-col>
      <v-col cols="12" md="6" class="text-right">
        <AddNodeDialog @node-created="handleNodeCreated">
          <template v-slot:activator="{ props }">
            <v-btn
              color="primary"
              v-bind="props"
              prepend-icon="mdi-plus"
              size="small"
            >
              Добавить узел
            </v-btn>
          </template>
        </AddNodeDialog>
      </v-col>
    </v-row>

    <v-row>

      <v-col
        v-for="node in nodesStore.nodes"
        :key="node.node_id"
        cols="12"
        sm="6"
        md="4"
        lg="3"
      >
        <NodeCard
          :node="node"
          @command="sendCommand(node.node_id, $event)"
        />
      </v-col>

      <v-col v-if="nodesStore.nodes.length === 0" cols="12">
        <v-card>
          <v-card-text class="text-center pa-8">
            <v-icon icon="mdi-access-point-network-off" size="64" color="grey"></v-icon>
            <div class="mt-4 text-h6 text-disabled">Нет доступных узлов</div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Latest Events -->
    <v-row>
      <v-col cols="12" md="6">
        <EventLog
          :events="eventsStore.events"
          :limit="5"
          @resolve="resolveEvent"
          @resolve-all="resolveAllEvents"
          @delete="deleteEvent"
        />
      </v-col>

      <!-- Latest Telemetry -->
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>
            <v-icon icon="mdi-chart-line" class="mr-2"></v-icon>
            Последняя телеметрия
          </v-card-title>
          <v-card-text>
            <v-list v-if="summary?.telemetry?.latest?.length > 0" lines="two">
              <v-list-item
                v-for="item in summary.telemetry.latest"
                :key="item.node_id"
              >
                <v-list-item-title>{{ item.node_id }}</v-list-item-title>
                <v-list-item-subtitle>
                  {{ formatTelemetryData(item.data) }}
                </v-list-item-subtitle>
              </v-list-item>
            </v-list>
            <div v-else class="text-center pa-4 text-disabled">
              Нет данных телеметрии
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useAppStore } from '@/stores/app'
import { useNodesStore } from '@/stores/nodes'
import { useEventsStore } from '@/stores/events'
import NodeCard from '@/components/NodeCard.vue'
import EventLog from '@/components/EventLog.vue'
import AddNodeDialog from '@/components/AddNodeDialog.vue'
import api from '@/services/api'

const appStore = useAppStore()
const nodesStore = useNodesStore()
const eventsStore = useEventsStore()

const summary = ref(null)

onMounted(async () => {
  try {
    summary.value = await appStore.fetchDashboardSummary()
  } catch (error) {
    console.error('Error loading dashboard:', error)
  }
})

// Send command to node
async function sendCommand(nodeId, { command, params }) {
  try {
    await nodesStore.sendCommand(nodeId, command, params)
    appStore.showSnackbar(`Команда "${command}" отправлена`, 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка отправки команды', 'error')
  }
}

// Resolve event
async function resolveEvent(eventId) {
  try {
    await eventsStore.resolveEvent(eventId)
    appStore.showSnackbar('Событие помечено как решенное', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка резолвения события', 'error')
  }
}

// Resolve all active events
async function resolveAllEvents() {
  try {
    const activeIds = eventsStore.activeEvents.map(e => e.id)
    await eventsStore.resolveBulk(activeIds)
    appStore.showSnackbar(`Решено ${activeIds.length} событий`, 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка резолвения событий', 'error')
  }
}

// Delete event
async function deleteEvent(eventId) {
  try {
    await eventsStore.deleteEvent(eventId)
    appStore.showSnackbar('Событие удалено', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка удаления события', 'error')
  }
}

// Format telemetry data for display
function formatTelemetryData(data) {
  if (!data) return 'Нет данных'
  
  const entries = Object.entries(data)
  if (entries.length === 0) return 'Нет данных'
  
  return entries
    .slice(0, 3)
    .map(([key, value]) => `${key}: ${typeof value === 'number' ? value.toFixed(2) : value}`)
    .join(' • ')
}

// Handle node creation
async function handleNodeCreated(nodeData) {
  try {
    await api.createNode(nodeData)
    
    // Reload nodes and summary
    await nodesStore.fetchNodes()
    summary.value = await appStore.fetchDashboardSummary()
    
    appStore.showSnackbar(
      `Узел "${nodeData.node_id}" успешно создан!`,
      'success',
      5000
    )
  } catch (error) {
    appStore.showSnackbar(
      'Ошибка создания узла: ' + (error.response?.data?.message || error.message),
      'error'
    )
  }
}
</script>

