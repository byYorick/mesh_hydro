<template>
  <v-card variant="outlined">
    <v-card-title class="text-subtitle-1">
      <v-icon icon="mdi-pump" class="mr-2"></v-icon>
      Ручное управление насосами
    </v-card-title>
    
    <v-card-text>
      <!-- Pump Selection -->
      <v-select
        v-model="selectedPump"
        :items="pumpOptions"
        item-title="label"
        item-value="value"
        label="Выберите насос"
        variant="outlined"
        class="mb-4"
      ></v-select>

      <!-- Duration Slider -->
      <div class="mb-4">
        <div class="d-flex justify-space-between align-center mb-2">
          <label class="text-body-2">Длительность работы</label>
          <v-chip color="primary" size="small">{{ duration }} сек</v-chip>
        </div>
        <v-slider
          v-model="duration"
          :min="0.1"
          :max="30"
          :step="0.1"
          thumb-label="always"
          color="primary"
        >
          <template v-slot:thumb-label="{ value }">
            {{ value }}с
          </template>
        </v-slider>
      </div>

      <!-- Quick Duration Buttons -->
      <div class="quick-buttons mb-4">
        <v-btn
          v-for="quick in quickDurations"
          :key="quick"
          size="small"
          variant="outlined"
          @click="duration = quick"
        >
          {{ quick }}с
        </v-btn>
      </div>

      <!-- Control Buttons -->
      <div class="d-grid gap-2">
        <v-btn
          color="success"
          size="large"
          :disabled="!selectedPump || isRunning || !isOnline"
          :loading="running"
          @click="startPump"
          block
        >
          <v-icon icon="mdi-play" start></v-icon>
          Запустить насос
        </v-btn>

        <v-btn
          v-if="isRunning"
          color="error"
          size="large"
          :loading="stopping"
          @click="stopPump"
          block
        >
          <v-icon icon="mdi-stop" start></v-icon>
          Остановить
        </v-btn>
      </div>

      <!-- Status -->
      <v-alert
        v-if="isRunning"
        type="info"
        variant="tonal"
        class="mt-4"
        density="compact"
      >
        <div class="d-flex justify-space-between align-center">
          <span>Насос работает...</span>
          <v-chip color="success" size="small">
            Осталось: {{ timeRemaining }} сек
          </v-chip>
        </div>
        <v-progress-linear
          :model-value="progress"
          color="success"
          height="8"
          rounded
          class="mt-2"
        ></v-progress-linear>
      </v-alert>

      <v-alert
        v-else-if="!isOnline"
        type="warning"
        variant="tonal"
        density="compact"
        class="mt-4"
      >
        <v-icon icon="mdi-wifi-off" start size="small"></v-icon>
        Узел офлайн. Управление недоступно.
      </v-alert>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { ref, computed, onUnmounted } from 'vue'
import { useNodeConfigStore } from '@/stores/nodeConfig'
import { useSnackbar } from '@/composables/useSnackbar'

const props = defineProps({
  nodeId: {
    type: String,
    required: true
  },
  isOnline: {
    type: Boolean,
    default: true
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

const emit = defineEmits(['pump-started', 'pump-stopped'])

const configStore = useNodeConfigStore()
const { showSuccess, showError } = useSnackbar()

const selectedPump = ref(null)
const duration = ref(5)
const running = ref(false)
const stopping = ref(false)
const isRunning = ref(false)
const timeRemaining = ref(0)
const progress = ref(0)

const quickDurations = [1, 3, 5, 10, 15, 30]

let timer = null

onUnmounted(() => {
  if (timer) {
    clearInterval(timer)
  }
})

async function startPump() {
  if (!selectedPump.value) return

  running.value = true
  isRunning.value = true
  timeRemaining.value = Math.ceil(duration.value)
  progress.value = 0

  try {
    await configStore.runPumpManually(props.nodeId, {
      pump_id: selectedPump.value,
      duration_sec: duration.value
    })

    showSuccess(`Насос ${selectedPump.value} запущен на ${duration.value} сек`)

    // Start countdown
    timer = setInterval(() => {
      if (timeRemaining.value > 0) {
        timeRemaining.value -= 0.1
        progress.value = ((duration.value - timeRemaining.value) / duration.value) * 100
      } else {
        stopPumpCountdown()
      }
    }, 100)

    emit('pump-started', {
      pump_id: selectedPump.value,
      duration: duration.value
    })
  } catch (err) {
    showError('Не удалось запустить насос')
    stopPumpCountdown()
  } finally {
    running.value = false
  }
}

function stopPump() {
  stopping.value = true
  stopPumpCountdown()
}

function stopPumpCountdown() {
  isRunning.value = false
  stopping.value = false
  
  if (timer) {
    clearInterval(timer)
    timer = null
  }

  timeRemaining.value = 0
  progress.value = 0

  emit('pump-stopped')
}
</script>

<style scoped>
.d-grid {
  display: grid;
}

.gap-2 {
  gap: 0.5rem;
}

/* Фиксированная высота для кнопок */
.v-btn {
  min-height: 48px;
  margin-bottom: 8px;
}

/* Фиксированная высота для слайдера */
.v-slider {
  margin-top: 8px;
}

/* Быстрые кнопки в одну строку */
.quick-buttons {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.quick-buttons .v-btn {
  flex: 1;
  min-width: 60px;
}
</style>
