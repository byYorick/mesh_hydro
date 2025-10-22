<template>
  <div>
    <!-- Текущие показания -->
    <v-card class="mb-4">
      <v-card-title class="bg-ph-gradient text-white">
        <v-icon icon="mdi-flask-outline" class="mr-2"></v-icon>
        pH Контроллер
        <v-spacer></v-spacer>
        <v-chip :color="node.online ? 'success' : 'error'" size="small">
          {{ node.online ? 'Online' : 'Offline' }}
        </v-chip>
      </v-card-title>

      <v-card-text>
        <v-row>
          <v-col cols="4">
            <v-sheet color="primary" class="pa-4 rounded text-center">
              <div class="text-h4 text-white font-weight-bold">{{ currentPh }}</div>
              <div class="text-caption text-white">Текущий pH</div>
            </v-sheet>
          </v-col>
          <v-col cols="4">
            <v-sheet color="success" class="pa-4 rounded text-center">
              <div class="text-h4 text-white font-weight-bold">{{ targetPh }}</div>
              <div class="text-caption text-white">Целевой pH</div>
            </v-sheet>
          </v-col>
          <v-col cols="4">
            <v-sheet color="info" class="pa-4 rounded text-center">
              <div class="text-subtitle-1 text-white font-weight-bold">{{ phRange }}</div>
              <div class="text-caption text-white">Диапазон</div>
            </v-sheet>
          </v-col>
        </v-row>
      </v-card-text>
    </v-card>

    <!-- Ручное управление насосами -->
    <v-card class="mb-4">
      <v-card-title>
        <v-icon icon="mdi-pump" class="mr-2"></v-icon>
        Ручное управление насосами
      </v-card-title>

      <v-card-text>
        <v-row v-for="pump in pumps" :key="pump.id" class="mb-4">
          <v-col cols="12">
            <v-card variant="outlined">
              <v-card-subtitle class="d-flex align-center">
                <v-icon :icon="pump.id === 0 ? 'mdi-arrow-up-bold' : 'mdi-arrow-down-bold'" 
                        :color="pump.id === 0 ? 'success' : 'error'" 
                        class="mr-2"></v-icon>
                <strong>{{ pump.name }}</strong>
                <v-spacer></v-spacer>
                <v-chip v-if="getPumpCalibration(pump.id)" color="success" size="small">
                  <v-icon icon="mdi-check-circle" start size="x-small"></v-icon>
                  {{ getPumpCalibration(pump.id).ml_per_second.toFixed(2) }} мл/с
                </v-chip>
                <v-chip v-else color="warning" size="small">
                  <v-icon icon="mdi-alert" start size="x-small"></v-icon>
                  Не откалиброван
                </v-chip>
              </v-card-subtitle>

              <v-card-text>
                <v-row align="center">
                  <v-col cols="6">
                    <v-text-field
                      v-model.number="pumpDuration[pump.id]"
                      type="number"
                      label="Длительность (сек)"
                      variant="outlined"
                      density="compact"
                      :min="0.1"
                      :max="30"
                      :step="0.1"
                      suffix="сек"
                      hide-details
                    ></v-text-field>
                  </v-col>
                  <v-col cols="6">
                    <v-btn
                      block
                      :color="pump.id === 0 ? 'success' : 'error'"
                      :prepend-icon="runningPump === pump.id ? 'mdi-stop' : 'mdi-play'"
                      @click="runPump(pump.id)"
                      :disabled="!node.online || loading || runningPump !== null"
                      :loading="runningPump === pump.id"
                      :variant="runningPump === pump.id ? 'elevated' : 'flat'"
                    >
                      {{ runningPump === pump.id ? 'Работает...' : 'Запустить' }}
                    </v-btn>
                  </v-col>
                </v-row>
                
                <!-- Прогресс-бар для работающего насоса -->
                <v-row v-if="runningPump === pump.id" class="mt-2">
                  <v-col cols="12">
                    <v-progress-linear
                      :model-value="pumpProgress[pump.id] || 0"
                      color="success"
                      height="6"
                      rounded
                    ></v-progress-linear>
                    <div class="text-caption text-center mt-1">
                      Насос работает: {{ Math.round((pumpProgress[pump.id] || 0) * (pumpDuration[pump.id] || 0) / 100) }}с / {{ pumpDuration[pump.id] || 0 }}с
                    </div>
                  </v-col>
                </v-row>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </v-card-text>
    </v-card>

    <!-- Калибровка насосов -->
    <v-card class="mb-4">
      <v-card-title>
        <v-icon icon="mdi-tune" class="mr-2"></v-icon>
        Калибровка насосов
        <v-spacer></v-spacer>
        <v-btn
          icon
          variant="text"
          @click="showCalibration = !showCalibration"
        >
          <v-icon>{{ showCalibration ? 'mdi-chevron-up' : 'mdi-chevron-down' }}</v-icon>
        </v-btn>
      </v-card-title>

      <v-expand-transition>
        <v-card-text v-show="showCalibration">
          <v-row v-for="pump in pumps" :key="'cal-' + pump.id" class="mb-4">
            <v-col cols="12">
              <v-card variant="tonal" :color="pump.id === 0 ? 'success' : 'error'">
                <v-card-subtitle>
                  <v-icon :icon="pump.id === 0 ? 'mdi-arrow-up-bold' : 'mdi-arrow-down-bold'" class="mr-2"></v-icon>
                  <strong>{{ pump.name }}</strong>
                </v-card-subtitle>

                <v-card-text>
                  <v-row>
                    <v-col cols="4">
                      <v-text-field
                        v-model.number="calibrationVolume[pump.id]"
                        type="number"
                        label="Объем"
                        variant="outlined"
                        density="compact"
                        suffix="мл"
                        :min="1"
                        :max="500"
                        hide-details
                      ></v-text-field>
                    </v-col>
                    <v-col cols="4">
                      <v-text-field
                        v-model.number="calibrationDuration[pump.id]"
                        type="number"
                        label="Время"
                        variant="outlined"
                        density="compact"
                        suffix="сек"
                        :min="1"
                        :max="120"
                        hide-details
                      ></v-text-field>
                    </v-col>
                    <v-col cols="4">
                      <v-btn
                        block
                        color="primary"
                        :prepend-icon="calibratingPump === pump.id ? 'mdi-stop' : 'mdi-auto-fix'"
                        @click="calibratePump(pump.id)"
                        :disabled="!node.online || loading || calibratingPump !== null || runningPump !== null"
                        :loading="calibratingPump === pump.id"
                        :variant="calibratingPump === pump.id ? 'elevated' : 'flat'"
                      >
                        {{ calibratingPump === pump.id ? 'Калибровка...' : 'Калибровать' }}
                      </v-btn>
                    </v-col>
                  </v-row>

                  <v-alert
                    v-if="getPumpCalibration(pump.id)"
                    type="success"
                    variant="tonal"
                    density="compact"
                    class="mt-3"
                  >
                    <div class="text-caption">
                      Последняя калибровка: {{ formatDate(getPumpCalibration(pump.id).last_calibrated) }}
                    </div>
                  </v-alert>
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-card-text>
      </v-expand-transition>
    </v-card>

    <!-- Запрос конфигурации -->
    <v-card>
      <v-card-title>
        <v-icon icon="mdi-cog-sync" class="mr-2"></v-icon>
        Конфигурация узла
      </v-card-title>

      <v-card-text>
        <v-btn
          block
          color="info"
          prepend-icon="mdi-download"
          @click="requestConfig"
          :disabled="!node.online"
          :loading="requestingConfig"
        >
          Запросить конфигурацию из NVS
        </v-btn>

        <v-alert
          v-if="lastConfigRequest"
          type="info"
          variant="tonal"
          class="mt-4"
        >
          <div class="text-caption">
            Последний запрос: {{ lastConfigRequest }}
          </div>
        </v-alert>
      </v-card-text>
    </v-card>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useToast } from 'vue-toastification'
