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
                
                <!-- Статус работы насоса -->
                <v-chip 
                  v-if="runningPump === pump.id" 
                  color="info" 
                  size="small"
                  class="mr-2"
                >
                  <v-icon icon="mdi-pulse" start size="x-small"></v-icon>
                  Работает
                </v-chip>
                
                <!-- Калибровка -->
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
              <v-card variant="tonal" :color="pump.id === 0 ? 'success' : 'error'">
                <v-card-subtitle>
                  <v-icon :icon="pump.id === 0 ? 'mdi-arrow-up-bold' : 'mdi-arrow-down-bold'" class="mr-2"></v-icon>
                  <strong>{{ pump.name }}</strong>
                </v-card-subtitle>

                <v-card-text>
                  <!-- Время калибровки и кнопка запуска -->
                  <v-row>
                    <v-col cols="6">
                      <v-text-field
                        v-model.number="calibrationDuration[pump.id]"
                        type="number"
                        label="Время калибровки"
                        variant="outlined"
                        density="compact"
                        suffix="сек"
                        :min="1"
                        :max="120"
                        hide-details
                      ></v-text-field>
                    </v-col>
                    <v-col cols="6">
                      <v-btn
                        block
                        color="primary"
                        :prepend-icon="calibratingPump === pump.id ? 'mdi-loading' : 'mdi-play'"
                        @click="startCalibration(pump.id)"
                        :disabled="!node.online || loading || calibratingPump !== null || runningPump !== null"
                        :loading="calibratingPump === pump.id"
                        :variant="calibratingPump === pump.id ? 'elevated' : 'flat'"
                      >
                        {{ calibratingPump === pump.id ? 'Калибровка...' : 'Запустить калибровку' }}
                      </v-btn>
                    </v-col>
                  </v-row>

                  <!-- Прогресс калибровки -->
                  <v-row v-if="calibratingPump === pump.id && pumpProgress[pump.id] > 0">
                    <v-col cols="12">
                      <v-progress-linear
                        :model-value="pumpProgress[pump.id]"
                        :color="pump.id === 0 ? 'success' : 'error'"
                        height="8"
                        rounded
                      ></v-progress-linear>
                      <div class="text-center text-caption mt-2">
                        Калибровка: {{ pumpProgress[pump.id].toFixed(1) }}%
                      </div>
                    </v-col>
                  </v-row>

                  <!-- Поле ввода объема после завершения калибровки -->
                  <v-row v-if="showVolumeInput[pump.id]">
                    <v-col cols="8">
                      <v-text-field
                        v-model.number="calibrationVolume[pump.id]"
                        type="number"
                        label="Введите объем в мл"
                        variant="outlined"
                        density="compact"
                        suffix="мл"
                        :min="0.1"
                        :max="1000"
                        step="0.1"
                        placeholder="Например: 100"
                        hide-details
                      ></v-text-field>
                    </v-col>
                    <v-col cols="4">
                      <v-btn
                        block
                        color="success"
                        prepend-icon="mdi-content-save"
                        @click="saveCalibration(pump.id)"
                        class="mt-1"
                      >
                        Сохранить
                      </v-btn>
                    </v-col>
                  </v-row>

                  <!-- Информация о последней калибровке -->
                  <v-alert
                    v-if="getPumpCalibration(pump.id)"
                    :type="getPumpCalibration(pump.id).is_calibrated ? 'success' : 'warning'"
                    variant="tonal"
                    density="compact"
                    class="mt-3"
                  >
                    <div class="d-flex align-center">
                      <v-icon class="me-2">
                        {{ getPumpCalibration(pump.id).is_calibrated ? 'mdi-check-circle' : 'mdi-alert-circle' }}
                      </v-icon>
                      <div>
                        <div class="font-weight-medium">
                          {{ getPumpCalibration(pump.id).is_calibrated ? 'Калиброван' : 'Не калиброван' }}
                        </div>
                        <div class="text-caption">
                          Производительность: {{ getPumpCalibration(pump.id).ml_per_second.toFixed(2) }} мл/с
                          <br>
                          Калиброван: {{ formatDate(getPumpCalibration(pump.id).calibrated_at) }}
                        </div>
                      </div>
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
import api from '@/services/api'

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
const showVolumeInput = ref({})

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

// Функции для определения состояния кнопки насоса
const getPumpButtonColor = (pumpId) => {
  if (runningPump.value === pumpId) {
    return pumpId === 0 ? 'success' : 'error'
  }
  return pumpId === 0 ? 'success' : 'error'
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
    console.log('Load calibrations response:', response)
    
    if (response.success || response.status === 200) {
      pumpCalibrations.value = response.calibrations || response
    } else {
      console.error('Failed to load calibrations:', response.error || response.message)
    }
  } catch (error) {
    console.error('Failed to load calibrations:', error)
  }
}

// Debounce для предотвращения множественных нажатий
const pumpDebounce = ref({})
const calibrationDebounce = ref({})

// Валидация форм
const validatePumpDuration = (value) => {
  if (!value || value <= 0) {
    return 'Длительность должна быть больше 0'
  }
  if (value > 30) {
    return 'Максимальная длительность: 30 секунд'
  }
  return true
}

const validateCalibrationVolume = (value) => {
  if (!value || value <= 0) {
    return 'Объем должен быть больше 0'
  }
  if (value > 500) {
    return 'Максимальный объем: 500 мл'
  }
  return true
}

const validateCalibrationDuration = (value) => {
  if (!value || value <= 0) {
    return 'Время должно быть больше 0'
  }
  if (value > 120) {
    return 'Максимальное время: 120 секунд'
  }
  return true
}

