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
      <v-list density="compact" nav v-model:opened="openedGroups">
        <template v-for="item in menuTree" :key="item.name || item.title">
          <v-list-group
            v-if="item.children && item.children.length"
            :value="item.name"
            :active="isItemActive(item)"
          >
            <template #activator="{ props }">
              <v-list-item
                v-bind="props"
                :to="item.to"
                :prepend-icon="item.icon"
                :title="item.title"
                :value="item.name"
                :active="isItemActive(item)"
              />
            </template>

            <template v-for="child in item.children" :key="child.name || child.title">
              <v-list-group
                v-if="child.children && child.children.length"
                :value="child.name"
                class="ms-4"
                :active="isItemActive(child)"
              >
                <template #activator="{ props: childProps }">
                  <v-list-item
                    v-bind="childProps"
                    :to="child.to"
                    :prepend-icon="child.icon"
                    :title="child.title"
                    :value="child.name"
                    :active="isItemActive(child)"
                  />
                </template>

                <v-list-item
                  v-for="grandChild in child.children"
                  :key="grandChild.name || grandChild.title"
                  :to="grandChild.to"
                  :prepend-icon="grandChild.icon"
                  :title="grandChild.title"
                  :value="grandChild.name"
                  :active="isItemActive(grandChild)"
                  class="ms-6"
                />
              </v-list-group>

              <v-list-item
                v-else
                :to="child.to"
                :prepend-icon="child.icon"
                :title="child.title"
                :value="child.name"
                :active="isItemActive(child)"
                class="ms-4"
              />
            </template>
          </v-list-group>

          <v-list-item
            v-else
            :to="item.to"
            :prepend-icon="item.icon"
            :title="item.title"
            :value="item.name"
            :active="isItemActive(item)"
          />
        </template>
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

      <ZoneSelector class="mr-4 d-none d-sm-flex" />

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

        <v-list v-model:opened="openedGroups">
          <template v-for="item in menuTree" :key="`mobile-${item.name || item.title}`">
            <v-list-group
              v-if="item.children && item.children.length"
              :value="item.name"
              :active="isItemActive(item)"
            >
              <template #activator="{ props }">
                <v-list-item
                  v-bind="props"
                  :to="item.to"
                  :prepend-icon="item.icon"
                  :title="item.title"
                  :value="item.name"
                  :active="isItemActive(item)"
                  @click="handleMobileSelect"
                />
              </template>

              <template v-for="child in item.children" :key="`mobile-${child.name || child.title}`">
                <v-list-group
                  v-if="child.children && child.children.length"
                  :value="child.name"
                  class="ms-4"
                  :active="isItemActive(child)"
                >
                  <template #activator="{ props: childProps }">
                    <v-list-item
                      v-bind="childProps"
                      :to="child.to"
                      :prepend-icon="child.icon"
                      :title="child.title"
                      :value="child.name"
                      :active="isItemActive(child)"
                      @click="handleMobileSelect"
                    />
                  </template>

                  <v-list-item
                    v-for="grandChild in child.children"
                    :key="`mobile-${grandChild.name || grandChild.title}`"
                    :to="grandChild.to"
                    :prepend-icon="grandChild.icon"
                    :title="grandChild.title"
                    :value="grandChild.name"
                    :active="isItemActive(grandChild)"
                    class="ms-6"
                    @click="handleMobileSelect"
                  />
                </v-list-group>

                <v-list-item
                  v-else
                  :to="child.to"
                  :prepend-icon="child.icon"
                  :title="child.title"
                  :value="child.name"
                  :active="isItemActive(child)"
                  class="ms-4"
                  @click="handleMobileSelect"
                />
              </template>
            </v-list-group>

            <v-list-item
              v-else
              :to="item.to"
              :prepend-icon="item.icon"
              :title="item.title"
              :value="item.name"
              :active="isItemActive(item)"
              @click="handleMobileSelect"
            />
          </template>
        </v-list>
      </v-card>
    </v-dialog>

    <!-- Universal Popup System -->
    <UniversalPopup />
  </v-app>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, inject, watch } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import type { RouteLocationRaw, RouteRecordNormalized } from 'vue-router'
