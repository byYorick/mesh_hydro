<template>
  <div class="ph-detail-wrapper">
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

    <!-- Управление конфигурацией -->
    <v-card class="mb-4">
      <v-card-title>
        <v-icon icon="mdi-cog" class="mr-2"></v-icon>
        Управление конфигурацией
        <v-spacer></v-spacer>
        <v-btn
          icon
          variant="text"
          @click="showConfig = !showConfig"
        >
          <v-icon>{{ showConfig ? 'mdi-chevron-up' : 'mdi-chevron-down' }}</v-icon>
        </v-btn>
      </v-card-title>

      <v-expand-transition>
        <v-card-text v-show="showConfig">
          <v-form @submit.prevent="saveConfig">
            <!-- Параметры pH -->
            <v-row>
              <v-col cols="12">
                <v-card variant="tonal" color="primary">
                  <v-card-subtitle>
                    <v-icon icon="mdi-flask-outline" class="mr-2"></v-icon>
                    <strong>Параметры pH</strong>
                  </v-card-subtitle>
                  <v-card-text>
                    <v-row>
                      <v-col cols="4">
                        <v-text-field
                          v-model.number="configForm.ph_target"
                          type="number"
                          label="Целевой pH"
                          variant="outlined"
                          density="compact"
                          :min="5.0"
                          :max="9.0"
                          :step="0.1"
                          :rules="[v => v >= 5.0 && v <= 9.0 || 'Диапазон: 5.0-9.0']"
                          hide-details="auto"
                        ></v-text-field>
                      </v-col>
                      <v-col cols="4">
                        <v-text-field
                          v-model.number="configForm.ph_min"
                          type="number"
                          label="Минимальный pH"
                          variant="outlined"
                          density="compact"
                          :min="4.0"
                          :max="8.0"
                          :step="0.1"
                          :rules="[v => v >= 4.0 && v <= 8.0 || 'Диапазон: 4.0-8.0']"
                          hide-details="auto"
                        ></v-text-field>
                      </v-col>
                      <v-col cols="4">
                        <v-text-field
                          v-model.number="configForm.ph_max"
                          type="number"
                          label="Максимальный pH"
                          variant="outlined"
                          density="compact"
                          :min="6.0"
                          :max="10.0"
                          :step="0.1"
                          :rules="[v => v >= 6.0 && v <= 10.0 || 'Диапазон: 6.0-10.0']"
                          hide-details="auto"
                        ></v-text-field>
                      </v-col>
                    </v-row>
                  </v-card-text>
                </v-card>
              </v-col>
            </v-row>

            <!-- Параметры PID -->
            <v-row class="mt-4">
              <v-col cols="12">
                <v-card variant="tonal" color="info">
                  <v-card-subtitle>
                    <v-icon icon="mdi-chart-line" class="mr-2"></v-icon>
                    <strong>Параметры PID контроллера</strong>
                  </v-card-subtitle>
                  <v-card-text>
                    <v-row>
                      <v-col cols="4">
                        <v-text-field
                          v-model.number="configForm.pid_params.kp"
                          type="number"
                          label="Kp (пропорциональный)"
                          variant="outlined"
                          density="compact"
                          :min="0.0"
                          :max="10.0"
                          :step="0.01"
                          hide-details="auto"
                        ></v-text-field>
                      </v-col>
                      <v-col cols="4">
                        <v-text-field
                          v-model.number="configForm.pid_params.ki"
                          type="number"
                          label="Ki (интегральный)"
                          variant="outlined"
                          density="compact"
                          :min="0.0"
                          :max="10.0"
                          :step="0.01"
                          hide-details="auto"
                        ></v-text-field>
                      </v-col>
                      <v-col cols="4">
                        <v-text-field
                          v-model.number="configForm.pid_params.kd"
                          type="number"
                          label="Kd (дифференциальный)"
                          variant="outlined"
                          density="compact"
                          :min="0.0"
                          :max="10.0"
                          :step="0.01"
                          hide-details="auto"
                        ></v-text-field>
                      </v-col>
                    </v-row>
                  </v-card-text>
                </v-card>
              </v-col>
            </v-row>

            <!-- Кнопки управления -->
            <v-row class="mt-4">
              <v-col cols="12" class="d-flex justify-end gap-2">
                <v-btn
                  variant="outlined"
                  prepend-icon="mdi-refresh"
                  @click="loadNodeConfig"
                  :disabled="!node.online || configLoading"
                >
                  Сбросить
                </v-btn>
                <v-btn
                  type="submit"
                  color="success"
                  prepend-icon="mdi-content-save"
                  :disabled="!node.online || configSaving"
                  :loading="configSaving"
                >
                  Сохранить конфигурацию
                </v-btn>
              </v-col>
            </v-row>

            <!-- Информация о последнем обновлении -->
            <v-alert
              v-if="configUpdateStatus"
              :type="configUpdateStatus.type"
              variant="tonal"
              density="compact"
              class="mt-4"
            >
              <div class="d-flex align-center">
                <v-icon class="me-2">{{ configUpdateStatus.icon }}</v-icon>
                <div>
                  <div class="font-weight-medium">{{ configUpdateStatus.title }}</div>
                  <div class="text-caption">
                    {{ configUpdateStatus.message }}
                  </div>
                </div>
                <v-spacer></v-spacer>
                <v-progress-circular
                  v-if="configUpdateStatus.loading"
                  indeterminate
                  size="20"
                  width="2"
                  class="ml-2"
                ></v-progress-circular>
              </div>
            </v-alert>
            
            <!-- Последнее обновление -->
            <v-alert
              v-if="node.config && node.config.updated_at && !configUpdateStatus"
              type="info"
              variant="tonal"
              density="compact"
              class="mt-4"
            >
              <div class="d-flex align-center">
                <v-icon class="me-2">mdi-information</v-icon>
                <div>
                  <div class="font-weight-medium">Последнее обновление конфигурации</div>
                  <div class="text-caption">
                    {{ formatDate(node.config.updated_at) }}
                  </div>
                </div>
              </div>
            </v-alert>
          </v-form>
        </v-card-text>
      </v-expand-transition>
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
  },
  telemetryData: {
    type: Array,
    default: () => []
  },
  nodeErrors: {
    type: Array,
    default: () => []
  }
})

