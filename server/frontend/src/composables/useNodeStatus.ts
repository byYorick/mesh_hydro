import { Ref, ComputedRef } from 'vue'
import { computed, ref } from 'vue'

/**
 * Интерфейс для статуса узла
 */
export interface NodeStatus {
  // Основные статусы
  isOnline: ComputedRef<boolean>
  isPumpRunning: ComputedRef<boolean>
  
  // UI свойства
  statusColor: ComputedRef<string>
  statusIcon: ComputedRef<string>
  statusText: ComputedRef<string>
  lastSeenText: ComputedRef<string>
  
  // Качество связи
  connectionQuality: ComputedRef<'excellent' | 'good' | 'poor' | 'offline'>
  connectionQualityIcon: ComputedRef<string>
  connectionQualityColor: ComputedRef<string>
  
  // Детальная информация
  secondsSinceLastSeen: ComputedRef<number>
  timeUntilOffline: ComputedRef<number>
  uptimePercent: ComputedRef<number>
  
  // Проверки доступности
  canPerformActions: ComputedRef<boolean>
  canRunPumps: ComputedRef<boolean>
  
  // Статистика
  totalUptime: ComputedRef<string>
  avgResponseTime: ComputedRef<number>
}

/**
 * Централизованная система статусов узлов
 * Единый источник истины для всех компонентов
 */
