<template>
  <v-container fluid class="dashboard pa-4 pb-12">
    <PullToRefreshIndicator
      v-if="isMobile"
      :is-pulling="isPulling"
      :is-refreshing="isRefreshing"
      :pull-distance="pullDistance"
    />

    <v-row class="gap-y-4">
      <v-col cols="12" lg="8">
        <v-card class="dashboard-summary-card" elevation="0">
          <v-card-text class="pa-6">
            <div class="d-flex flex-column flex-md-row justify-space-between align-start gap-6">
              <div>
                <h1 class="text-h4 font-weight-bold mb-2">Центр управления</h1>
                <p class="text-body-1 text-medium-emphasis mb-4">
                  Обновлено {{ lastUpdated }}
                </p>
                <div class="d-flex flex-wrap gap-3">
                  <v-chip color="primary" variant="tonal" prepend-icon="mdi-access-point-network">
                    Всего узлов: {{ summaryNodes.total }}
                  </v-chip>
                  <v-chip color="success" variant="tonal" prepend-icon="mdi-check-decagram">
                    Онлайн: {{ summaryNodes.online }}
                  </v-chip>
                  <v-chip color="warning" variant="tonal" prepend-icon="mdi-alert-circle">
                    Активные события: {{ summaryEvents.active }}
                  </v-chip>
                </div>
                <div v-if="nodeTypeChips.length" class="d-flex flex-wrap gap-2 mt-4">
                  <v-chip
                    v-for="chip in nodeTypeChips"
                    :key="chip.type"
                    color="info"
                    variant="outlined"
                    size="small"
                    class="text-caption"
                  >
                    {{ chip.label }}
                  </v-chip>
                </div>
              </div>
              <div class="text-right">
                <v-avatar size="80" :color="systemStatusColor" class="elevation-2">
                  <v-icon :icon="systemStatusIcon" size="48" color="white"></v-icon>
                </v-avatar>
                <div class="mt-3">
                  <v-btn
                    class="mt-2"
                    color="primary"
                    variant="tonal"
                    prepend-icon="mdi-refresh"
                    :loading="isRefreshingData"
                    @click="refreshDashboard"
                  >
                    Обновить данные
                  </v-btn>
                </div>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" lg="4">
        <v-card class="system-status-card" elevation="0">
          <v-card-title class="d-flex justify-space-between align-center">
            <span class="text-subtitle-1 font-weight-medium">Интеграции</span>
            <v-chip size="small" color="primary" variant="tonal" prepend-icon="mdi-clock-outline">
              {{ lastUpdatedShort }}
            </v-chip>
          </v-card-title>
          <v-divider></v-divider>
          <v-list density="comfortable">
            <v-list-item
              v-for="item in systemStatusItems"
              :key="item.label"
              :class="['status-item', item.status]"
            >
              <template #prepend>
                <v-avatar color="transparent" size="40">
                  <v-icon :icon="item.icon" :color="item.color"></v-icon>
                </v-avatar>
              </template>
              <v-list-item-title>{{ item.label }}</v-list-item-title>
              <v-list-item-subtitle>{{ item.description }}</v-list-item-subtitle>
              <template #append>
                <v-chip :color="item.color" size="small" variant="tonal">
                  {{ item.statusLabel }}
                </v-chip>
              </template>
            </v-list-item>
          </v-list>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-2">
      <v-col
        v-for="stat in quickStats"
        :key="stat.label"
        cols="12"
        sm="6"
        lg="3"
        xl="2"
        class="d-flex"
      >
        <StatCard
          :icon="stat.icon"
          :value="stat.value"
          :label="stat.label"
          :color="stat.color"
          :subtitle="stat.subtitle"
          class="flex-grow-1"
        />
      </v-col>
    </v-row>

    <v-row class="mt-6">
      <v-col cols="12">
        <v-card class="greenhouse-section" variant="outlined">
          <v-card-title class="d-flex justify-space-between align-center">
            <div>
              <h2 class="text-h5 mb-1">Теплицы</h2>
              <p class="text-body-2 text-medium-emphasis mb-0">
                Обзор активных теплиц, зон и узлов
              </p>
            </div>
            <v-btn
              color="primary"
              variant="text"
              prepend-icon="mdi-open-in-new"
              to="/greenhouses"
            >
              Открыть теплицы
            </v-btn>
          </v-card-title>
          <v-divider></v-divider>
          <v-card-text>
            <div v-if="!greenhouseOverview.length" class="text-center py-6 text-medium-emphasis">
              Нет созданных теплиц. Добавьте первую на вкладке «Теплицы».
            </div>
            <v-slide-group v-else show-arrows class="greenhouse-slider">
              <v-slide-group-item
                v-for="greenhouse in greenhouseOverview"
                :key="greenhouse.id"
              >
                <v-card class="greenhouse-card" variant="tonal">
                  <v-card-text>
                    <div class="d-flex justify-space-between align-center mb-2">
                      <h3 class="text-h6 mb-0">{{ greenhouse.name }}</h3>
                      <v-chip
                        :color="greenhouse.statusColor"
                        variant="tonal"
                        size="small"
                      >
                        {{ greenhouse.statusLabel }}
                      </v-chip>
                    </div>
                    <div class="text-caption text-medium-emphasis mb-4">
                      Последнее обновление: {{ lastUpdatedShort }}
                    </div>
                    <div class="d-flex flex-wrap gap-2">
                      <v-chip size="small" color="primary" variant="outlined" prepend-icon="mdi-map-marker-radius">
                        Зон: {{ greenhouse.zoneCount }}
                      </v-chip>
                      <v-chip size="small" color="success" variant="outlined" prepend-icon="mdi-access-point-network">
                        Узлов: {{ greenhouse.nodeCount }}
                      </v-chip>
                      <v-chip
                        v-if="greenhouse.offlineNodes > 0"
                        size="small"
                        color="error"
                        variant="outlined"
                        prepend-icon="mdi-alert"
                      >
                        Оффлайн: {{ greenhouse.offlineNodes }}
                      </v-chip>
                      <v-chip
                        v-if="greenhouse.climateProfiles > 0"
                        size="small"
                        color="info"
                        variant="outlined"
                        prepend-icon="mdi-thermometer"
                      >
                        Профили: {{ greenhouse.climateProfiles }}
                      </v-chip>
                    </div>
                  </v-card-text>
                </v-card>
              </v-slide-group-item>
            </v-slide-group>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row class="mt-6">
      <v-col cols="12" lg="6">
        <v-card class="activity-card" variant="outlined">
          <v-card-title class="d-flex justify-space-between align-center">
            <span class="text-subtitle-1 font-weight-medium">Журнал событий</span>
            <v-chip color="warning" size="small" variant="tonal">
              Активных: {{ summaryEvents.active }} / критичных: {{ summaryEvents.critical }}
            </v-chip>
          </v-card-title>
          <v-divider></v-divider>
          <EventLog
            :events="eventsStore.events"
            :limit="5"
            class="event-log"
            @resolve="resolveEvent"
            @resolve-all="resolveAllEvents"
            @delete="deleteEvent"
          />
        </v-card>
      </v-col>

      <v-col cols="12" lg="6">
        <v-card class="telemetry-card" variant="outlined">
          <v-card-title class="d-flex justify-space-between align-center">
            <div>
              <span class="text-subtitle-1 font-weight-medium">Последняя телеметрия</span>
              <div class="text-caption text-medium-emphasis">
                За последний час: {{ summaryTelemetry.lastHour }}
              </div>
            </div>
            <v-btn
              icon
              variant="text"
              @click="refreshDashboard"
              :loading="isRefreshingData"
            >
              <v-icon icon="mdi-refresh"></v-icon>
            </v-btn>
          </v-card-title>
          <v-divider></v-divider>
          <v-list
            v-if="latestTelemetry.length"
            lines="two"
            density="comfortable"
          >
            <v-list-item
              v-for="item in latestTelemetry"
              :key="item.node_id"
            >
              <template #prepend>
                <v-avatar color="primary" variant="tonal">
                  <v-icon :icon="nodeTypeIcon(item.node_type)"></v-icon>
                </v-avatar>
              </template>
              <v-list-item-title>{{ item.node_id }}</v-list-item-title>
              <v-list-item-subtitle>
                {{ formatTelemetryData(item.data) }}
              </v-list-item-subtitle>
              <template #append>
                <span class="text-caption text-medium-emphasis">
                  {{ formatRelativeTime(item.received_at) }}
                </span>
              </template>
            </v-list-item>
          </v-list>
          <div v-else class="text-center pa-6 text-medium-emphasis">
            Нет данных телеметрии
          </div>
        </v-card>
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12">
        <v-alert
          v-if="isDashboardLoading"
          type="info"
          variant="tonal"
          class="mb-4"
          border="start"
          icon="mdi-progress-check"
        >
          Обновление данных...
        </v-alert>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useAppStore } from '@/stores/app'
