<template>
  <v-app :theme="appStore.theme">
    <!-- Navigation Drawer (Desktop only) -->
    <v-navigation-drawer
      v-if="!isMobile"
      v-model="appStore.drawer"
      app
      :rail="rail"
      @update:rail="rail = $event"
    >
      <!-- Logo / Header -->
      <v-list-item
        :prepend-avatar="rail ? undefined : '/favicon.svg'"
        :title="rail ? '' : 'Hydro System'"
        :subtitle="rail ? '' : 'v2.0'"
        class="px-2"
      >
        <template v-slot:append>
          <v-btn
            icon="mdi-chevron-left"
            variant="text"
            @click.stop="rail = !rail"
          ></v-btn>
        </template>
      </v-list-item>

      <v-divider></v-divider>

      <!-- Navigation Menu -->
      <v-list density="compact" nav>
        <v-list-item
          v-for="route in menuRoutes"
          :key="route.name"
          :to="{ name: route.name }"
          :prepend-icon="route.meta.icon"
          :title="route.meta.title"
          :value="route.name"
        ></v-list-item>
      </v-list>

      <!-- Connection Status -->
      <template v-slot:append>
        <div class="pa-2">
          <v-divider class="mb-2"></v-divider>
          <v-list-item
            density="compact"
            :subtitle="rail ? '' : connectionStatus"
          >
            <template v-slot:prepend>
              <v-icon
                :color="appStore.backendConnected ? 'success' : 'error'"
                size="small"
              >
                {{ appStore.backendConnected ? 'mdi-lan-connect' : 'mdi-lan-disconnect' }}
              </v-icon>
            </template>
          </v-list-item>
        </div>
      </template>
    </v-navigation-drawer>

    <!-- App Bar -->
    <v-app-bar app prominent>
      <v-app-bar-nav-icon @click="appStore.toggleDrawer()"></v-app-bar-nav-icon>
      
      <v-toolbar-title>
        {{ $route.meta.title || 'Mesh Hydro System' }}
      </v-toolbar-title>

      <v-spacer></v-spacer>

      <!-- Critical Events Badge -->
      <v-badge
        v-if="eventsStore.criticalCount > 0"
        :content="eventsStore.criticalCount"
        color="error"
        offset-x="-5"
        offset-y="5"
      >
        <v-btn icon="mdi-alert-circle" color="error" @click="goToEvents"></v-btn>
      </v-badge>

      <!-- Theme Toggle -->
      <v-btn
        :icon="appStore.isDarkTheme ? 'mdi-weather-sunny' : 'mdi-weather-night'"
        @click="appStore.toggleTheme()"
      ></v-btn>

      <!-- Offline Indicator -->
      <v-chip
        v-if="isOfflineMode"
        color="warning"
        size="small"
        prepend-icon="mdi-wifi-off"
        class="mr-2"
      >
        Офлайн
      </v-chip>

      <!-- Refresh Button -->
      <v-btn
        icon="mdi-refresh"
        @click="refreshData"
        :loading="appStore.loading"
        :disabled="isOfflineMode"
      ></v-btn>

      <!-- Settings -->
      <v-btn icon="mdi-cog" :to="{ name: 'Settings' }"></v-btn>
    </v-app-bar>

    <!-- Main Content -->
    <v-main>
      <v-container fluid>
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </v-container>
    </v-main>

    <!-- Snackbar -->
    <v-snackbar
      v-model="appStore.snackbar.show"
      :color="appStore.snackbar.color"
      :timeout="appStore.snackbar.timeout"
      location="top right"
    >
      {{ appStore.snackbar.message }}
      
      <template v-slot:actions>
        <v-btn
          variant="text"
          @click="appStore.hideSnackbar()"
        >
          Закрыть
        </v-btn>
      </template>
    </v-snackbar>

    <!-- Status Bar -->
    <StatusBar
      :visible="statusBarStore.isVisible"
      :message="statusBarStore.message"
      :node-id="statusBarStore.nodeId"
      :level="statusBarStore.level"
      :timestamp="statusBarStore.timestamp"
      :pump-details="statusBarStore.pumpDetails"
      :pid-params="statusBarStore.pidParams"
      :additional-params="statusBarStore.additionalParams"
      :progress="statusBarStore.progress"
      @close="statusBarStore.hide"
      @toggle="statusBarStore.toggleExpanded"
    />

    <!-- Node Auto-Discovery Indicator -->
    <NodeDiscoveryIndicator ref="discoveryIndicator" />

    <!-- Bottom Navigation (Mobile only) -->
    <v-bottom-navigation
      v-if="isMobile"
      v-model="mobileTab"
      grow
      app
      bg-color="surface"
      elevation="8"
    >
      <v-btn value="dashboard" @click="$router.push('/')">
        <v-icon>mdi-view-dashboard</v-icon>
        <span>Главная</span>
      </v-btn>

      <v-btn value="nodes" @click="$router.push('/nodes')">
        <v-badge
          :content="nodesStore.onlineNodes.length"
          :model-value="nodesStore.onlineNodes.length > 0"
          color="success"
        >
          <v-icon>mdi-access-point-network</v-icon>
        </v-badge>
        <span>Узлы</span>
      </v-btn>

      <v-btn value="events" @click="$router.push('/events')">
        <v-badge
          :content="eventsStore.criticalCount"
          :model-value="eventsStore.criticalCount > 0"
          color="error"
        >
          <v-icon>mdi-alert-circle</v-icon>
        </v-badge>
        <span>События</span>
      </v-btn>

      <v-btn value="more" @click="showMobileMenu = true">
        <v-icon>mdi-menu</v-icon>
        <span>Еще</span>
      </v-btn>
    </v-bottom-navigation>

    <!-- Mobile Menu Dialog -->
    <v-dialog v-model="showMobileMenu" fullscreen transition="dialog-bottom-transition">
      <v-card>
        <v-toolbar color="primary">
          <v-btn icon="mdi-close" @click="showMobileMenu = false"></v-btn>
          <v-toolbar-title>Меню</v-toolbar-title>
        </v-toolbar>

        <v-list>
          <v-list-item
            v-for="route in menuRoutes"
            :key="route.name"
            :to="{ name: route.name }"
            :prepend-icon="route.meta.icon"
            :title="route.meta.title"
            @click="showMobileMenu = false"
          ></v-list-item>
        </v-list>
      </v-card>
    </v-dialog>
  </v-app>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted, inject } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useAppStore } from '@/stores/app'