import axios from 'axios'

const props = defineProps({
  node: {
    type: Object,
    required: true
  }
})

const toast = useToast()

const pumps = [
  { id: 0, name: 'Насос UP (повышение pH)' },
  { id: 1, name: 'Насос DOWN (понижение pH)' }
]

const pumpDuration = ref({ 0: 5, 1: 5 })
const calibrationVolume = ref({ 0: 100, 1: 100 })
const calibrationDuration = ref({ 0: 10, 1: 10 })
const pumpCalibrations = ref([])
const showCalibration = ref(false)
const loading = ref(false)
const runningPump = ref(null)
const calibratingPump = ref(null)
const requestingConfig = ref(false)
const lastConfigRequest = ref(null)
const pumpProgress = ref({})
const pumpTimers = ref({})

// Вычисляемые значения
const currentPh = computed(() => {
  return props.node.last_telemetry?.data?.ph?.toFixed(2) || 'N/A'
})

const targetPh = computed(() => {
  return props.node.config?.ph_target?.toFixed(2) || 'N/A'
})

const phRange = computed(() => {
  if (props.node.config?.ph_min && props.node.config?.ph_max) {
    return `${props.node.config.ph_min.toFixed(1)} - ${props.node.config.ph_max.toFixed(1)}`
  }
  return 'N/A'
})

// Получить калибровку насоса
const getPumpCalibration = (pumpId) => {
  return pumpCalibrations.value.find(cal => cal.pump_id === pumpId)
}