import { useAppStore } from '@/stores/app'
import { useNodesStore } from '@/stores/nodes'
import { useEventsStore } from '@/stores/events'
import ZoneSelector from '@/components/ZoneSelector.vue'
import { useErrorsStore } from '@/stores/errors'
import { useSettingsStore } from '@/stores/settings'
import { useTelemetryStore } from '@/stores/telemetry'
import { useSetupStore } from '@/stores/setup'
import { useResponsive } from '@/composables/useResponsive'
import { useOfflineMode } from '@/composables/useOfflineMode'
import { usePopup } from '@/composables/usePopup'
import NodeDiscoveryIndicator from '@/components/NodeDiscoveryIndicator.vue'
import UniversalPopup from '@/components/ui/UniversalPopup.vue'
import { getConnectionStatus } from '@/services/echo'
import { nodeStatusManager } from '@/services/NodeStatusManager'

const router = useRouter()
const route = useRoute()
const appStore = useAppStore()
const nodesStore = useNodesStore()
const eventsStore = useEventsStore()
const errorsStore = useErrorsStore()
const settingsStore = useSettingsStore()
const telemetryStore = useTelemetryStore()
const setupStore = useSetupStore()
const { isMobile } = useResponsive()
const { isOnline, isOfflineMode } = useOfflineMode()
const popup = usePopup()

const echo = inject('echo')
const rail = ref(false)
const discoveryIndicator = ref(null)
const mobileTab = ref('dashboard')
const showMobileMenu = ref(false)

const openedGroups = ref<string[]>([])

type MenuItem = {
  title: string
  icon?: string
  name?: string
  to?: RouteLocationRaw
  children?: MenuItem[]
}

const menuTree = computed<MenuItem[]>(() => {
  const routeMap = new Map<string, RouteRecordNormalized>()

  router.getRoutes().forEach(r => {
    if (typeof r.name === 'string') {
      routeMap.set(r.name, r)
    }
  })

  const buildItem = (routeName: string, overrides: Partial<MenuItem> = {}): MenuItem | null => {
    const routeConfig = routeMap.get(routeName)
    if (!routeConfig || routeConfig.meta?.showInMenu === false) {
      return null
    }

    if (typeof routeConfig.name !== 'string') {
      return null
    }

    return {
      title: overrides.title ?? (routeConfig.meta?.title as string) ?? routeName,
      icon: overrides.icon ?? (routeConfig.meta?.icon as string | undefined),
      name: routeConfig.name,
      to: { name: routeConfig.name },
      children: overrides.children ?? [],
    }
  }

  const nodesChildren = ['Setup', 'Telemetry', 'Events', 'Analytics', 'Errors']
    .map(name => buildItem(name))
    .filter((item): item is MenuItem => item !== null)

  const greenhousesChildren = ['Zones', 'GrowthPlanner']
    .map(name => buildItem(name))
    .filter((item): item is MenuItem => item !== null)

  const items: MenuItem[] = []

  const dashboard = buildItem('Dashboard')
  if (dashboard) {
    items.push(dashboard)
  }

  const nodes = buildItem('Nodes')
  if (nodes) {
    nodes.children = nodesChildren
    items.push(nodes)
  }

  const greenhouses = buildItem('Greenhouses')
  if (greenhouses) {
    greenhouses.children = greenhousesChildren
    items.push(greenhouses)
  }

  const presetLibrary = buildItem('PresetLibrary')
  if (presetLibrary) {
    items.push(presetLibrary)
  }

  const documentation = buildItem('Documentation')
  if (documentation) {
    items.push(documentation)
  }

  const settings = buildItem('Settings')
  if (settings) {
    items.push(settings)
  }

  return items
})