export function useNodeStatus(node: Ref<any>): NodeStatus {
  // Кэш для предотвращения частых пересчетов
  const lastComputed = ref<number>(0)
  const CACHE_DURATION = 1000 // 1 секунда кэш

  // Основной статус подключения
  const isOnline = computed<boolean>(() => {
    if (!node.value) return false
    return Boolean(node.value.online)
  })

  // Статус насосов (отдельный от онлайн-статуса)
  const isPumpRunning = computed<boolean>(() => {
    if (!node.value) return false
    return Boolean(node.value.isPumpRunning)
  })

  // Секунды с последнего контакта
  const secondsSinceLastSeen = computed<number>(() => {
    if (!node.value?.last_seen_at) return Infinity
    
    const lastSeen = new Date(node.value.last_seen_at)
    const now = new Date()
    return Math.floor((now.getTime() - lastSeen.getTime()) / 1000)
  })

  // Время до автоматического перехода в офлайн (20 сек таймаут)
  const timeUntilOffline = computed<number>(() => {
    if (!isOnline.value) return 0
    const timeout = 20
    const remaining = timeout - secondsSinceLastSeen.value
    return Math.max(0, remaining)
  })

  // Улучшенный цвет статуса с градацией
  const statusColor = computed<string>(() => {
    if (!node.value) return 'grey'
    if (!isOnline.value) return 'error'
    
    const seconds = secondsSinceLastSeen.value
    if (seconds < 5) return 'success'
    if (seconds < 15) return 'warning'
    return 'error'
  })

  // Улучшенная иконка статуса
  const statusIcon = computed<string>(() => {
    if (!node.value) return 'mdi-help-circle'
    
    if (!isOnline.value) return 'mdi-lan-disconnect'
    
    const seconds = secondsSinceLastSeen.value
    if (seconds < 5) return 'mdi-check-circle'
    if (seconds < 15) return 'mdi-alert-circle'
    return 'mdi-wifi-off'
  })

  // Текст статуса с деталями
  const statusText = computed<string>(() => {
    if (!node.value) return 'Неизвестно'
    
    if (!isOnline.value) return 'Офлайн'
    
    const seconds = secondsSinceLastSeen.value
    if (seconds < 5) return 'Онлайн'
    if (seconds < 15) return 'Подключение слабое'
    return 'Потеря связи'
  })

  // Время последнего контакта
  const lastSeenText = computed<string>(() => {
    if (!node.value?.last_seen_at) return 'Никогда'
    
    const seconds = secondsSinceLastSeen.value
    
    if (seconds < 1) return 'Только что'
    if (seconds < 60) return `${seconds} сек назад`
    if (seconds < 3600) {
      const minutes = Math.floor(seconds / 60)
      return `${minutes} мин назад`
    }
    if (seconds < 86400) {
      const hours = Math.floor(seconds / 3600)
      return `${hours} ч назад`
    }
    const days = Math.floor(seconds / 86400)
    return `${days} дн назад`
  })

  // Качество подключения
  const connectionQuality = computed<'excellent' | 'good' | 'poor' | 'offline'>(() => {
    if (!isOnline.value) return 'offline'
    
    const seconds = secondsSinceLastSeen.value
    if (seconds < 3) return 'excellent'
    if (seconds < 10) return 'good'
    return 'poor'
  })

  // Иконка качества связи
  const connectionQualityIcon = computed<string>(() => {
    const quality = connectionQuality.value
    switch (quality) {
      case 'excellent': return 'mdi-wifi'
      case 'good': return 'mdi-wifi'
      case 'poor': return 'mdi-wifi-off'
      case 'offline': return 'mdi-lan-disconnect'
    }
  })

  // Цвет качества связи
  const connectionQualityColor = computed<string>(() => {
    const quality = connectionQuality.value
    switch (quality) {
      case 'excellent': return 'success'
      case 'good': return 'info'
      case 'poor': return 'warning'
      case 'offline': return 'error'
    }
  })

  // Процент аптайма (за последние 24 часа)
  const uptimePercent = computed<number>(() => {
    // TODO: Расчет на основе исторических данных
    // Пока возвращаем 100% если онлайн
    return isOnline.value ? 100 : 0
  })

  // Время работы за последние 24 часа
  const totalUptime = computed<string>(() => {
    // TODO: Расчет на основе исторических данных
    return isOnline.value ? '24:00:00' : '00:00:00'
  })

  // Среднее время отклика (мс)
  const avgResponseTime = computed<number>(() => {
    // TODO: Расчет на основе исторических данных
    return 0
  })

  // Проверка доступности действий
  const canPerformActions = computed<boolean>(() => {
    return isOnline.value && secondsSinceLastSeen.value < 15
  })

  // Проверка доступности насосов
  const canRunPumps = computed<boolean>(() => {
    return canPerformActions.value && !isPumpRunning.value
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
    
    // Качество связи
    connectionQuality,
    connectionQualityIcon,
    connectionQualityColor,
    
    // Детальная информация
    secondsSinceLastSeen,
    timeUntilOffline,
    uptimePercent,
    
    // Проверки доступности
    canPerformActions,
    canRunPumps,
    
    // Статистика
    totalUptime,
    avgResponseTime
  }
}

/**
 * Интерфейс для статуса коллекции узлов
 */
export interface NodesStatus {
  stats: ComputedRef<{
    total: number
    online: number
    offline: number
    byType: Record<string, number>
    onlineByType: Record<string, number>
    connectionQualityStats: {
      excellent: number
      good: number
      poor: number
      offline: number
    }
  }>
  onlineNodes: ComputedRef<any[]>
  offlineNodes: ComputedRef<any[]>
  nodesByType: (type: string) => any[]
  onlineNodesByType: (type: string) => any[]
}

/**
 * Централизованная система для работы с коллекцией узлов
 */
export function useNodesStatus(nodes: Ref<any[]>): NodesStatus {
  // Статистика по статусам
  const stats = computed<any>(() => {
    if (!nodes.value || !Array.isArray(nodes.value)) {
      return { 
        total: 0, 
        online: 0, 
        offline: 0, 
        byType: {},
        onlineByType: {},
        connectionQualityStats: {
          excellent: 0,
          good: 0,
          poor: 0,
          offline: 0
        }
      }
    }
    
    const total = nodes.value.length
    let online = 0
    let offline = 0
    
    const byType: Record<string, number> = {}
    const onlineByType: Record<string, number> = {}
    const qualityStats = {
      excellent: 0,
      good: 0,
      poor: 0,
      offline: 0
    }

    nodes.value.forEach(node => {
      // Подсчет по типу
      byType[node.node_type] = (byType[node.node_type] || 0) + 1
      
      // Подсчет онлайн/оффлайн
      if (node.online) {
        online++
        onlineByType[node.node_type] = (onlineByType[node.node_type] || 0) + 1
      } else {
        offline++
        qualityStats.offline++
      }
    })
    
    return { 
      total, 
      online, 
      offline, 
      byType,
      onlineByType,
      connectionQualityStats: qualityStats
    }
  })

  // Онлайн узлы
  const onlineNodes = computed<any[]>(() => {
    if (!nodes.value) return []
    return nodes.value.filter(node => node.online)
  })

  // Офлайн узлы
  const offlineNodes = computed<any[]>(() => {
    if (!nodes.value) return []
    return nodes.value.filter(node => !node.online)
  })

  // Узлы по типу
  const nodesByType = computed<any>(() => (type: string) => {
    if (!nodes.value) return []
    return nodes.value.filter(node => node.node_type === type)
  })

  // Онлайн узлы по типу
  const onlineNodesByType = computed<any>(() => (type: string) => {
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
