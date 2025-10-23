<template>
  <div>
    <!-- Текущие показания -->
    <v-card class="mb-4">
      <v-card-title class="bg-ec-gradient text-white">
        <v-icon icon="mdi-flash" class="mr-2"></v-icon>
        EC Контроллер
        <v-spacer></v-spacer>
        <v-chip :color="node.online ? 'success' : 'error'" size="small">
          {{ node.online ? 'Online' : 'Offline' }}
        </v-chip>
      </v-card-title>

      <v-card-text>
        <v-row>
          <v-col cols="4">
            <v-sheet color="orange" class="pa-4 rounded text-center">
              <div class="text-h4 text-white font-weight-bold">{{ currentEc }}</div>
              <div class="text-caption text-white">Текущий EC</div>
            </v-sheet>
          </v-col>
          <v-col cols="4">
            <v-sheet color="success" class="pa-4 rounded text-center">
              <div class="text-h4 text-white font-weight-bold">{{ targetEc }}</div>
              <div class="text-caption text-white">Целевой EC</div>
            </v-sheet>
          </v-col>
          <v-col cols="4">
            <v-sheet color="info" class="pa-4 rounded text-center">
              <div class="text-subtitle-1 text-white font-weight-bold">{{ ecRange }}</div>
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
                <v-icon :icon="getPumpIcon(pump.id)" 
                        :color="getPumpColor(pump.id)" 
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
                      :color="getPumpButtonColor(pump.id)"
                      :prepend-icon="getPumpButtonIcon(pump.id)"
                      @click="runningPump === pump.id ? stopPump(pump.id) : runPump(pump.id)"
                      :disabled="!node.online || loading || (runningPump !== null && runningPump !== pump.id)"
                      :loading="runningPump === pump.id"
                      :variant="runningPump === pump.id ? 'elevated' : 'flat'"
                    >
                      {{ getPumpButtonText(pump.id) }}
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
              <v-card variant="tonal" :color="getPumpColor(pump.id)">
                <v-card-subtitle>
                  <v-icon :icon="getPumpIcon(pump.id)" class="mr-2"></v-icon>
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
                        prepend-icon="mdi-auto-fix"
                        @click="calibratePump(pump.id)"
                        :disabled="!node.online || loading"
                        :loading="calibratingPump === pump.id"
                      >
                        Калибровать
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
import { axios as api } from '@/services/api'

const props = defineProps({
  node: {
    type: Object,
    required: true
  }
})

const toast = useToast()

const pumps = [
  { id: 0, name: 'Насос A (основной)' },
  { id: 1, name: 'Насос B (вторичный)' },
  { id: 2, name: 'Насос C (микроэлементы)' }
]

const pumpDuration = ref({ 0: 5, 1: 5, 2: 5 })
const calibrationVolume = ref({ 0: 100, 1: 100, 2: 100 })
const calibrationDuration = ref({ 0: 10, 1: 10, 2: 10 })
const pumpCalibrations = ref([])
const showCalibration = ref(false)
const loading = ref(false)
const runningPump = ref(null)
const calibratingPump = ref(null)
const requestingConfig = ref(false)
const pumpDebounce = ref({}) // Защита от дублирования запросов
const pumpTimers = ref({}) // Таймеры для отслеживания работы насосов
const pumpProgress = ref({}) // Прогресс работы насосов
const lastConfigRequest = ref(null)

// Вычисляемые значения
const currentEc = computed(() => {
  return props.node.last_telemetry?.data?.ec?.toFixed(2) || 'N/A'
})

const targetEc = computed(() => {
  return props.node.config?.ec_target?.toFixed(2) || 'N/A'
})

const ecRange = computed(() => {
  if (props.node.config?.ec_min && props.node.config?.ec_max) {
    return `${props.node.config.ec_min.toFixed(1)} - ${props.node.config.ec_max.toFixed(1)}`
  }
  return 'N/A'
})

// Получить иконку насоса
const getPumpIcon = (pumpId) => {
  const icons = { 0: 'mdi-alpha-a', 1: 'mdi-alpha-b', 2: 'mdi-alpha-c' }
  return icons[pumpId] || 'mdi-pump'
}

// Получить цвет насоса
const getPumpColor = (pumpId) => {
  const colors = { 0: 'orange', 1: 'amber', 2: 'yellow' }
  return colors[pumpId] || 'primary'
}

// Получить калибровку насоса
const getPumpCalibration = (pumpId) => {
  return pumpCalibrations.value.find(cal => cal.pump_id === pumpId)
}

// Функции для определения состояния кнопки насоса
const getPumpButtonColor = (pumpId) => {
  if (runningPump.value === pumpId) {
    return getPumpColor(pumpId)
  }
  return getPumpColor(pumpId)
}

const getPumpButtonIcon = (pumpId) => {
  if (runningPump.value === pumpId) {
    return 'mdi-stop'
  }
  return 'mdi-play'
}