import { useNodesStore } from '@/stores/nodes'
import { useEventsStore } from '@/stores/events'
import { useGreenhousesStore } from '@/stores/greenhouses'
import { useResponsive } from '@/composables/useResponsive'
import { usePullToRefresh } from '@/composables/usePullToRefresh'
import EventLog from '@/components/EventLog.vue'
import StatCard from '@/components/ui/StatCard.vue'
import PullToRefreshIndicator from '@/components/ui/PullToRefreshIndicator.vue'

interface GreenhouseOverviewItem {
  id: number
  name: string
  statusLabel: string
  statusColor: string
  zoneCount: number
  nodeCount: number
  offlineNodes: number
  climateProfiles: number
}

const appStore = useAppStore()
const nodesStore = useNodesStore()
const eventsStore = useEventsStore()
const greenhousesStore = useGreenhousesStore()
const { isMobile } = useResponsive()

const summary = ref<any>(null)
const isRefreshingData = ref(false)

const { isPulling, pullDistance, isRefreshing } = usePullToRefresh(async () => {
  await refreshDashboard()
  appStore.showSnackbar('Данные обновлены', 'success', 2000)
})

const summaryNodes = computed(() => ({
  total: summary.value?.nodes?.total ?? 0,
  online: summary.value?.nodes?.online ?? 0,
  offline: summary.value?.nodes?.offline ?? 0,
}))