import { useNodesStore } from '@/stores/nodes'
import { useStatusBarStore } from '@/stores/statusBar'
import { useEventsStore } from '@/stores/events'
import { useErrorsStore } from '@/stores/errors'
import { useSettingsStore } from '@/stores/settings'
import { useTelemetryStore } from '@/stores/telemetry'
import { useResponsive } from '@/composables/useResponsive'
import { useOfflineMode } from '@/composables/useOfflineMode'
import NodeDiscoveryIndicator from '@/components/NodeDiscoveryIndicator.vue'
import StatusBar from '@/components/StatusBar.vue'
import { getConnectionStatus } from '@/services/echo'

const router = useRouter()
const route = useRoute()
const appStore = useAppStore()
const nodesStore = useNodesStore()
const statusBarStore = useStatusBarStore()
const eventsStore = useEventsStore()
const errorsStore = useErrorsStore()
const settingsStore = useSettingsStore()
const telemetryStore = useTelemetryStore()
const { isMobile } = useResponsive()
const { isOnline, isOfflineMode } = useOfflineMode()

const echo = inject('echo')
const rail = ref(false)
const discoveryIndicator = ref(null)
const mobileTab = ref('dashboard')
const showMobileMenu = ref(false)

// Filtered menu routes (exclude hidden ones)
const menuRoutes = computed(() => {
  console.log('🔍 App.vue: menuRoutes computed called')
  console.log('🔍 App.vue: router.getRoutes():', router.getRoutes())
  
  const routes = router.getRoutes().filter(r => {
    console.log('🔍 App.vue: Processing route:', r)
    console.log('🔍 App.vue: r.meta:', r.meta)
    console.log('🔍 App.vue: r.meta.icon:', r.meta?.icon)
    console.log('🔍 App.vue: r.meta.showInMenu:', r.meta?.showInMenu)
    
    return r.meta.icon && r.meta.showInMenu !== false
  })
  
  console.log('🔍 App.vue: Filtered routes:', routes)
  return routes
})

// Connection status text
const connectionStatus = computed(() => {
  console.log('🔍 App.vue: connectionStatus computed called')
  console.log('🔍 App.vue: appStore.backendConnected:', appStore.backendConnected)
  console.log('🔍 App.vue: appStore.mqttConnected:', appStore.mqttConnected)
  
  if (!appStore.backendConnected) return 'Нет связи'
  if (!appStore.mqttConnected) return 'MQTT: offline'
  
  const wsStatus = getConnectionStatus()
  console.log('🔍 App.vue: wsStatus:', wsStatus)
  
  if (wsStatus.fallbackMode) return 'WebSocket: fallback'
  if (!wsStatus.isWebSocketConnected) return 'WebSocket: offline'
  
  return 'Подключено'
})