const toast = useToast()

// Pump control variables
const pumps = ref([
  { id: 0, name: 'Насос UP (повышение pH)' },
  { id: 1, name: 'Насос DOWN (понижение pH)' }
])
const pumpDuration = ref({ 0: 5, 1: 5 })
const calibrationVolume = ref({ 0: 100, 1: 100 })
const calibrationDuration = ref({ 0: 10, 1: 10 })
const pumpCalibrations = ref([])
const showCalibration = ref(false)
const runningPump = ref(null)
const calibratingPump = ref(null)
const pumpProgress = ref({})
const pumpTimers = ref({})
const showVolumeInput = ref({})
const pumpDebounce = ref({})
const calibrationDebounce = ref({})
const loading = ref(false)

// Config control variables
const showConfig = ref(false)
const configLoading = ref(false)
const configSaving = ref(false)
const configUpdateStatus = ref(null)
const configSaveDebounce = ref(false)
const configForm = ref({
  ph_target: 6.5,
  ph_min: 6.0,
  ph_max: 7.0,
  pid_params: {
    kp: 1.0,
    ki: 0.1,
    kd: 0.05
  }
})

// Computed values
const currentPh = computed(() => {
  // Проверяем все возможные источники данных телеметрии
  const ph = props.node.last_telemetry?.data?.ph 
    || props.node.lastTelemetry?.data?.ph
    || props.node.telemetry?.[0]?.data?.ph
    || props.telemetryData?.[0]?.data?.ph
  
  return ph?.toFixed(2) || 'N/A'
})

const targetPh = computed(() => {
  const ph_target = props.node.config?.ph_target
    || props.node.last_telemetry?.data?.ph_target
    || props.node.lastTelemetry?.data?.ph_target
    || props.node.telemetry?.[0]?.data?.ph_target
    || props.telemetryData?.[0]?.data?.ph_target
  
  return ph_target?.toFixed(2) || 'N/A'
})

const phRange = computed(() => {
  const min = props.node.config?.ph_min || '6.0'
  const max = props.node.config?.ph_max || '7.0'
  return `${min} - ${max}`
})

// Functions
const getPumpCalibration = (pumpId) => {
  return pumpCalibrations.value.find(cal => cal.pump_id === pumpId)
}