const summaryEvents = computed(() => ({
  active: summary.value?.events?.active ?? 0,
  critical: summary.value?.events?.critical ?? 0,
}))

const summaryTelemetry = computed(() => ({
  lastHour: summary.value?.telemetry?.last_hour ?? 0,
}))

const nodeTypeChips = computed(() => {
  const byType = summary.value?.nodes?.by_type ?? {}
  return Object.entries(byType).map(([type, count]) => ({
    type,
    label: `${type}: ${count}`,
  }))
})

const quickStats = computed(() => [
  {
    label: 'Узлы онлайн',
    value: summaryNodes.value.online,
    icon: 'mdi-access-point-network',
    color: 'success',
    subtitle: `из ${summaryNodes.value.total}`,
  },
  {
    label: 'Узлы офлайн',
    value: summaryNodes.value.offline,
    icon: 'mdi-access-point-network-off',
    color: 'error',
    subtitle: summaryNodes.value.total ? `из ${summaryNodes.value.total}` : null,
  },
  {
    label: 'Активные события',
    value: summaryEvents.value.active,
    icon: 'mdi-alert-circle',
    color: summaryEvents.value.active ? 'warning' : 'primary',
    subtitle: `критичных: ${summaryEvents.value.critical}`,
  },
  {
    label: 'Команды в очереди',
    value: summary.value?.commands?.pending ?? 0,
    icon: 'mdi-progress-clock',
    color: 'info',
    subtitle: `сегодня: ${summary.value?.commands?.today ?? 0}`,
  },
  {
    label: 'Телеметрия',
    value: summaryTelemetry.value.lastHour,
    icon: 'mdi-chart-line',
    color: 'primary',
    subtitle: 'за последний час',
  },
])