const findPathInTree = (items: MenuItem[], targetName: string | undefined, stack: MenuItem[] = []): MenuItem[] | null => {
  if (!targetName) {
    return null
  }

  for (const item of items) {
    const currentPath = [...stack, item]

    if (item.name === targetName) {
      return currentPath
    }

    if (item.children && item.children.length) {
      const childPath = findPathInTree(item.children, targetName, currentPath)
      if (childPath) {
        return childPath
      }
    }
  }

  return null
}

const isItemActive = (item: MenuItem): boolean => {
  if (item.name && item.name === route.name) {
    return true
  }

  return !!item.children?.some(child => isItemActive(child))
}

watch(
  () => route.name,
  (currentName) => {
    const path = findPathInTree(menuTree.value, currentName)

    if (!path) {
      openedGroups.value = []
      return
    }

    openedGroups.value = path
    .filter((item): item is MenuItem & { name: string } => !!item.children?.length && !!item.name)
    .map(item => item.name)
  },
  { immediate: true }
)

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
    // Последовательная загрузка с небольшими задержками для избежания throttling
    await nodesStore.fetchNodes()
    await new Promise(resolve => setTimeout(resolve, 100)) // 100ms задержка

    await setupStore.fetchNewNodes()
    await new Promise(resolve => setTimeout(resolve, 100))

    await eventsStore.fetchEvents({ status: 'active' })
    await new Promise(resolve => setTimeout(resolve, 100)) // 100ms задержка
    
    await appStore.fetchSystemStatus()
    
    popup.toast.success('Данные обновлены')
  } catch (error) {
    console.error('Error refreshing data:', error)
    // Не показываем ошибку для throttling, это нормально при перезагрузке
    if (!error?.response || error.response.status !== 429) {
      popup.toast.error('Ошибка обновления данных')
    }
  } finally {
    appStore.loading = false
  }
}

onMounted(async () => {
  // Setup global error handlers
  const app = (window as any).appInstance
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
      popup.toast.error(`Критическая ошибка: ${err?.message || 'Неизвестная ошибка'}`)
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
  
  // Загрузить конфигурацию статусов перед остальной инициализацией
  await nodeStatusManager.loadConfig()
  
  // Continue with mounted logic
  // Initial data load
  await refreshData()

  // Setup fallback listeners FIRST (they work even if WebSocket fails)
  setupFallbackListeners()
  
  // Setup WebSocket listeners for real-time updates
  setupWebSocketListeners()
  
  // Catch unhandled promise rejections
  window.addEventListener('unhandledrejection', (event: PromiseRejectionEvent) => {
    console.error('🔴 UNHANDLED PROMISE REJECTION:', event.reason)
    popup.toast.error(`Необработанная ошибка: ${event.reason?.message || 'Неизвестная ошибка'}`)
  })
  
  // Catch uncaught errors
  window.addEventListener('error', (event: ErrorEvent) => {
    console.error('🔴 UNCAUGHT ERROR:', event.error)
    popup.toast.error(`Системная ошибка: ${event.error?.message || 'Неизвестная ошибка'}`)
  })
})

onUnmounted(() => {
  // Cleanup WebSocket listeners
  if (echo) {
    echo.leave('hydro-system')
    echo.leave('hydro-setup')
  }

  setupStore.setRealtimeConnection(false)
})

