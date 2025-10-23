<template>
  <v-card>
    <v-card-title class="bg-docker">
      <v-icon icon="mdi-docker" class="mr-2"></v-icon>
      Управление Docker контейнерами
      
      <v-spacer></v-spacer>
      
      <v-btn
        icon="mdi-refresh"
        variant="text"
        color="white"
        :loading="loading"
        @click="loadStatus"
      ></v-btn>
    </v-card-title>

    <v-card-text>
      <!-- Статус Docker -->
      <v-alert
        v-if="!dockerStatus.enabled"
        type="warning"
        variant="tonal"
        class="mb-4"
      >
        Docker управление отключено в конфигурации
      </v-alert>

      <v-alert
        v-else-if="!dockerStatus.running"
        type="error"
        variant="tonal"
        class="mb-4"
      >
        <div class="font-weight-bold">Docker недоступен</div>
        <div class="text-caption mt-1">{{ dockerStatus.error }}</div>
      </v-alert>

      <!-- Список контейнеров -->
      <v-list v-if="containers.length > 0">
        <v-list-item
          v-for="container in containers"
          :key="container.name"
        >
          <template #prepend>
            <v-avatar :color="getStatusColor(container.state)" size="40">
              <v-icon icon="mdi-docker" color="white"></v-icon>
            </v-avatar>
          </template>

          <v-list-item-title>
            {{ container.service }}
          </v-list-item-title>

          <v-list-item-subtitle>
            <div class="d-flex align-center">
              <v-chip
                :color="getStatusColor(container.state)"
                size="x-small"
                class="mr-2"
              >
                {{ container.state }}
              </v-chip>
              
              <span class="text-caption">{{ container.status }}</span>
              
              <v-chip
                v-if="container.health !== 'N/A'"
                :color="container.health === 'healthy' ? 'success' : 'warning'"
                size="x-small"
                class="ml-2"
              >
                {{ container.health }}
              </v-chip>
            </div>
          </v-list-item-subtitle>

          <template #append>
            <div class="d-flex align-center">
              <v-btn
                icon="mdi-file-document-outline"
                size="small"
                variant="text"
                @click="showLogs(container)"
                :disabled="loading"
              ></v-btn>

              <v-btn
                icon="mdi-restart"
                size="small"
                variant="text"
                color="primary"
                @click="restartContainer(container)"
                :disabled="loading"
                :loading="restartingContainer === container.service"
              ></v-btn>
            </div>
          </template>
        </v-list-item>
      </v-list>

      <!-- Действия -->
      <v-divider class="my-4"></v-divider>

      <v-row>
        <v-col cols="12" sm="4">
          <v-btn
            color="success"
            variant="outlined"
            block
            prepend-icon="mdi-play"
            :disabled="!dockerStatus.enabled || loading"
            :loading="startingAll"
            @click="startAll"
          >
            Запустить все
          </v-btn>
        </v-col>

        <v-col cols="12" sm="4">
          <v-btn
            color="primary"
            variant="outlined"
            block
            prepend-icon="mdi-restart"
            :disabled="!dockerStatus.enabled || loading"
            :loading="restartingAll"
            @click="confirmRestartAll"
          >
            Перезапустить все
          </v-btn>
        </v-col>

        <v-col cols="12" sm="4">
          <v-btn
            color="error"
            variant="outlined"
            block
            prepend-icon="mdi-stop"
            :disabled="!dockerStatus.enabled || loading"
            :loading="stoppingAll"
            @click="confirmStopAll"
          >
            Остановить все
          </v-btn>
        </v-col>
      </v-row>
    </v-card-text>

    <!-- Диалог подтверждения -->
    <v-dialog v-model="confirmDialog" max-width="500">
      <v-card>
        <v-card-title class="bg-warning">
          <v-icon icon="mdi-alert" class="mr-2"></v-icon>
          Подтверждение действия
        </v-card-title>

        <v-card-text class="pt-4">
          <div class="text-body-1">{{ confirmMessage }}</div>
          
          <v-alert type="warning" variant="tonal" class="mt-4">
            <strong>⚠️ Внимание:</strong> Это действие повлияет на работу всей системы!
          </v-alert>
        </v-card-text>

        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="confirmDialog = false">Отмена</v-btn>
          <v-btn
            :color="confirmAction === 'stop' ? 'error' : 'primary'"
            @click="executeConfirmedAction"
          >
            Подтвердить
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Диалог логов -->
    <v-dialog v-model="logsDialog" max-width="900" scrollable>
      <v-card>
        <v-card-title>
          <v-icon icon="mdi-file-document-outline" class="mr-2"></v-icon>
          Логи: {{ selectedContainer?.service }}
          
          <v-spacer></v-spacer>
          
          <v-btn icon="mdi-close" variant="text" @click="logsDialog = false"></v-btn>
        </v-card-title>

        <v-card-text class="pa-0">
          <v-sheet
            color="grey-darken-4"
            class="pa-4"
            style="max-height: 600px; overflow-y: auto;"
          >
            <pre style="color: white; margin: 0; font-size: 0.85em; white-space: pre-wrap;">{{ containerLogs }}</pre>
          </v-sheet>
        </v-card-text>

        <v-card-actions>
          <v-btn
            prepend-icon="mdi-refresh"
            @click="refreshLogs"
            :loading="loadingLogs"
          >
            Обновить
          </v-btn>
          <v-spacer></v-spacer>
          <v-btn @click="logsDialog = false">Закрыть</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-card>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useToast } from 'vue-toastification'
import api from '@/services/api'

const toast = useToast()

