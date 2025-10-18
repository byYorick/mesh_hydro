<template>
  <div>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3 mb-4">Настройки</h1>
      </v-col>
    </v-row>

    <!-- System Status -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title>Статус системы</v-card-title>
          <v-card-text>
            <v-list v-if="systemStatus" density="compact">
              <v-list-item>
                <template v-slot:prepend>
                  <v-icon :color="systemStatus.database === 'ok' ? 'success' : 'error'">
                    {{ systemStatus.database === 'ok' ? 'mdi-database-check' : 'mdi-database-alert' }}
                  </v-icon>
                </template>
                <v-list-item-title>База данных</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.database }}</v-list-item-subtitle>
              </v-list-item>

              <v-list-item>
                <template v-slot:prepend>
                  <v-icon :color="systemStatus.mqtt === 'connected' ? 'success' : 'error'">
                    {{ systemStatus.mqtt === 'connected' ? 'mdi-access-point' : 'mdi-access-point-off' }}
                  </v-icon>
                </template>
                <v-list-item-title>MQTT Broker</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.mqtt }}</v-list-item-subtitle>
              </v-list-item>

              <v-list-item>
                <template v-slot:prepend>
                  <v-icon :color="systemStatus.telegram === 'ok' ? 'success' : 'warning'">
                    mdi-send
                  </v-icon>
                </template>
                <v-list-item-title>Telegram Bot</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.telegram }}</v-list-item-subtitle>
              </v-list-item>

              <v-divider class="my-4"></v-divider>

              <v-list-item v-if="systemStatus.system">
                <v-list-item-title>PHP Version</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.system.php_version }}</v-list-item-subtitle>
              </v-list-item>

              <v-list-item v-if="systemStatus.system">
                <v-list-item-title>Laravel Version</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.system.laravel_version }}</v-list-item-subtitle>
              </v-list-item>

              <v-list-item v-if="systemStatus.system">
                <v-list-item-title>Server Time</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.system.server_time }}</v-list-item-subtitle>
              </v-list-item>
            </v-list>

            <div v-else class="text-center pa-4">
              <v-progress-circular indeterminate></v-progress-circular>
            </div>
          </v-card-text>

          <v-card-actions>
            <v-btn
              prepend-icon="mdi-refresh"
              @click="loadSystemStatus"
              :loading="loading"
            >
              Обновить статус
            </v-btn>
          </v-card-actions>
        </v-card>
      </v-col>
    </v-row>

    <!-- Appearance Settings -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title>Внешний вид</v-card-title>
          <v-card-text>
            <v-switch
              :model-value="appStore.isDarkTheme"
              @update:model-value="appStore.toggleTheme()"
              label="Темная тема"
              color="primary"
            ></v-switch>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- About -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title>О системе</v-card-title>
          <v-card-text>
            <v-list density="compact">
              <v-list-item>
                <v-list-item-title>Название</v-list-item-title>
                <v-list-item-subtitle>Mesh Hydro System</v-list-item-subtitle>
              </v-list-item>
              <v-list-item>
                <v-list-item-title>Версия</v-list-item-title>
                <v-list-item-subtitle>2.0.0</v-list-item-subtitle>
              </v-list-item>
              <v-list-item>
                <v-list-item-title>Технологии</v-list-item-title>
                <v-list-item-subtitle>
                  Vue.js 3 • Vuetify 3 • Laravel 10 • PostgreSQL 15 • MQTT
                </v-list-item-subtitle>
              </v-list-item>
            </v-list>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useAppStore } from '@/stores/app'

const appStore = useAppStore()

const systemStatus = ref(null)
const loading = ref(false)

onMounted(async () => {
  await loadSystemStatus()
})

async function loadSystemStatus() {
  loading.value = true
  try {
    systemStatus.value = await appStore.fetchSystemStatus()
  } catch (error) {
    appStore.showSnackbar('Ошибка загрузки статуса', 'error')
  } finally {
    loading.value = false
  }
}
</script>

