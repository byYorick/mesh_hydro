<template>
  <v-dialog v-model="dialog" max-width="600" persistent>
    <v-card>
      <v-card-title class="bg-primary text-white">
        <v-icon icon="mdi-medical-bag" class="mr-2"></v-icon>
        Калибровка насоса
      </v-card-title>

      <v-card-text class="pt-4">
        <!-- Selection Form -->
        <div v-if="!isCalibrating && !calibrationResult">
          <v-select
            v-model="form.pump_id"
            :items="pumpOptions"
            item-title="label"
            item-value="value"
            label="Выберите насос"
            variant="outlined"
            :disabled="loading"
            class="mb-4"
          ></v-select>

          <v-text-field
            v-model.number="form.volume_ml"
            label="Объем жидкости (мл)"
            type="number"
            step="0.1"
            min="0.1"
            max="1000"
            variant="outlined"
            hint="Введите объем жидкости, которую насос должен перекачать"
            persistent-hint
            :rules="[rules.volume]"
            :disabled="loading"
            class="mb-4"
          ></v-text-field>

          <v-text-field
            v-model.number="form.duration_sec"
            label="Длительность (секунды)"
            type="number"
            step="0.1"
            min="1"
            max="60"
            variant="outlined"
            hint="Время работы насоса для калибровки"
            persistent-hint
            :rules="[rules.duration]"
            :disabled="loading"
          ></v-text-field>

          <v-alert type="info" variant="tonal" class="mt-4">
            <strong>Внимание!</strong> Убедитесь, что узел подключен и находится в режиме калибровки.
            Насос будет запущен на указанное время.
          </v-alert>
        </div>

        <!-- Calibrating Progress -->
        <div v-if="isCalibrating" class="text-center py-8">
          <v-progress-circular
            indeterminate
            size="64"
            color="primary"
            class="mb-4"
          ></v-progress-circular>
          
          <h3 class="mb-2">Калибровка в процессе...</h3>
          <p class="text-disabled">
            Пожалуйста, подождите. Насос работает.
          </p>
          
          <v-progress-linear
            :model-value="calibrationProgress"
            color="primary"
            height="24"
            rounded
            class="mt-4"
          >
            <template v-slot:default="{ value }">
              <strong>{{ Math.round(value) }}%</strong>
            </template>
          </v-progress-linear>
          
          <p class="text-caption mt-2">
            Осталось: {{ timeRemaining }} сек
          </p>
        </div>

        <!-- Results -->
        <div v-if="calibrationResult" class="text-center py-4">
          <v-icon icon="mdi-check-circle" color="success" size="64" class="mb-4"></v-icon>
          
          <h3 class="mb-4">Калибровка завершена!</h3>
          
          <v-card variant="outlined" class="mb-4">
            <v-card-text>
              <v-row>
                <v-col cols="6">
                  <div class="text-caption text-disabled">Производительность</div>
                  <div class="text-h5 text-primary">
                    {{ formatMlPerSecond(calibrationResult?.ml_per_second) }}
                  </div>
                </v-col>
                <v-col cols="6">
                  <div class="text-caption text-disabled">Объем</div>
                  <div class="text-h6">
                    {{ form.volume_ml }} мл
                  </div>
                </v-col>
                <v-col cols="6">
                  <div class="text-caption text-disabled">Время</div>
                  <div class="text-h6">
                    {{ form.duration_sec }} сек
                  </div>
                </v-col>
                <v-col cols="6">
                  <div class="text-caption text-disabled">Статус</div>
                  <v-chip color="success" size="small">
                    Успешно
                  </v-chip>
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>

          <v-alert type="success" variant="tonal">
            Калибровочные данные сохранены в NVS узла.
          </v-alert>
        </div>

        <!-- Error -->
        <v-alert v-if="error" type="error" variant="tonal" class="mt-4">
          {{ error }}
        </v-alert>
      </v-card-text>

      <v-card-actions>
        <v-spacer></v-spacer>
        
        <v-btn
          v-if="!isCalibrating && !calibrationResult"
          @click="close"
          :disabled="loading"
        >
          Отмена
        </v-btn>
        
        <v-btn
          v-if="calibrationResult"
          color="primary"
          @click="close"
        >
          Закрыть
        </v-btn>
        
        <v-btn
          v-if="!isCalibrating && !calibrationResult"
          color="primary"
          :disabled="!isFormValid || loading"
          :loading="loading"
          @click="startCalibration"
        >
          Начать калибровку
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup>
import { ref, computed, watch, onUnmounted } from 'vue'
import { useNodeConfigStore } from '@/stores/nodeConfig'
import { useSnackbar } from '@/composables/useSnackbar'

