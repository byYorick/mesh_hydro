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

    <!-- Universal Dialog System -->
    <UniversalDialog />
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
import UniversalDialog from '@/components/ui/UniversalDialog.vue'
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
  return router.getRoutes().filter(r => r.meta.icon && r.meta.showInMenu !== false)
})

// Connection status text
const connectionStatus = computed(() => {
  if (!appStore.backendConnected) return 'Нет связи'
  if (!appStore.mqttConnected) return 'MQTT: offline'
  
  const wsStatus = getConnectionStatus()
  
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
  // Setup global error handlers
  const app = window.appInstance
  if (app) {
    // Global error handler
    app.config.errorHandler = (err, instance, info) => {
      console.error('🔴 GLOBAL ERROR:', {
        error: err,
        instance,
        info,
        timestamp: new Date().toISOString()
      })
      
      // Show error notification
      appStore.showSnackbar(
        `Критическая ошибка: ${err?.message || 'Неизвестная ошибка'}`,
        'error',
        8000
      )
    }

    // Vue warn handler
    app.config.warnHandler = (msg, instance, trace) => {
      console.warn('⚠️ VUE WARNING:', {
        message: msg,
        instance,
        trace,
        timestamp: new Date().toISOString()
      })
    }
  }
  
  // Continue with mounted logic
  // Initial data load
  await refreshData()

  // Setup WebSocket listeners for real-time updates
  setupWebSocketListeners()
  
  // Catch unhandled promise rejections
  window.addEventListener('unhandledrejection', (event) => {
    console.error('🔴 UNHANDLED PROMISE REJECTION:', event.reason)
    appStore.showSnackbar(
      `Необработанная ошибка: ${event.reason?.message || 'Неизвестная ошибка'}`,
      'error',
      8000
    )
  })
  
  // Catch uncaught errors
  window.addEventListener('error', (event) => {
    console.error('🔴 UNCAUGHT ERROR:', event.error)
    appStore.showSnackbar(
      `Системная ошибка: ${event.error?.message || 'Неизвестная ошибка'}`,
      'error',
      8000
    )
  })
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
    
    // Показываем popup для всех событий в зависимости от уровня
    try {
      if (data && data.level && typeof data.level === 'string') {
        const eventLevelMap = {
          'emergency': { type: 'error', icon: '🚨', duration: 15000 },
          'critical': { type: 'error', icon: '⚠️', duration: 10000 },
          'error': { type: 'error', icon: '❌', duration: 8000 },
          'warning': { type: 'warning', icon: '⚡', duration: 6000 },
          'info': { type: 'info', icon: 'ℹ️', duration: 4000 },
          'success': { type: 'success', icon: '✅', duration: 3000 }
        }
        
        const eventConfig = eventLevelMap[data.level] || { type: 'info', icon: '📢', duration: 4000 }
        
        // Формируем сообщение с деталями
        let message = `${eventConfig.icon} ${data.message}`
        
        // Добавляем node_id если есть
        if (data.node_id) {
          message = `[${data.node_id}] ${message}`
        }
        
        // Добавляем детали из data если есть
        if (data.data) {
          if (data.data.pump_id !== undefined) {
            message += ` | Насос ${data.data.pump_id}`
          }
          if (data.data.volume_ml !== undefined) {
            message += ` | ${data.data.volume_ml.toFixed(1)} мл`
          }
          if (data.data.duration_ms !== undefined) {
            message += ` | ${(data.data.duration_ms / 1000).toFixed(1)}с`
          }
        }
        
        appStore.showSnackbar(message, eventConfig.type, eventConfig.duration)
      }
    } catch (error) {
      console.error('App.vue: event notification - Error:', error)
      console.error('App.vue: event notification - data:', data)
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

}
</script>

<style scoped>
.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>