const loading = ref(false)
const dockerStatus = ref({ enabled: true, running: false, containers: [] })
const containers = ref([])
const restartingAll = ref(false)
const startingAll = ref(false)
const stoppingAll = ref(false)
const restartingContainer = ref(null)

const confirmDialog = ref(false)
const confirmMessage = ref('')
const confirmAction = ref('')

const logsDialog = ref(false)
const selectedContainer = ref(null)
const containerLogs = ref('')
const loadingLogs = ref(false)

// Загрузка статуса
const loadStatus = async () => {
  loading.value = true
  try {
    const response = await api.get('/docker/status')
    if (response.data.success) {
      dockerStatus.value = response.data.docker
      containers.value = response.data.docker.containers || []
    }
  } catch (error) {
    toast.error('Не удалось загрузить статус Docker')
    console.error('Docker status error:', error)
  } finally {
    loading.value = false
  }
}

// Получить цвет для статуса
const getStatusColor = (state) => {
  switch (state?.toLowerCase()) {
    case 'running': return 'success'
    case 'starting': return 'info'
    case 'stopping': return 'warning'
    case 'stopped': return 'error'
    case 'exited': return 'error'
    default: return 'grey'
  }
}

// Подтверждение перезапуска всех
const confirmRestartAll = () => {
  confirmMessage.value = 'Вы уверены что хотите перезапустить все Docker контейнеры?\n\nСистема будет недоступна 30-60 секунд.'
  confirmAction.value = 'restart-all'
  confirmDialog.value = true
}

// Подтверждение остановки всех
const confirmStopAll = () => {
  confirmMessage.value = 'Вы уверены что хотите ОСТАНОВИТЬ все Docker контейнеры?\n\nСистема перестанет работать!'
  confirmAction.value = 'stop'
  confirmDialog.value = true
}

// Выполнить подтвержденное действие
const executeConfirmedAction = async () => {
  confirmDialog.value = false
  
  if (confirmAction.value === 'restart-all') {
    await restartAll()
  } else if (confirmAction.value === 'stop') {
    await stopAll()
  }
}

// Перезапустить все
const restartAll = async () => {
  restartingAll.value = true
  try {
    const response = await api.post('/docker/restart/all')
    if (response.data.success) {
      toast.success('Все контейнеры перезапускаются...')
      
      // Подождать 5 секунд и обновить статус
      setTimeout(async () => {
        await loadStatus()
        toast.info('Контейнеры перезапущены')
      }, 5000)
    } else {
      toast.error(response.data.message || 'Ошибка перезапуска')
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка перезапуска'
    toast.error(errorMsg)
    console.error('Restart all error:', error)
  } finally {
    restartingAll.value = false
  }
}

// Запустить все
const startAll = async () => {
  startingAll.value = true
  try {
    const response = await api.post('/docker/start/all')
    if (response.data.success) {
      toast.success('Контейнеры запускаются...')
      setTimeout(loadStatus, 3000)
    } else {
      toast.error(response.data.message || 'Ошибка запуска')
    }
  } catch (error) {
    toast.error(error.response?.data?.error || 'Ошибка запуска')
  } finally {
    startingAll.value = false
  }
}

// Остановить все
const stopAll = async () => {
  stoppingAll.value = true
  try {
    const response = await api.post('/docker/stop/all')
    if (response.data.success) {
      toast.warning('Все контейнеры остановлены')
      setTimeout(loadStatus, 2000)
    } else {
      toast.error(response.data.message || 'Ошибка остановки')
    }
  } catch (error) {
    toast.error(error.response?.data?.error || 'Ошибка остановки')
  } finally {
    stoppingAll.value = false
  }
}

// Перезапустить контейнер
const restartContainer = async (container) => {
  if (!confirm(`Перезапустить контейнер ${container.service}?`)) {
    return
  }

  restartingContainer.value = container.service
  try {
    const response = await api.post('/docker/restart/container', {
      container: container.service
    })
    
    if (response.data.success) {
      toast.success(`Контейнер ${container.service} перезапускается...`)
      setTimeout(loadStatus, 3000)
    } else {
      toast.error(response.data.message || 'Ошибка перезапуска')
    }
  } catch (error) {
    toast.error(error.response?.data?.error || 'Ошибка перезапуска')
  } finally {
    restartingContainer.value = null
  }
}

// Показать логи
const showLogs = async (container) => {
  selectedContainer.value = container
  logsDialog.value = true
  await loadLogs()
}

// Загрузить логи
const loadLogs = async () => {
  if (!selectedContainer.value) return

  loadingLogs.value = true
  try {
    const response = await api.get('/docker/logs', {
      params: {
        container: selectedContainer.value.service,
        lines: 200
      }
    })
    
    if (response.data.success) {
      containerLogs.value = response.data.logs || 'Логи пусты'
    } else {
      containerLogs.value = 'Ошибка загрузки логов: ' + (response.data.error || 'Unknown')
    }
  } catch (error) {
    containerLogs.value = 'Ошибка: ' + (error.message || 'Failed to load logs')
  } finally {
    loadingLogs.value = false
  }
}

// Обновить логи
const refreshLogs = async () => {
  await loadLogs()
  toast.success('Логи обновлены')
}

// При монтировании
onMounted(() => {
  loadStatus()
  
  // Автообновление каждые 30 секунд
  setInterval(loadStatus, 30000)
})
</script>

<style scoped>
.bg-docker {
  background: linear-gradient(135deg, #0db7ed 0%, #0099cc 100%);
  color: white;
}

.bg-warning {
  background: linear-gradient(135deg, #f59e0b 0%, #f97316 100%);
  color: white;
}
</style>