// Go to events page
const goToEvents = () => {
  router.push({ name: 'Events', query: { filter: 'critical' } })
}

// Refresh data
const refreshData = async () => {
  appStore.loading = true
  try {
    await Promise.all([
      nodesStore.fetchNodes(),
      eventsStore.fetchEvents({ status: 'active' }),
      appStore.fetchSystemStatus(),
    ])
    appStore.showSnackbar('Данные обновлены', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка обновления данных', 'error')
  } finally {
    appStore.loading = false
  }
}

onMounted(async () => {
  // Initial data load
  await refreshData()

  // Setup WebSocket listeners for real-time updates
  setupWebSocketListeners()
})

onUnmounted(() => {
  // Cleanup WebSocket listeners
  if (echo) {
    echo.leave('hydro-system')
  }
})

// Setup WebSocket listeners for real-time updates
function setupWebSocketListeners() {
  if (!echo) {
    console.warn('Echo not available, falling back to polling')
    // Подписываемся на fallback события
    setupFallbackListeners()
    return
  }

  // Subscribe to hydro-system channel
  const channel = echo.channel('hydro-system')

  // Listen for telemetry updates
  channel.listen('.telemetry.received', (data) => {
    console.log('📡 Real-time telemetry:', data)
    telemetryStore.addTelemetryRealtime(data)
    nodesStore.updateNodeRealtime({
      node_id: data.node_id,
      last_seen_at: data.received_at,
      online: true,
    })
  })

  // Listen for node status changes
  channel.listen('.node.status.changed', (data) => {
    console.log('🔄 Node status changed:', data)
    nodesStore.updateNodeRealtime({
      node_id: data.node_id,
      online: data.online,
      last_seen_at: data.last_seen_at,
    })
    
    // Show notification if node went offline
    if (!data.online) {
      appStore.showSnackbar(`Узел ${data.node_id} офлайн`, 'warning')
    }
  })

  // Listen for new nodes discovered
  channel.listen('.node.discovered', (data) => {
    console.log('🔍 New node discovered:', data)
    nodesStore.updateNodeRealtime(data.node)
    
    // Show discovery notification
    if (discoveryIndicator.value) {
      discoveryIndicator.value.showDiscovery(data)
    }
  })

  // Listen for new events
  channel.listen('.event.created', (data) => {
    console.log('🔔 New event:', data)
    eventsStore.addEventRealtime(data)
    
    // Show status bar for events with pump or PID details
    if (data.data && (data.data.pump_id || data.data.kp || data.data.current_value)) {
      statusBarStore.showForEvent(data)
    }
    
    // Show notification for critical events
    const criticalLevels = ['critical', 'emergency']
    try {
      // Дополнительная проверка на undefined/null перед вызовом includes
      if (data && data.level && typeof data.level === 'string' && criticalLevels && Array.isArray(criticalLevels) && criticalLevels.includes(data.level)) {
        appStore.showSnackbar(
          `⚠️ ${data.message}`,
          'error',
          8000
        )
      }
    } catch (error) {
      console.error('App.vue: critical event notification - Error:', error)
      console.error('App.vue: critical event notification - data:', data)
      console.error('App.vue: critical event notification - criticalLevels:', criticalLevels)
    }
  })

  // Listen for node errors
  channel.listen('.error.occurred', (data) => {
    console.log('❌ Node error:', data)
    errorsStore.addErrorRealtime(data)
    
    // Show notification for critical errors
    if (data.severity === 'critical') {
      appStore.showSnackbar(
        `🚨 Критичная ошибка: ${data.message}`,
        'error',
        10000
      )
    }
  })

  console.log('✅ WebSocket listeners configured')
  
  // Также подписываемся на fallback события (на случай отключения WS)
  setupFallbackListeners()
}

// Обработка fallback polling событий
function setupFallbackListeners() {
  // Слушаем события от fallback polling
  window.addEventListener('echo:fallback', (event) => {
    const { channel, event: eventName, data } = event.detail
    
    if (channel === 'hydro.nodes' && Array.isArray(data)) {
      console.log('📡 Fallback nodes update:', data.length, 'nodes')
      // Обновляем все узлы из fallback polling
      data.forEach(node => {
        nodesStore.updateNodeRealtime(node)
      })
    }
    
    if (channel === 'hydro.events' && Array.isArray(data)) {
      console.log('🔔 Fallback events update:', data.length, 'events')
      // Обновляем события (если метод существует)
      if (eventsStore.updateEventsFromFallback) {
        eventsStore.updateEventsFromFallback(data)
      } else {
        data.forEach(event => {
          eventsStore.addEventRealtime(event)
        })
      }
    }
  })
}
</script>

<style scoped>
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>