const props = defineProps({
  modelValue: {
    type: Boolean,
    default: false
  },
  nodeId: {
    type: String,
    required: true
  },
  pumpOptions: {
    type: Array,
    default: () => [
      { label: 'pH Up (Насос 0)', value: 0 },
      { label: 'pH Down (Насос 1)', value: 1 },
      { label: 'EC Up (Насос 2)', value: 2 },
      { label: 'EC Down (Насос 3)', value: 3 }
    ]
  }
})

const emit = defineEmits(['update:modelValue', 'calibrated'])

const configStore = useNodeConfigStore()
const { showSuccess, showError } = useSnackbar()

const dialog = computed({
  get: () => props.modelValue,
  set: (value) => emit('update:modelValue', value)
})

const form = ref({
  pump_id: null,
  volume_ml: 10,
  duration_sec: 5
})

const isCalibrating = ref(false)
const calibrationProgress = ref(0)
const timeRemaining = ref(0)
const calibrationResult = ref(null)
const error = ref(null)

let progressInterval = null

// Cleanup on unmount
onUnmounted(() => {
  if (progressInterval) {
    clearInterval(progressInterval)
  }
})

const rules = {
  volume: (v) => {
    if (!v) return 'Обязательное поле'
    if (v < 0.1 || v > 1000) return 'Объем должен быть от 0.1 до 1000 мл'
    return true
  },
  duration: (v) => {
    if (!v) return 'Обязательное поле'
    if (v < 1 || v > 60) return 'Длительность должна быть от 1 до 60 сек'
    return true
  }
}

const isFormValid = computed(() => {
  return form.value.pump_id !== null && 
         form.value.volume_ml >= 0.1 && form.value.volume_ml <= 1000 &&
         form.value.duration_sec >= 1 && form.value.duration_sec <= 60
})

const loading = computed(() => configStore.calibrating)

// Reset form when dialog opens
watch(dialog, (newVal) => {
  if (newVal) {
    resetForm()
  }
})

function resetForm() {
  form.value = {
    pump_id: null,
    volume_ml: 10,
    duration_sec: 5
  }
  isCalibrating.value = false
  calibrationProgress.value = 0
  timeRemaining.value = 0
  calibrationResult.value = null
  error.value = null
  
  // Очищаем интервал
  if (progressInterval) {
    clearInterval(progressInterval)
    progressInterval = null
  }
}

function close() {
  dialog.value = false
  resetForm()
}

function formatMlPerSecond(value) {
  if (!value && value !== 0) return '-'
  return `${Number(value).toFixed(2)} мл/сек`
}

async function startCalibration() {
  if (!isFormValid.value) return

  error.value = null
  isCalibrating.value = true
  calibrationProgress.value = 0
  timeRemaining.value = form.value.duration_sec

  // Очищаем предыдущий интервал если есть
  if (progressInterval) {
    clearInterval(progressInterval)
  }

  try {
    // Simulate calibration progress
    progressInterval = setInterval(() => {
      if (calibrationProgress.value < 90) {
        calibrationProgress.value += 10
      }
      if (timeRemaining.value > 0) {
        timeRemaining.value -= 1
      }
    }, form.value.duration_sec * 100)

    // Start actual calibration
    const result = await configStore.calibratePump(props.nodeId, {
      pump_id: form.value.pump_id,
      duration_sec: form.value.duration_sec,
      volume_ml: form.value.volume_ml
    })

    // Очищаем интервал
    if (progressInterval) {
      clearInterval(progressInterval)
      progressInterval = null
    }
    
    calibrationProgress.value = 100
    timeRemaining.value = 0

    calibrationResult.value = result
    const mlPerSec = result?.ml_per_second || result?.calibration?.ml_per_second || 0
    showSuccess(`Насос откалиброван: ${Number(mlPerSec).toFixed(2)} мл/сек`)
    
    emit('calibrated', result)
  } catch (err) {
    // Очищаем интервал при ошибке
    if (progressInterval) {
      clearInterval(progressInterval)
      progressInterval = null
    }
    
    error.value = err.message || 'Ошибка калибровки'
    isCalibrating.value = false
    showError('Не удалось откалибровать насос')
  }
}
</script>

<style scoped>
/* Фиксированная высота для кнопок диалога */
.v-card-actions .v-btn {
  min-width: 100px;
  min-height: 36px;
}

/* Прогресс-бар фиксированной высоты */
.v-progress-circular {
  margin: 16px 0;
}

/* Результаты калибровки */
.v-card-text {
  min-height: 200px;
}
</style>
