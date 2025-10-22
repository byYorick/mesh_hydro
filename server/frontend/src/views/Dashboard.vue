<template>
  <div>
    <!-- Pull to Refresh Indicator -->
    <PullToRefreshIndicator
      v-if="isMobile"
      :is-pulling="isPulling"
      :is-refreshing="isRefreshing"
      :pull-distance="pullDistance"
    />

    <!-- Hero Section -->
    <v-sheet
      :class="['hero-section rounded-xl mb-6', { 'hero-mobile': isMobile }]"
      :style="heroStyle"
      elevation="0"
    >
      <v-row align="center" :class="{ 'pa-4': isMobile, 'pa-8': !isMobile }">
        <v-col cols="12" md="8">
          <h1 :class="isMobile ? 'text-h4' : 'text-h2'" class="font-weight-bold mb-2">
            Mesh Hydro System
          </h1>
          <p :class="isMobile ? 'text-subtitle-1' : 'text-h6'" class="text-medium-emphasis">
            {{ summary?.nodes?.online || 0 }} из {{ summary?.nodes?.total || 0 }} узлов онлайн
          </p>
        </v-col>
        <v-col cols="12" md="4" class="text-center">
          <v-icon
            :icon="systemStatusIcon"
            :color="systemStatusColor"
            :size="isMobile ? 64 : 80"
            class="pulse-animation"
          ></v-icon>
        </v-col>
      </v-row>
    </v-sheet>

    <!-- System Status Cards -->
    <v-row>
      <v-col :cols="cardCols">
        <StatCard
          icon="mdi-access-point-network"
          :value="summary?.nodes?.online || 0"
          label="Узлов Online"
          color="success"
        />
      </v-col>

      <v-col :cols="cardCols">
        <StatCard
          icon="mdi-access-point-network-off"
          :value="summary?.nodes?.offline || 0"
          label="Узлов Offline"
          color="error"
        />
      </v-col>

      <v-col :cols="cardCols">
        <StatCard
          icon="mdi-alert-circle"
          :value="summary?.events?.active || 0"
          label="Активных событий"
          color="warning"
        />
      </v-col>

      <v-col :cols="cardCols">
        <StatCard
          icon="mdi-fire"
          :value="summary?.events?.critical || 0"
          label="Критичных"
          color="error"
        />
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

    <!-- Nodes Grid with staggered animation -->
    <transition-group name="stagger-fade" tag="div" class="v-row">
      <v-col
        v-for="node in nodesStore.nodes"
        :key="node.node_id"
        :cols="cardCols"
      >
        <NodeCard
          :node="node"
          :mobile-layout="isMobile"
          @command="sendCommand(node.node_id, $event)"
        />
      </v-col>

      <v-col v-if="nodesStore.nodes.length === 0" cols="12" key="empty-state">
        <v-card elevation="0" class="glass-card">
          <v-card-text class="text-center pa-12">
            <v-icon icon="mdi-access-point-network-off" :size="isMobile ? 64 : 80" color="grey"></v-icon>
            <div class="mt-4 text-h6 text-disabled">Нет доступных узлов</div>
            <div class="text-caption text-disabled mt-2">
              Подключите ESP32 устройства для начала работы
            </div>
            <v-btn
              color="primary"
              class="mt-6"
              prepend-icon="mdi-plus"
              href="/doc/START_HERE.md"
              target="_blank"
            >
              Как подключить узлы
            </v-btn>
          </v-card-text>
        </v-card>
      </v-col>
    </transition-group>

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
import { ref, computed, onMounted } from 'vue'
import { useAppStore } from '@/stores/app'
import { useNodesStore } from '@/stores/nodes'
import { useEventsStore } from '@/stores/events'
import { useResponsive } from '@/composables/useResponsive'
import { usePullToRefresh } from '@/composables/usePullToRefresh'
import NodeCard from '@/components/NodeCard.vue'
import EventLog from '@/components/EventLog.vue'
import AddNodeDialog from '@/components/AddNodeDialog.vue'
import StatCard from '@/components/ui/StatCard.vue'
import PullToRefreshIndicator from '@/components/ui/PullToRefreshIndicator.vue'
import api from '@/services/api'

const appStore = useAppStore()
const nodesStore = useNodesStore()
const eventsStore = useEventsStore()
const { isMobile, isTablet, cardCols } = useResponsive()

const summary = ref(null)

// Pull to refresh для mobile
const { isPulling, pullDistance, isRefreshing } = usePullToRefresh(async () => {
  summary.value = await appStore.fetchDashboardSummary()
  await nodesStore.fetchNodes()
  appStore.showSnackbar('Данные обновлены', 'success', 2000)
})

// Hero section styling
const heroStyle = computed(() => {
  const isDark = appStore.theme === 'dark'
  const gradient = isDark
    ? 'linear-gradient(135deg, rgba(66, 165, 245, 0.1), rgba(156, 39, 176, 0.1))'
    : 'linear-gradient(135deg, rgba(33, 150, 243, 0.1), rgba(156, 39, 176, 0.1))'
  
  return {
    background: gradient,
  }
})

// System status indicator
const systemStatusColor = computed(() => {
  const onlinePercent = summary.value?.nodes?.total 
    ? (summary.value.nodes.online / summary.value.nodes.total) * 100 
    : 0
  
  if (onlinePercent >= 80) return 'success'
  if (onlinePercent >= 50) return 'warning'
  return 'error'
})

const systemStatusIcon = computed(() => {
  const onlinePercent = summary.value?.nodes?.total 
    ? (summary.value.nodes.online / summary.value.nodes.total) * 100 
    : 0
  
  if (onlinePercent >= 80) return 'mdi-check-circle'
  if (onlinePercent >= 50) return 'mdi-alert-circle'
  return 'mdi-close-circle'
})

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
      await api.post(`/nodes/${nodeId}/pump/run`, {
        pump_id: pumpId,
        duration_sec: params.duration
      })
      
      appStore.showSnackbar(`Насос ${params.pump} запущен на ${params.duration} сек`, 'success')
    } else {
      // Обычные команды через стандартный API
      await nodesStore.sendCommand(nodeId, command, params)
      appStore.showSnackbar(`Команда "${command}" отправлена`, 'success')
    }
  } catch (error) {
    console.error('Error sending command:', error)
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