const systemStatusItems = computed(() => {
  const systemStatus = appStore.systemStatus
  return [
    {
      label: 'Backend',
      status: appStore.backendConnected ? 'online' : 'offline',
      statusLabel: appStore.backendConnected ? 'online' : 'offline',
      icon: appStore.backendConnected ? 'mdi-database-check' : 'mdi-database-off',
      color: appStore.backendConnected ? 'success' : 'error',
      description: appStore.backendConnected ? 'База данных доступна' : 'Нет доступа к БД',
    },
    {
      label: 'MQTT',
      status: appStore.mqttConnected ? 'online' : 'offline',
      statusLabel: appStore.mqttConnected ? 'connected' : 'disconnected',
      icon: appStore.mqttConnected ? 'mdi-antenna' : 'mdi-antenna-off',
      color: appStore.mqttConnected ? 'success' : 'error',
      description: `Статус брокера: ${systemStatus?.mqtt ?? '—'}`,
    },
    {
      label: 'Telegram',
      status: systemStatus?.telegram === 'configured' ? 'online' : 'offline',
      statusLabel: systemStatus?.telegram ?? '—',
      icon: systemStatus?.telegram === 'configured' ? 'mdi-send-check' : 'mdi-send-off',
      color: systemStatus?.telegram === 'configured' ? 'info' : 'grey',
      description: systemStatus?.telegram === 'configured'
        ? 'Уведомления включены'
        : 'Интеграция отключена',
    },
  ]
})

const greenhouseOverview = computed<GreenhouseOverviewItem[]>(() => {
  const result: GreenhouseOverviewItem[] = []

  greenhousesStore.items.forEach((gh) => {
    const nodes = nodesStore.nodes.filter((node: any) => node.greenhouse_id === gh.id)
    const offlineNodes = nodes.filter((node: any) => !(node.online || node.is_online)).length
    const statusMap: Record<string, { label: string; color: string }> = {
      active: { label: 'Активна', color: 'success' },
      draft: { label: 'Черновик', color: 'info' },
      maintenance: { label: 'Сервис', color: 'warning' },
      offline: { label: 'Отключена', color: 'grey' },
    }

    const statusInfo = statusMap[gh.status as keyof typeof statusMap] || {
      label: gh.status || '—',
      color: 'grey',
    }

    result.push({
      id: gh.id,
      name: gh.name,
      statusLabel: statusInfo.label,
      statusColor: statusInfo.color,
      zoneCount: gh.zone_count ?? 0,
      nodeCount: nodes.length,
      offlineNodes,
      climateProfiles: gh.climate_profiles?.length ?? 0,
    })
  })

  return result
})

const latestTelemetry = computed(() => {
  const data = Array.isArray(summary.value?.telemetry?.latest) ? summary.value.telemetry.latest : []
  return data.slice(0, 6)
})

const lastUpdated = computed(() => {
  const timestamp = summary.value?.timestamp
  if (!timestamp) return 'нет данных'
  const date = new Date(timestamp)
  if (Number.isNaN(date.getTime())) return 'нет данных'
  return date.toLocaleString('ru-RU', { hour: '2-digit', minute: '2-digit', day: '2-digit', month: 'long' })
})

const lastUpdatedShort = computed(() => {
  const timestamp = summary.value?.timestamp
  if (!timestamp) return '—'
  const date = new Date(timestamp)
  if (Number.isNaN(date.getTime())) return '—'
  return date.toLocaleTimeString('ru-RU', { hour: '2-digit', minute: '2-digit' })
})

const systemStatusColor = computed(() => {
  if (!summaryNodes.value.total) {
    return 'grey'
  }
  const onlinePercent = (summaryNodes.value.online / summaryNodes.value.total) * 100
  if (onlinePercent >= 80) return 'success'
  if (onlinePercent >= 50) return 'warning'
  return 'error'
})

const systemStatusIcon = computed(() => {
  if (!summaryNodes.value.total) {
    return 'mdi-help-circle'
  }
  const onlinePercent = (summaryNodes.value.online / summaryNodes.value.total) * 100
  if (onlinePercent >= 80) return 'mdi-check-circle'
  if (onlinePercent >= 50) return 'mdi-alert-circle'
  return 'mdi-close-circle'
})

const isDashboardLoading = computed(() =>
  appStore.loading || nodesStore.loading || eventsStore.loading || greenhousesStore.loading,
)