// Запуск насоса
const runPump = async (pumpId) => {
  // Валидация длительности
  const durationValidation = validatePumpDuration(pumpDuration.value[pumpId])
  if (durationValidation !== true) {
    toast.warning(durationValidation)
    return
  }

  // Проверяем debounce
  if (pumpDebounce.value[pumpId]) {
    toast.warning('Подождите, насос уже запускается...')
    return
  }

  // Проверяем, не запущен ли уже другой насос
  if (runningPump.value !== null && runningPump.value !== pumpId) {
    toast.warning('Другой насос уже работает')
    return
  }

  runningPump.value = pumpId
  pumpDebounce.value[pumpId] = true
  pumpProgress.value[pumpId] = 0
  
  // Очищаем предыдущий таймер если есть
  if (pumpTimers.value[pumpId]) {
    clearInterval(pumpTimers.value[pumpId])
    delete pumpTimers.value[pumpId]
  }
  
  try {
    console.log(`🚀 Запуск насоса ${pumpId} на ${pumpDuration.value[pumpId]} сек`)
    
    const response = await api.runPump(
      props.node.node_id,
      pumpId,
      pumpDuration.value[pumpId]
    )

    console.log('📡 API ответ:', response)

    // Проверяем успешность ответа (response уже извлечен из .data в interceptor)
    const isSuccess = response.success || response.status === 200
    
    if (isSuccess) {
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
      const errorMsg = response.error || response.message || 'Ошибка запуска насоса'
      toast.error(errorMsg)
      resetPumpState(pumpId)
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка запуска насоса'
    toast.error(errorMsg)
    resetPumpState(pumpId)
  }
}

// Функция для сброса состояния насоса
const resetPumpState = (pumpId) => {
  // Очищаем таймер прогресса
  if (pumpTimers.value[pumpId]) {
    clearInterval(pumpTimers.value[pumpId])
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

// Запуск калибровки насоса (включение насоса на время)
const startCalibration = async (pumpId) => {
  // Валидация времени
  const durationValidation = validateCalibrationDuration(calibrationDuration.value[pumpId])
  if (durationValidation !== true) {
    toast.warning(durationValidation)
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
    console.log('Starting calibration pump with data:', {
      nodeId: props.node.node_id,
      pumpId,
      duration: calibrationDuration.value[pumpId]
    })
    
    // Запускаем насос на время калибровки
    const response = await api.runPump(
      props.node.node_id,
      pumpId,
      calibrationDuration.value[pumpId]
    )

    if (response.success) {
      toast.success(`Насос ${pumpId} запущен для калибровки на ${calibrationDuration.value[pumpId]} сек`)
      
      // Запускаем таймер прогресса
      const duration = calibrationDuration.value[pumpId] * 1000 // в миллисекундах
      const interval = 100 // обновляем каждые 100мс
      let elapsed = 0
      
      pumpTimers.value[pumpId] = setInterval(() => {
        elapsed += interval
        const progress = Math.min((elapsed / duration) * 100, 100)
        pumpProgress.value[pumpId] = progress
        
        if (progress >= 100) {
          clearInterval(pumpTimers.value[pumpId])
          delete pumpTimers.value[pumpId]
          
          // Показываем поле ввода объема после завершения
          setTimeout(() => {
            showVolumeInput.value[pumpId] = true
            calibratingPump.value = null
            pumpProgress.value[pumpId] = 0
          }, 500)
        }
      }, interval)
    } else {
      toast.error(response.error || 'Ошибка запуска насоса')
      resetCalibrationState(pumpId)
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка запуска насоса'
    toast.error(errorMsg)
    resetCalibrationState(pumpId)
  }
}

// Сохранение калибровки с введенным объемом
const saveCalibration = async (pumpId) => {
  // Валидация объема
  const volumeValidation = validateCalibrationVolume(calibrationVolume.value[pumpId])
  if (volumeValidation !== true) {
    toast.warning(volumeValidation)
    return
  }

  try {
    console.log('Saving calibration with data:', {
      nodeId: props.node.node_id,
      pumpId,
      duration: calibrationDuration.value[pumpId],
      volume: calibrationVolume.value[pumpId]
    })
    
    // Отправляем конфиг на узел и сохраняем в БД
    const response = await api.calibratePump(
      props.node.node_id,
      pumpId,
      calibrationDuration.value[pumpId],
      calibrationVolume.value[pumpId]
    )

    console.log('Calibration response:', response)

    if (response.success || response.status === 200) {
      const mlPerSecond = response.ml_per_second || response.calibration?.ml_per_second || 0
      toast.success(`Калибровка насоса ${pumpId} сохранена: ${mlPerSecond.toFixed(2)} мл/с`)
      // Скрываем поле ввода
      showVolumeInput.value[pumpId] = false
      // Обновляем данные калибровки
      await loadCalibrations()
    } else {
      toast.error(response.error || response.message || 'Ошибка сохранения калибровки')
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка сохранения калибровки'
    toast.error(errorMsg)
  }
}

// Сброс состояния калибровки
const resetCalibrationState = (pumpId) => {
  calibratingPump.value = null
  showVolumeInput.value[pumpId] = false
  pumpProgress.value[pumpId] = 0
  calibrationDebounce.value[pumpId] = false
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
  
  // Если это строка ISO (например, "2025-10-23T11:09:41.000000Z")
  if (typeof timestamp === 'string') {
    return new Date(timestamp).toLocaleString('ru-RU')
  }
  
  // Если это Unix timestamp (число)
  if (typeof timestamp === 'number') {
    return new Date(timestamp * 1000).toLocaleString('ru-RU')
  }
  
  return 'N/A'
}

// При монтировании загрузить калибровки
onMounted(() => {
  loadCalibrations()
})

// При размонтировании очищаем таймеры
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
.bg-ph-gradient {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}
</style>