// Загрузка калибровок
const loadCalibrations = async () => {
  try {
    const response = await axios.get(`/api/nodes/${props.node.node_id}/pump/calibrations`)
    if (response.data.success) {
      pumpCalibrations.value = response.data.calibrations
    }
  } catch (error) {
    console.error('Failed to load calibrations:', error)
  }
}

// Debounce для предотвращения множественных нажатий
const pumpDebounce = ref({})
const calibrationDebounce = ref({})

// Запуск насоса
const runPump = async (pumpId) => {
  if (!pumpDuration.value[pumpId] || pumpDuration.value[pumpId] <= 0) {
    toast.warning('Введите длительность')
    return
  }

  // Проверяем debounce
  if (pumpDebounce.value[pumpId]) {
    toast.warning('Подождите, насос уже запускается...')
    return
  }

  runningPump.value = pumpId
  pumpDebounce.value[pumpId] = true
  pumpProgress.value[pumpId] = 0
  
  // Запускаем таймер прогресса
  const duration = pumpDuration.value[pumpId] * 1000 // в миллисекундах
  const interval = 100 // обновляем каждые 100мс
  let elapsed = 0
  
  pumpTimers.value[pumpId] = setInterval(() => {
    elapsed += interval
    const progress = Math.min((elapsed / duration) * 100, 100)
    pumpProgress.value[pumpId] = progress
    
    if (progress >= 100) {
      clearInterval(pumpTimers.value[pumpId])
      delete pumpTimers.value[pumpId]
    }
  }, interval)
  
  try {
    const response = await axios.post(`/api/nodes/${props.node.node_id}/pump/run`, {
      pump_id: pumpId,
      duration_sec: pumpDuration.value[pumpId]
    })

    if (response.data.success) {
      toast.success(`Насос ${pumpId} запущен на ${pumpDuration.value[pumpId]} сек`)
    } else {
      toast.error(response.data.error || 'Ошибка запуска насоса')
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка запуска насоса'
    toast.error(errorMsg)
  } finally {
    // Очищаем таймер и сбрасываем состояние
    if (pumpTimers.value[pumpId]) {
      clearInterval(pumpTimers.value[pumpId])
      delete pumpTimers.value[pumpId]
    }
    
    // Задержка перед сбросом состояния для показа завершения
    setTimeout(() => {
      runningPump.value = null
      pumpProgress.value[pumpId] = 0
      pumpDebounce.value[pumpId] = false
    }, 1000)
  }
}

// Калибровка насоса
const calibratePump = async (pumpId) => {
  if (!calibrationVolume.value[pumpId] || !calibrationDuration.value[pumpId]) {
    toast.warning('Введите объем и время')
    return
  }

  // Проверяем debounce
  if (calibrationDebounce.value[pumpId]) {
    toast.warning('Подождите, калибровка уже выполняется...')
    return
  }

  calibratingPump.value = pumpId
  calibrationDebounce.value[pumpId] = true
  
  try {
    const response = await axios.post(`/api/nodes/${props.node.node_id}/pump/calibrate`, {
      pump_id: pumpId,
      volume_ml: calibrationVolume.value[pumpId],
      duration_sec: calibrationDuration.value[pumpId]
    })

    if (response.data.success) {
      toast.success(`Насос ${pumpId} откалиброван: ${response.data.ml_per_second.toFixed(2)} мл/с`)
      await loadCalibrations()
    } else {
      toast.error(response.data.error || 'Ошибка калибровки')
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка калибровки'
    toast.error(errorMsg)
  } finally {
    calibratingPump.value = null
    // Сбрасываем debounce через 3 секунды (калибровка занимает больше времени)
    setTimeout(() => {
      calibrationDebounce.value[pumpId] = false
    }, 3000)
  }
}

// Запрос конфигурации
const requestConfig = async () => {
  requestingConfig.value = true
  try {
    const response = await axios.post(`/api/nodes/${props.node.node_id}/config/request`)

    if (response.data.success) {
      toast.success('Запрос конфигурации отправлен. Ожидайте ответа через WebSocket...')
      lastConfigRequest.value = new Date().toLocaleTimeString('ru-RU')
    } else {
      toast.error(response.data.error || 'Ошибка запроса конфигурации')
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка запроса'
    toast.error(errorMsg)
  } finally {
    requestingConfig.value = false
  }
}

// Форматирование даты
const formatDate = (timestamp) => {
  if (!timestamp) return 'N/A'
  return new Date(timestamp * 1000).toLocaleString('ru-RU')
}

// При монтировании загрузить калибровки
onMounted(() => {
  loadCalibrations()
})

// При размонтировании очищаем таймеры
onUnmounted(() => {
  Object.values(pumpTimers.value).forEach(timer => {
    if (timer) clearInterval(timer)
  })
})
</script>

<style scoped>
.bg-ph-gradient {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}
</style>