onMounted(async () => {
  await refreshDashboard()
})

async function refreshDashboard() {
  try {
    isRefreshingData.value = true
    const [summaryData] = await Promise.all([
      appStore.fetchDashboardSummary(),
      appStore.fetchSystemStatus(),
      nodesStore.fetchNodes(),
      eventsStore.fetchEvents({ limit: 20 }),
      greenhousesStore.fetchGreenhouses({ refresh: true }),
    ])

    summary.value = summaryData
  } catch (error) {
    console.error('Error loading dashboard:', error)
  } finally {
    isRefreshingData.value = false
  }
}

function nodeTypeIcon(type: string) {
  const map: Record<string, string> = {
    root: 'mdi-router-network',
    climate: 'mdi-weather-partly-cloudy',
    relay: 'mdi-transmission-tower',
    water: 'mdi-water',
    display: 'mdi-monitor',
    'ph_ec': 'mdi-flask',
    'deprecated_ph_ec': 'mdi-flask-off',
  }
  return map[type] || map[type === 'deprecated_ph_ec' ? 'ph_ec' : type] || 'mdi-chip'
}

function formatTelemetryData(data: Record<string, any>) {
  if (!data) return 'Нет данных'
  const entries = Object.entries(data)
  if (entries.length === 0) return 'Нет данных'
  return entries
    .slice(0, 3)
    .map(([key, value]) => `${key}: ${typeof value === 'number' ? value.toFixed(2) : value}`)
    .join(' • ')
}

function formatRelativeTime(value: string | Date | null | undefined) {
  if (!value) return '—'
  const date = new Date(value)
  if (Number.isNaN(date.getTime())) return '—'
  const diff = (Date.now() - date.getTime()) / 1000
  if (diff < 60) {
    return `${Math.max(1, Math.floor(diff))} с назад`
  }
  if (diff < 3600) {
    return `${Math.floor(diff / 60)} мин назад`
  }
  if (diff < 86400) {
    return `${Math.floor(diff / 3600)} ч назад`
  }
  return date.toLocaleDateString('ru-RU')
}

async function resolveEvent(eventId: number) {
  try {
    await eventsStore.resolveEvent(eventId)
    appStore.showSnackbar('Событие помечено как решенное', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка резолвения события', 'error')
  }
}

async function resolveAllEvents() {
  try {
    const activeIds = eventsStore.activeEvents.map((e: any) => e.id)
    await eventsStore.resolveBulk(activeIds)
    appStore.showSnackbar(`Решено ${activeIds.length} событий`, 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка резолвения событий', 'error')
  }
}

async function deleteEvent(eventId: number) {
  try {
    await eventsStore.deleteEvent(eventId)
    appStore.showSnackbar('Событие удалено', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка удаления события', 'error')
  }
}
</script>

<style scoped>
.dashboard {
  background: var(--v-theme-background);
}

.dashboard-summary-card {
  background: linear-gradient(135deg, rgba(33, 150, 243, 0.08), rgba(156, 39, 176, 0.08));
  border-radius: 20px;
}

.system-status-card,
.greenhouse-section,
.activity-card,
.telemetry-card {
  border-radius: 16px;
}

.status-item.online .v-chip {
  text-transform: uppercase;
}

.status-item.offline .v-chip {
  text-transform: uppercase;
}

.greenhouse-slider {
  padding: 12px 4px 4px;
}

.greenhouse-card {
  min-width: 260px;
  margin: 0 8px;
  border-radius: 18px;
}

.empty-state-card {
  background: rgba(0, 0, 0, 0.02);
  border-radius: 16px;
}

.stagger-fade-enter-active,
.stagger-fade-leave-active {
  transition: all 0.3s ease;
}

.stagger-fade-enter-from,
.stagger-fade-leave-to {
  opacity: 0;
  transform: translateY(10px);
}

.event-log {
  max-height: 420px;
  overflow-y: auto;
}

.status-item .v-avatar {
  background: rgba(0, 0, 0, 0.02);
}

@media (max-width: 960px) {
  .greenhouse-card {
    min-width: 220px;
  }
}
</style>

