<template>
  <v-card
    v-if="isVisible"
    class="status-bar"
    :color="statusColor"
    dark
    elevation="4"
  >
    <v-card-text class="pa-4">
      <div class="d-flex align-center justify-space-between">
        <div class="d-flex align-center">
          <v-icon :color="iconColor" class="mr-3" size="24">
            {{ statusIcon }}
          </v-icon>
          <div>
            <div class="text-h6 font-weight-bold">
              {{ statusMessage }}
            </div>
            <div class="text-caption opacity-75">
              {{ nodeId }} • {{ formattedTime }}
            </div>
          </div>
        </div>
        
        <div class="d-flex align-center">
          <v-btn
            icon
            size="small"
            @click="toggleExpanded"
            class="mr-2"
          >
            <v-icon>{{ expanded ? 'mdi-chevron-up' : 'mdi-chevron-down' }}</v-icon>
          </v-btn>
          
          <v-btn
            icon
            size="small"
            @click="close"
          >
            <v-icon>mdi-close</v-icon>
          </v-btn>
        </div>
      </div>

      <!-- Расширенные детали -->
      <v-expand-transition>
        <div v-if="expanded" class="mt-4">
          <v-divider class="mb-4"></v-divider>
          
          <!-- Параметры насоса -->
          <div v-if="pumpDetails" class="mb-4">
            <h4 class="text-subtitle-1 mb-2">
              <v-icon class="mr-2">mdi-pump</v-icon>
              Параметры насоса
            </h4>
            <v-row>
              <v-col cols="6" md="3">
                <div class="text-caption opacity-75">Насос</div>
                <div class="text-body-2 font-weight-medium">
                  {{ pumpDetails.pumpId || 'N/A' }}
                </div>
              </v-col>
              <v-col cols="6" md="3">
                <div class="text-caption opacity-75">Время работы</div>
                <div class="text-body-2 font-weight-medium">
                  {{ pumpDetails.duration || 'N/A' }} сек
                </div>
              </v-col>
              <v-col cols="6" md="3">
                <div class="text-caption opacity-75">Объем</div>
                <div class="text-body-2 font-weight-medium">
                  {{ pumpDetails.volume || 'N/A' }} мл
                </div>
              </v-col>
              <v-col cols="6" md="3">
                <div class="text-caption opacity-75">Скорость</div>
                <div class="text-body-2 font-weight-medium">
                  {{ pumpDetails.rate || 'N/A' }} мл/с
                </div>
              </v-col>
            </v-row>
          </div>

          <!-- Параметры PID -->
          <div v-if="pidParams" class="mb-4">
            <h4 class="text-subtitle-1 mb-2">
              <v-icon class="mr-2">mdi-tune</v-icon>
              Параметры PID
            </h4>
            <v-row>
              <v-col cols="4" md="2">
                <div class="text-caption opacity-75">Kp</div>
                <div class="text-body-2 font-weight-medium">
                  {{ pidParams.kp || 'N/A' }}
                </div>
              </v-col>
              <v-col cols="4" md="2">
                <div class="text-caption opacity-75">Ki</div>
                <div class="text-body-2 font-weight-medium">
                  {{ pidParams.ki || 'N/A' }}
                </div>
              </v-col>
              <v-col cols="4" md="2">
                <div class="text-caption opacity-75">Kd</div>
                <div class="text-body-2 font-weight-medium">
                  {{ pidParams.kd || 'N/A' }}
                </div>
              </v-col>
              <v-col cols="6" md="3">
                <div class="text-caption opacity-75">Текущее значение</div>
                <div class="text-body-2 font-weight-medium">
                  {{ pidParams.currentValue || 'N/A' }}
                </div>
              </v-col>
              <v-col cols="6" md="3">
                <div class="text-caption opacity-75">Целевое значение</div>
                <div class="text-body-2 font-weight-medium">
                  {{ pidParams.targetValue || 'N/A' }}
                </div>
              </v-col>
            </v-row>
          </div>

          <!-- Дополнительные параметры -->
          <div v-if="additionalParams" class="mb-4">
            <h4 class="text-subtitle-1 mb-2">
              <v-icon class="mr-2">mdi-information</v-icon>
              Дополнительные параметры
            </h4>
            <v-row>
              <v-col
                v-for="(value, key) in additionalParams"
                :key="key"
                cols="6"
                md="3"
              >
                <div class="text-caption opacity-75">{{ formatParamName(key) }}</div>
                <div class="text-body-2 font-weight-medium">
                  {{ value }}
                </div>
              </v-col>
            </v-row>
          </div>

          <!-- Прогресс бар (если есть) -->
          <div v-if="progress !== null" class="mb-4">
            <h4 class="text-subtitle-1 mb-2">
              <v-icon class="mr-2">mdi-progress-clock</v-icon>
              Прогресс выполнения
            </h4>
            <v-progress-linear
              :model-value="progress"
              :color="progressColor"
              height="8"
              rounded
            ></v-progress-linear>
            <div class="text-caption mt-1 text-center">
              {{ progress }}% завершено
            </div>
          </div>
        </div>
      </v-expand-transition>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'