const getPumpButtonColor = (pumpId) => {
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

const runPump = async (pumpId) => {
  if (!props.node.online) {
    toast.error('❌ Узел офлайн. Проверьте подключение.')
    return
  }

  if (pumpDebounce.value[pumpId]) {
    toast.warning('Подождите, насос уже запускается...')
    return
  }

  if (runningPump.value !== null && runningPump.value !== pumpId) {
    toast.warning('Другой насос уже работает')
    return
  }

  runningPump.value = pumpId
  pumpDebounce.value[pumpId] = true
  pumpProgress.value[pumpId] = 0

  if (pumpTimers.value[pumpId]) {
    clearInterval(pumpTimers.value[pumpId])
    delete pumpTimers.value[pumpId]
  }

  try {
    const response = await api.runPump(
      props.node.node_id,
      pumpId,
      pumpDuration.value[pumpId]
    )

    if (response.success || response.status === 200) {
      toast.success(`Насос ${pumpId} запущен на ${pumpDuration.value[pumpId]} сек`)

      const duration = pumpDuration.value[pumpId] * 1000
      const interval = 100
      let elapsed = 0

      pumpTimers.value[pumpId] = setInterval(() => {
        elapsed += interval
        const progress = Math.min((elapsed / duration) * 100, 100)
        pumpProgress.value[pumpId] = progress

        if (progress >= 100) {
          clearInterval(pumpTimers.value[pumpId])
          delete pumpTimers.value[pumpId]

          setTimeout(() => {
            runningPump.value = null
            pumpProgress.value[pumpId] = 0
            pumpDebounce.value[pumpId] = false
          }, 500)
        }
      }, interval)
    } else {
      const errorMsg = response.error || response.message || 'Ошибка запуска насоса'
      toast.error(`❌ ${errorMsg}`)
      resetPumpState(pumpId)
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка запуска насоса'
    toast.error(`❌ ${errorMsg}`)
    resetPumpState(pumpId)
  }
}

const stopPump = async (pumpId) => {
  if (runningPump.value !== pumpId) return

  try {
    await api.stopPump(props.node.node_id, pumpId)
    toast.info(`Насос ${pumpId} остановлен`)
  } catch (error) {
    console.warn('Не удалось отправить команду остановки:', error)
  } finally {
    resetPumpState(pumpId)
  }
}

const resetPumpState = (pumpId) => {
  if (pumpTimers.value[pumpId]) {
    clearInterval(pumpTimers.value[pumpId])
    delete pumpTimers.value[pumpId]
  }

  runningPump.value = null
  pumpProgress.value[pumpId] = 0
  pumpDebounce.value[pumpId] = false
}

const startCalibration = async (pumpId) => {
  if (calibrationDebounce.value[pumpId]) {
    toast.warning('Подождите, калибровка уже выполняется...')
    return
  }

  calibratingPump.value = pumpId
  calibrationDebounce.value[pumpId] = true

  try {
    const response = await api.runPump(
      props.node.node_id,
      pumpId,
      calibrationDuration.value[pumpId]
    )

    if (response.success) {
      toast.success(`Насос ${pumpId} запущен для калибровки на ${calibrationDuration.value[pumpId]} сек`)

      const duration = calibrationDuration.value[pumpId] * 1000
      const interval = 100
      let elapsed = 0

      pumpTimers.value[pumpId] = setInterval(() => {
        elapsed += interval
        const progress = Math.min((elapsed / duration) * 100, 100)
        pumpProgress.value[pumpId] = progress

        if (progress >= 100) {
          clearInterval(pumpTimers.value[pumpId])
          delete pumpTimers.value[pumpId]

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

const saveCalibration = async (pumpId) => {
  try {
    const response = await api.calibratePump(
      props.node.node_id,
      pumpId,
      calibrationDuration.value[pumpId],
      calibrationVolume.value[pumpId]
    )

    if (response.success || response.status === 200) {
      const mlPerSecond = response.ml_per_second || response.calibration?.ml_per_second || 0
      toast.success(`Калибровка насоса ${pumpId} сохранена: ${mlPerSecond.toFixed(2)} мл/с`)
      showVolumeInput.value[pumpId] = false
      await loadCalibrations()
    } else {
      toast.error(response.error || response.message || 'Ошибка сохранения калибровки')
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка сохранения калибровки'
    toast.error(errorMsg)
  }
}

const resetCalibrationState = (pumpId) => {
  calibratingPump.value = null
  showVolumeInput.value[pumpId] = false
  pumpProgress.value[pumpId] = 0
  calibrationDebounce.value[pumpId] = false
}

const loadCalibrations = async () => {
  try {
    const response = await api.getPumpCalibrations(props.node.node_id)
    if (response.success || response.status === 200) {
      pumpCalibrations.value = response.calibrations || response
    }
  } catch (error) {
    console.error('Failed to load calibrations:', error)
  }
}

const formatDate = (dateString) => {
  if (!dateString) return 'Никогда'
  const date = new Date(dateString)
  return date.toLocaleDateString('ru-RU', {
    day: '2-digit',
    month: '2-digit',
    year: 'numeric',
    hour: '2-digit',
    minute: '2-digit'
  })
}

// Config management functions
const loadNodeConfig = async () => {
  configLoading.value = true
  
  try {
    // Загружаем конфигурацию из props.node.config
    if (props.node.config) {
      configForm.value.ph_target = props.node.config.ph_target || 6.5
      configForm.value.ph_min = props.node.config.ph_min || 6.0
      configForm.value.ph_max = props.node.config.ph_max || 7.0
      
      if (props.node.config.pid_params) {
        configForm.value.pid_params.kp = props.node.config.pid_params.kp || 1.0
        configForm.value.pid_params.ki = props.node.config.pid_params.ki || 0.1
        configForm.value.pid_params.kd = props.node.config.pid_params.kd || 0.05
      }
    }
  } catch (error) {
    console.error('Failed to load config:', error)
    toast.error('❌ Не удалось загрузить конфигурацию')
  } finally {
    configLoading.value = false
  }
}

const saveConfig = async () => {
  if (!props.node.online) {
    toast.error('❌ Узел офлайн. Проверьте подключение.')
    return
  }

  // Защита от дублирования
  if (configSaveDebounce.value) {
    toast.warning('⏳ Подождите, сохранение уже выполняется...')
    return
  }

  configSaving.value = true
  configSaveDebounce.value = true
  configUpdateStatus.value = null

  try {
    // Валидация
    if (configForm.value.ph_target < 5.0 || configForm.value.ph_target > 9.0) {
      toast.error('❌ Целевой pH должен быть в диапазоне 5.0-9.0')
      return
    }
    if (configForm.value.ph_min < 4.0 || configForm.value.ph_min > 8.0) {
      toast.error('❌ Минимальный pH должен быть в диапазоне 4.0-8.0')
      return
    }
    if (configForm.value.ph_max < 6.0 || configForm.value.ph_max > 10.0) {
      toast.error('❌ Максимальный pH должен быть в диапазоне 6.0-10.0')
      return
    }
    if (configForm.value.ph_min >= configForm.value.ph_target) {
      toast.error('❌ Минимальный pH должен быть меньше целевого')
      return
    }
    if (configForm.value.ph_max <= configForm.value.ph_target) {
      toast.error('❌ Максимальный pH должен быть больше целевого')
      return
    }

    // Показываем статус отправки
    configUpdateStatus.value = {
      type: 'info',
      icon: 'mdi-send',
      title: 'Отправка конфигурации',
      message: 'Конфигурация отправляется на сервер...',
      loading: true
    }

    // Отправка конфигурации на backend
    const response = await api.updateNodeConfig(props.node.node_id, {
      ph_target: configForm.value.ph_target,
      ph_min: configForm.value.ph_min,
      ph_max: configForm.value.ph_max,
      pid_params: {
        kp: configForm.value.pid_params.kp,
        ki: configForm.value.pid_params.ki,
        kd: configForm.value.pid_params.kd
      }
    })

    if (response.success || response.status === 200) {
      // Показываем статус ожидания подтверждения от узла
      configUpdateStatus.value = {
        type: 'warning',
        icon: 'mdi-clock-outline',
        title: 'Ожидание подтверждения',
        message: 'Конфигурация отправлена. Ожидание подтверждения от узла...',
        loading: true
      }

      toast.success('✅ Конфигурация сохранена и отправлена на узел')
      
      // Обновляем локальные данные
      if (props.node.config) {
        Object.assign(props.node.config, {
          ph_target: configForm.value.ph_target,
          ph_min: configForm.value.ph_min,
          ph_max: configForm.value.ph_max,
          pid_params: configForm.value.pid_params
        })
      }

      // Ждем подтверждения от узла (timeout 30 секунд)
      setTimeout(() => {
        if (configUpdateStatus.value && configUpdateStatus.value.loading) {
          configUpdateStatus.value = {
            type: 'warning',
            icon: 'mdi-alert',
            title: 'Подтверждение не получено',
            message: 'Конфигурация отправлена, но узел не подтвердил получение. Проверьте логи узла.',
            loading: false
          }
          
          // Автоматически скрыть через 10 секунд
          setTimeout(() => {
            configUpdateStatus.value = null
          }, 10000)
        }
      }, 30000)
    } else {
      configUpdateStatus.value = {
        type: 'error',
        icon: 'mdi-alert-circle',
        title: 'Ошибка отправки',
        message: response.error || response.message || 'Не удалось отправить конфигурацию',
        loading: false
      }
      toast.error(response.error || response.message || '❌ Ошибка сохранения конфигурации')
      
      // Автоматически скрыть через 5 секунд
      setTimeout(() => {
        configUpdateStatus.value = null
      }, 5000)
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.response?.data?.message || error.message || 'Ошибка сохранения конфигурации'
    
    configUpdateStatus.value = {
      type: 'error',
      icon: 'mdi-alert-circle',
      title: 'Ошибка',
      message: errorMsg,
      loading: false
    }
    
    toast.error(`❌ ${errorMsg}`)
    console.error('Failed to save config:', error)
    
    // Автоматически скрыть через 5 секунд
    setTimeout(() => {
      configUpdateStatus.value = null
    }, 5000)
  } finally {
    configSaving.value = false
    // Снимаем блокировку через 2 секунды
    setTimeout(() => {
      configSaveDebounce.value = false
    }, 2000)
  }
}

onMounted(async () => {
  if (props.node && props.node.node_id) {
    await loadCalibrations()
    await loadNodeConfig()
    
    // Подписка на события обновления конфигурации узла
    const echo = window.Echo
    if (echo) {
      echo.channel('hydro')
        .listen('.node.config.updated', (event) => {
          // Проверяем, что событие для нашего узла
          if (event.node_id === props.node.node_id) {
            console.log('Config update confirmed from node:', event)
            
            // Показываем успех
            configUpdateStatus.value = {
              type: 'success',
              icon: 'mdi-check-circle',
              title: 'Конфигурация применена',
              message: 'Узел подтвердил получение и применение конфигурации',
              loading: false
            }
            
            toast.success('✅ Узел подтвердил применение конфигурации')
            
            // Обновляем конфигурацию в node
            if (props.node.config && event.config) {
              Object.assign(props.node.config, event.config)
            }
            
            // Автоматически скрыть через 5 секунд
            setTimeout(() => {
              configUpdateStatus.value = null
            }, 5000)
          }
        })
    }
  }
})

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
.ph-detail-wrapper {
  max-height: 100%;
  overflow-y: auto;
}

/* Компактный вид когда применяется класс .ph-detail-compact */
.ph-detail-compact .ph-detail-wrapper .v-card {
  margin-bottom: 8px;
}

.ph-detail-compact .ph-detail-wrapper .v-card-title {
  padding: 8px 16px;
  font-size: 14px;
}

.ph-detail-compact .ph-detail-wrapper .v-card-text {
  padding: 8px 16px;
  font-size: 13px;
}

.ph-detail-compact .ph-detail-wrapper .v-btn {
  min-height: 36px;
  font-size: 12px;
  padding: 0 12px;
}

.ph-detail-compact .ph-detail-wrapper .text-h4 {
  font-size: 1.5rem;
}

.ph-detail-compact .ph-detail-wrapper .v-row {
  margin: 0 -4px;
}

.ph-detail-compact .ph-detail-wrapper .v-col {
  padding: 4px;
}

.ph-detail-compact .ph-detail-wrapper .v-sheet {
  padding: 8px;
}
</style>
