import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export const useStatusBarStore = defineStore('statusBar', () => {
  // State
  const isVisible = ref(false)
  const message = ref('')
  const nodeId = ref('')
  const level = ref('info')
  const timestamp = ref(null)
  const pumpDetails = ref(null)
  const pidParams = ref(null)
  const additionalParams = ref(null)
  const progress = ref(null)
  const expanded = ref(false)

  // Getters
  const hasPumpDetails = computed(() => {
    return pumpDetails.value && (
      pumpDetails.value.pumpId ||
      pumpDetails.value.duration ||
      pumpDetails.value.volume ||
      pumpDetails.value.rate
    )
  })

  const hasPidParams = computed(() => {
    return pidParams.value && (
      pidParams.value.kp ||
      pidParams.value.ki ||
      pidParams.value.kd ||
      pidParams.value.currentValue ||
      pidParams.value.targetValue
    )
  })

  const hasAdditionalParams = computed(() => {
    return additionalParams.value && Object.keys(additionalParams.value).length > 0
  })

  // Actions
  const show = (options = {}) => {
    isVisible.value = true
    message.value = options.message || ''
    nodeId.value = options.nodeId || ''
    level.value = options.level || 'info'
    timestamp.value = options.timestamp || new Date()
    pumpDetails.value = options.pumpDetails || null
    pidParams.value = options.pidParams || null
    additionalParams.value = options.additionalParams || null
    progress.value = options.progress || null
    expanded.value = false
  }

  const hide = () => {
    isVisible.value = false
    message.value = ''
    nodeId.value = ''
    level.value = 'info'
    timestamp.value = null
    pumpDetails.value = null
    pidParams.value = null
    additionalParams.value = null
    progress.value = null
    expanded.value = false
  }

  const toggleExpanded = () => {
    expanded.value = !expanded.value
  }

  const updateProgress = (newProgress) => {
    if (isVisible.value) {
      progress.value = newProgress
    }
  }

  const updatePumpDetails = (details) => {
    if (isVisible.value) {
      pumpDetails.value = { ...pumpDetails.value, ...details }
    }
  }

  const updatePidParams = (params) => {
    if (isVisible.value) {
      pidParams.value = { ...pidParams.value, ...params }
    }
  }

  const updateAdditionalParams = (params) => {
    if (isVisible.value) {
      additionalParams.value = { ...additionalParams.value, ...params }
    }
  }

  // Показать статус для события
  const showForEvent = (event) => {
    const options = {
      message: event.message,
      nodeId: event.node_id,
      level: event.level,
      timestamp: event.created_at || event.timestamp,
      additionalParams: event.data || {}
    }

    // Извлекаем параметры насоса из данных события
    if (event.data) {
      if (event.data.pump_id || event.data.duration || event.data.volume) {
        options.pumpDetails = {
          pumpId: event.data.pump_id,
          duration: event.data.duration,
          volume: event.data.volume,
          rate: event.data.rate || event.data.ml_per_second
        }
      }

      // Извлекаем параметры PID из данных события
      if (event.data.kp || event.data.ki || event.data.kd || event.data.current_value || event.data.target_value) {
        options.pidParams = {
          kp: event.data.kp,
          ki: event.data.ki,
          kd: event.data.kd,
          currentValue: event.data.current_value || event.data.currentValue,
          targetValue: event.data.target_value || event.data.targetValue
        }
      }
    }

    show(options)
  }

  // Показать статус для насоса
  const showForPump = (nodeId, pumpId, duration, volume, rate = null) => {
    show({
      message: `Насос ${pumpId} запущен`,
      nodeId: nodeId,
      level: 'info',
      timestamp: new Date(),
      pumpDetails: {
        pumpId: pumpId,
        duration: duration,
        volume: volume,
        rate: rate
      }
    })
  }

  // Показать статус для PID
  const showForPid = (nodeId, currentValue, targetValue, kp = null, ki = null, kd = null) => {
    show({
      message: `PID коррекция: ${currentValue} → ${targetValue}`,
      nodeId: nodeId,
      level: 'info',
      timestamp: new Date(),
      pidParams: {
        kp: kp,
        ki: ki,
        kd: kd,
        currentValue: currentValue,
        targetValue: targetValue
      }
    })
  }

  return {
    // State
    isVisible,
    message,
    nodeId,
    level,
    timestamp,
    pumpDetails,
    pidParams,
    additionalParams,
    progress,
    expanded,

    // Getters
    hasPumpDetails,
    hasPidParams,
    hasAdditionalParams,

    // Actions
    show,
    hide,
    toggleExpanded,
    updateProgress,
    updatePumpDetails,
    updatePidParams,
    updateAdditionalParams,
    showForEvent,
    showForPump,
    showForPid
  }
})