const props = defineProps({
  visible: {
    type: Boolean,
    default: false
  },
  message: {
    type: String,
    default: ''
  },
  nodeId: {
    type: String,
    default: ''
  },
  level: {
    type: String,
    default: 'info'
  },
  timestamp: {
    type: [String, Number, Date],
    default: null
  },
  pumpDetails: {
    type: Object,
    default: null
  },
  pidParams: {
    type: Object,
    default: null
  },
  additionalParams: {
    type: Object,
    default: null
  },
  progress: {
    type: Number,
    default: null
  }
})

const emit = defineEmits(['close', 'toggle'])

const isVisible = ref(props.visible)
const expanded = ref(false)

// Цвета статуса
const statusColor = computed(() => {
  switch (props.level) {
    case 'critical':
    case 'emergency':
      return 'red-darken-2'
    case 'warning':
      return 'orange-darken-2'
    case 'info':
      return 'blue-darken-2'
    case 'debug':
      return 'grey-darken-2'
    default:
      return 'blue-darken-2'
  }
})

const iconColor = computed(() => {
  switch (props.level) {
    case 'critical':
    case 'emergency':
      return 'red-lighten-3'
    case 'warning':
      return 'orange-lighten-3'
    case 'info':
      return 'blue-lighten-3'
    case 'debug':
      return 'grey-lighten-3'
    default:
      return 'blue-lighten-3'
  }
})

const statusIcon = computed(() => {
  switch (props.level) {
    case 'critical':
    case 'emergency':
      return 'mdi-alert-circle'
    case 'warning':
      return 'mdi-alert'
    case 'info':
      return 'mdi-information'
    case 'debug':
      return 'mdi-bug'
    default:
      return 'mdi-information'
  }
})

const progressColor = computed(() => {
  if (props.progress === null) return 'primary'
  if (props.progress < 30) return 'red'
  if (props.progress < 70) return 'orange'
  return 'green'
})

const formattedTime = computed(() => {
  if (!props.timestamp) return 'N/A'
  
  const date = new Date(props.timestamp)
  return date.toLocaleString('ru-RU', {
    day: '2-digit',
    month: '2-digit',
    year: 'numeric',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
  })
})

const statusMessage = computed(() => {
  return props.message || 'Статус неизвестен'
})

const formatParamName = (key) => {
  const translations = {
    'temperature': 'Температура',
    'humidity': 'Влажность',
    'pressure': 'Давление',
    'ph': 'pH',
    'ec': 'EC',
    'tds': 'TDS',
    'voltage': 'Напряжение',
    'current': 'Ток',
    'power': 'Мощность',
    'uptime': 'Время работы',
    'heap_free': 'Свободная память',
    'wifi_rssi': 'Сила WiFi сигнала',
    'cpu_freq': 'Частота CPU'
  }
  
  return translations[key] || key
}

const toggleExpanded = () => {
  expanded.value = !expanded.value
  emit('toggle', expanded.value)
}

const close = () => {
  isVisible.value = false
  emit('close')
}

// Автоматическое закрытие через 10 секунд для некритичных сообщений
onMounted(() => {
  if (props.level !== 'critical' && props.level !== 'emergency') {
    setTimeout(() => {
      if (isVisible.value) {
        close()
      }
    }, 10000)
  }
})

// Следим за изменениями visible prop
watch(() => props.visible, (newVal) => {
  isVisible.value = newVal
  if (newVal) {
    expanded.value = false
  }
})
</script>

<style scoped>
.status-bar {
  position: fixed;
  top: 20px;
  right: 20px;
  z-index: 1000;
  min-width: 400px;
  max-width: 600px;
  border-radius: 12px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
}

@media (max-width: 768px) {
  .status-bar {
    top: 10px;
    right: 10px;
    left: 10px;
    min-width: auto;
    max-width: none;
  }
}
</style>
