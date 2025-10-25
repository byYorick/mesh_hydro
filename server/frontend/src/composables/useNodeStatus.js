import { computed } from 'vue'

/**
 * Централизованная система статусов узлов
 * Единый источник истины для всех компонентов
 */
export function useNodeStatus(node) {
  // Основной статус подключения
  const isOnline = computed(() => {
    if (!node.value) return false
    return Boolean(node.value.online)
  })

  // Статус насосов (отдельный от онлайн-статуса)
  const isPumpRunning = computed(() => {
    if (!node.value) return false
    return Boolean(node.value.isPumpRunning)
  })

  // Цвет статуса
  const statusColor = computed(() => {
    if (!node.value) return 'grey'
    return isOnline.value ? 'success' : 'error'
  })

  // Иконка статуса
  const statusIcon = computed(() => {
    if (!node.value) return 'mdi-help-circle'
    return isOnline.value ? 'mdi-check-circle' : 'mdi-lan-disconnect'
  })

  // Текст статуса
  const statusText = computed(() => {
    if (!node.value) return 'Неизвестно'
    return isOnline.value ? 'Онлайн' : 'Офлайн'
  })

  // Время последнего контакта
  const lastSeenText = computed(() => {
    if (!node.value?.last_seen_at) return 'Никогда'
    
    const lastSeen = new Date(node.value.last_seen_at)
    const now = new Date()
    const diffSeconds = Math.floor((now - lastSeen) / 1000)
    
    if (diffSeconds < 60) return `${diffSeconds} сек назад`
    if (diffSeconds < 3600) return `${Math.floor(diffSeconds / 60)} мин назад`
    if (diffSeconds < 86400) return `${Math.floor(diffSeconds / 3600)} ч назад`
    return `${Math.floor(diffSeconds / 86400)} дн назад`
  })

  // Проверка доступности действий
  const canPerformActions = computed(() => {
    return isOnline.value
  })

  // Проверка доступности насосов
  const canRunPumps = computed(() => {
    return isOnline.value && !isPumpRunning.value
  })

  return {
    // Основные статусы
    isOnline,
    isPumpRunning,
    
    // UI свойства
    statusColor,
    statusIcon,
    statusText,
    lastSeenText,
    
    // Проверки доступности
    canPerformActions,
    canRunPumps
  }
}

/**
 * Централизованная система для работы с коллекцией узлов
 */
export function useNodesStatus(nodes) {
  // Статистика по статусам
  const stats = computed(() => {
    if (!nodes.value) return { total: 0, online: 0, offline: 0, byType: {} }
    
    const total = nodes.value.length
    const online = nodes.value.filter(node => node.online).length
    const offline = total - online
    
    const byType = {}
    nodes.value.forEach(node => {
      byType[node.node_type] = (byType[node.node_type] || 0) + 1
    })
    
    return { total, online, offline, byType }
  })

  // Онлайн узлы
  const onlineNodes = computed(() => {
    if (!nodes.value) return []
    return nodes.value.filter(node => node.online)
  })

  // Офлайн узлы
  const offlineNodes = computed(() => {
    if (!nodes.value) return []
    return nodes.value.filter(node => !node.online)
  })

  // Узлы по типу
  const nodesByType = computed(() => (type) => {
    if (!nodes.value) return []
    return nodes.value.filter(node => node.node_type === type)
  })

  // Онлайн узлы по типу
  const onlineNodesByType = computed(() => (type) => {
    if (!nodes.value) return []
    return nodes.value.filter(node => node.node_type === type && node.online)
  })

  return {
    stats,
    onlineNodes,
    offlineNodes,
    nodesByType,
    onlineNodesByType
  }
}