// Setup WebSocket listeners for real-time updates
function setupWebSocketListeners() {
  if (!echo) {
    console.warn('Echo not available, falling back to polling')
    setupStore.setRealtimeConnection(false)
    // Подписываемся на fallback события
    setupFallbackListeners()
    return
  }

  // Subscribe to hydro-system channel
  const channel = echo.channel('hydro-system')
  const setupChannel = echo.channel('hydro-setup')

  setupStore.setRealtimeConnection(true)

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
      popup.toast.warning(`Узел ${data.node_id} офлайн`)
    }
  })

  // Listen for periodic node status updates (every heartbeat)
  channel.listen('.node.status.update', (data) => {
    console.log('🔄 Node status update:', data.node_id)
    
    // Обновить в store
    nodesStore.updateNodeRealtime({
      node_id: data.node_id,
      online: data.online,
      last_seen_at: data.last_seen_at,
      status_color: data.status_color,
      icon: data.icon,
      metadata: data.metadata,
    })
    
    // Менеджер автоматически уведомит подписчиков через updateNodeRealtime
  })

    // Listen for new nodes discovered
  channel.listen('.node.discovered', (data) => {
    console.log('🔍 New node discovered:', data)
    nodesStore.updateNodeRealtime(data.node)
    
    // Show discovery notification через новую систему usePopup
    if (discoveryIndicator.value) {
      discoveryIndicator.value.showDiscovery(data)
    }
  })

  // Listen for setup channel events
  setupChannel.listen('.new-node.discovered', async (data) => {
    console.log('🆕 Setup discovery event:', data)
    await setupStore.handleDiscovered(data)
    popup.toast.add({
      level: 'info',
      message: `Обнаружено устройство в режиме настройки (${data?.node_type || data?.mac_address})`,
      data,
    })
  })

  setupChannel.listen('.new-node.updated', (data) => {
    console.log('🔁 Setup node updated:', data)
    setupStore.handleUpdated(data)
  })

  setupChannel.listen('.new-node.configured', (data) => {
    console.log('✅ Setup node configured:', data)
    setupStore.handleConfigured(data)
    popup.toast.success(`Узел ${data?.node_id || data?.mac_address} успешно настроен`)
  })

  // Listen for new events
  channel.listen('.event.created', (data) => {
    console.log('🔔 New event received:', data)
    
    // Добавляем событие в store (автоматически обновит все компоненты)
    eventsStore.addEventRealtime(data)
    
    // Обновляем события узла в nodesStore, если узел открыт
    if (data.node_id) {
      const node = nodesStore.nodes.find(n => n.node_id === data.node_id)
      if (node) {
        // Инициализируем events если его нет
        if (!node.events) {
          node.events = []
        }
        // Проверяем, нет ли уже такого события (по ID)
        const existingIndex = node.events.findIndex(e => e.id === data.id)
        if (existingIndex >= 0) {
          // Обновляем существующее событие
          node.events[existingIndex] = data
        } else {
          // Добавляем новое событие в начало списка
          node.events.unshift(data)
        }
        // Ограничиваем количество событий (последние 100)
        if (node.events.length > 100) {
          node.events = node.events.slice(0, 100)
        }
      }
    }
    
    // Показываем уведомление через систему usePopup
    popup.toast.add({
      level: data.level || 'info',
      message: data.message,
      nodeId: data.node_id,
      data: data.data
    })
  })

  // Listen for node errors
  channel.listen('.error.occurred', (data) => {
    console.log('❌ Node error:', data)
    errorsStore.addErrorRealtime(data)
    
    // Show notification for critical errors
    if (data.severity === 'critical') {
      popup.toast.error(`🚨 Критичная ошибка: ${data.message}`)
    }
  })

}

// Setup fallback listeners for polling mode
function setupFallbackListeners() {
  console.log('🔧 Setting up fallback listeners for polling mode')
  
  // Listen for fallback polling events
  window.addEventListener('echo:fallback', (event: Event) => {
    const { detail } = event as CustomEvent<{
      channel?: string
      event?: string
      data?: unknown
    }>
    const { channel, event: eventName, data } = detail || {}
    
    if (channel === 'hydro.nodes' && eventName === 'NodeStatusChanged') {
      // data is an array of all nodes from polling
      if (Array.isArray(data)) {
        // Update all nodes from polling response
        nodesStore.fetchNodes().then(() => {
          console.log('🔄 Nodes updated via fallback polling')
        })
      }
    }
    
    if (channel === 'hydro.events' && eventName === 'EventCreated') {
      // data is an array of recent events
      if (Array.isArray(data)) {
        eventsStore.fetchEvents({ status: 'active' }).then(() => {
          console.log('🔄 Events updated via fallback polling')
        })
      }
    }
  })
}

const handleMobileSelect = () => {
  showMobileMenu.value = false
}
</script>

<style scoped>
.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>

