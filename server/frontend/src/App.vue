<template>
  <v-app :theme="appStore.theme">
    <!-- Navigation Drawer -->
    <v-navigation-drawer
      v-model="appStore.drawer"
      app
      :rail="rail"
      @update:rail="rail = $event"
    >
      <!-- Logo / Header -->
      <v-list-item
        :prepend-avatar="rail ? undefined : '/logo.png'"
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

      <!-- Refresh Button -->
      <v-btn
        icon="mdi-refresh"
        @click="refreshData"
        :loading="appStore.loading"
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
  </v-app>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useAppStore } from '@/stores/app'
import { useNodesStore } from '@/stores/nodes'
import { useEventsStore } from '@/stores/events'

const router = useRouter()
const route = useRoute()
const appStore = useAppStore()
const nodesStore = useNodesStore()
const eventsStore = useEventsStore()

const rail = ref(false)

// Filtered menu routes (exclude hidden ones)
const menuRoutes = computed(() => {
  return router.getRoutes().filter(r => 
    r.meta.icon && r.meta.showInMenu !== false
  )
})

// Connection status text
const connectionStatus = computed(() => {
  if (!appStore.backendConnected) return 'Нет связи'
  if (!appStore.mqttConnected) return 'MQTT: offline'
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

// Auto-refresh interval
let refreshInterval = null

onMounted(async () => {
  // Initial data load
  await refreshData()

  // Auto-refresh every 30 seconds
  refreshInterval = setInterval(() => {
    refreshData()
  }, 30000)
})

onUnmounted(() => {
  if (refreshInterval) {
    clearInterval(refreshInterval)
  }
})
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