const getPumpButtonText = (pumpId) => {
  if (runningPump.value === pumpId) {
    return 'Работает...'
  }
  if (runningPump.value !== null && runningPump.value !== pumpId) {
    return 'Другой насос работает'
  }
  return 'Запустить'
}

// Загрузка калибровок
const loadCalibrations = async () => {
  try {
    const response = await api.getPumpCalibrations(props.node.node_id)
    if (response.success) {
      pumpCalibrations.value = response.calibrations
    }
  } catch (error) {
    console.error('Failed to load calibrations:', error)
  }
}

// Функция для сброса состояния насоса
const resetPumpState = (pumpId) => {
  // Очищаем таймер прогресса
  if (pumpTimers.value[pumpId]) {
    clearTimeout(pumpTimers.value[pumpId])
    delete pumpTimers.value[pumpId]
  }
  
  // Сбрасываем состояние
  runningPump.value = null
  pumpProgress.value[pumpId] = 0
  pumpDebounce.value[pumpId] = false
}

// Функция для принудительной остановки насоса
const stopPump = async (pumpId) => {
  if (runningPump.value !== pumpId) return
  
  try {
    // Отправляем команду остановки на сервер
    await api.stopPump(props.node.node_id, pumpId)
    
    toast.info(`Насос ${pumpId} остановлен`)
  } catch (error) {
    console.warn('Не удалось отправить команду остановки:', error)
  } finally {
    // В любом случае сбрасываем состояние
    resetPumpState(pumpId)
  }
}

// Запуск насоса
const runPump = async (pumpId) => {
  if (!pumpDuration.value[pumpId] || pumpDuration.value[pumpId] <= 0) {
    toast.warning('Введите длительность')
    return
  }

  // Проверяем, не запущен ли уже другой насос
  if (runningPump.value !== null && runningPump.value !== pumpId) {
    toast.warning('Другой насос уже работает')
    return
  }

  // Защита от дублирования запросов
  if (runningPump.value === pumpId || pumpDebounce.value[pumpId]) {
    toast.warning('Насос уже запускается, подождите...')
    return
  }

  runningPump.value = pumpId
  pumpDebounce.value[pumpId] = true
  pumpProgress.value[pumpId] = 0
  
  // Очищаем предыдущий таймер если есть
  if (pumpTimers.value[pumpId]) {
    clearTimeout(pumpTimers.value[pumpId])
    delete pumpTimers.value[pumpId]
  }
  
  try {
    const response = await api.runPump(
      props.node.node_id,
      pumpId,
      pumpDuration.value[pumpId]
    )

    if (response.success) {
      toast.success(`Насос ${pumpId} запущен на ${pumpDuration.value[pumpId]} сек`)
      
      // Запускаем таймер прогресса только после успешного API вызова
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
          
          // Сбрасываем состояние после завершения
          setTimeout(() => {
            runningPump.value = null
            pumpProgress.value[pumpId] = 0
            pumpDebounce.value[pumpId] = false
          }, 500) // Небольшая задержка для плавности
        }
      }, interval)
      
    } else {
      const errorMsg = response.error || response.data?.error || 'Ошибка запуска насоса'
      toast.error(errorMsg)
      resetPumpState(pumpId)
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка запуска насоса'
    toast.error(errorMsg)
    resetPumpState(pumpId)
  }
}

// Калибровка насоса
const calibratePump = async (pumpId) => {
  if (!calibrationVolume.value[pumpId] || !calibrationDuration.value[pumpId]) {
    toast.warning('Введите объем и время')
    return
  }

  calibratingPump.value = pumpId
  try {
    const response = await api.calibratePump(
      props.node.node_id,
      pumpId,
      calibrationDuration.value[pumpId],
      calibrationVolume.value[pumpId]
    )

    if (response.success) {
      toast.success(`Насос ${pumpId} откалиброван: ${response.ml_per_second.toFixed(2)} мл/с`)
      await loadCalibrations()
    } else {
      toast.error(response.error || 'Ошибка калибровки')
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка калибровки'
    toast.error(errorMsg)
  } finally {
    calibratingPump.value = null
  }
}

// Запрос конфигурации
const requestConfig = async () => {
  requestingConfig.value = true
  try {
    const response = await api.requestConfig(props.node.node_id)

    if (response.success) {
      toast.success('Запрос конфигурации отправлен. Ожидайте ответа через WebSocket...')
      lastConfigRequest.value = new Date().toLocaleTimeString('ru-RU')
    } else {
      toast.error(response.error || 'Ошибка запроса конфигурации')
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

// Очистка таймеров при размонтировании
onUnmounted(() => {
  Object.values(pumpTimers.value).forEach(timer => {
    if (timer) {
      if (typeof timer === 'number') {
        clearTimeout(timer)
      } else {
        clearInterval(timer)
      }
    }
  })
  pumpTimers.value = {}
})
</script>

<style scoped>
.bg-ec-gradient {
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
}
</style>
