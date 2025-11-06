import { axios } from '@/services/api'
import type { Node, NodeStatusData, StatusConfig, StatusThresholds } from '@/types/nodeStatus'

/**
 * Централизованный менеджер статусов узлов
 * Singleton класс для управления статусами всех узлов
 */
class NodeStatusManager {
  private config: StatusConfig = {
    heartbeat_interval: 10,
    node_offline_timeout: 30,
  }

  private subscribers: Map<string, Set<(status: NodeStatusData) => void>> = new Map()
  private configLoaded: boolean = false

  /**
   * Загрузить конфигурацию с бэкенда
   */
  async loadConfig(): Promise<void> {
    try {
      // Используем axios instance напрямую
      // Response interceptor автоматически извлекает response.data
      // baseURL уже настроен в axios, поэтому используем относительный путь
      const data = await axios.get('/status/thresholds')
      if (data?.heartbeat_interval) {
        this.config.heartbeat_interval = data.heartbeat_interval
      }
      if (data?.node_offline_timeout) {
        this.config.node_offline_timeout = data.node_offline_timeout
      }
      this.configLoaded = true
      console.log('✅ NodeStatusManager: Config loaded', this.config)
    } catch (error) {
      console.warn('⚠️ NodeStatusManager: Failed to load config, using defaults', error)
      this.configLoaded = false
    }
  }

  /**
   * Получить текущую конфигурацию
   */
  getConfig(): StatusConfig {
    return { ...this.config }
  }

  /**
   * Получить пороги статусов
   */
  getThresholds(): StatusThresholds {
    const heartbeatInterval = this.config.heartbeat_interval
    const timeout = this.config.node_offline_timeout

    return {
      excellent: heartbeatInterval,
      good: timeout * 0.7,
      poor: timeout,
      offline: timeout,
    }
  }

  /**
   * Вычислить статус узла
   */
  calculateStatus(node: Node | null): NodeStatusData {
    if (!node) {
      return this.getDefaultStatus()
    }

    const lastSeenAt = node.last_seen_at ? new Date(node.last_seen_at) : null
    const now = new Date()
    const secondsSinceLastSeen = lastSeenAt
      ? Math.floor((now.getTime() - lastSeenAt.getTime()) / 1000)
      : Infinity

    const heartbeatInterval = this.config.heartbeat_interval
    const timeout = this.config.node_offline_timeout

    const isOnline = node.online ?? false
    const timeUntilOffline = isOnline ? Math.max(0, timeout - secondsSinceLastSeen) : 0

    // Определение качества связи
    let quality: 'excellent' | 'good' | 'poor' | 'offline'
    let color: string
    let icon: string
    let text: string

    if (!isOnline || secondsSinceLastSeen >= timeout) {
      quality = 'offline'
      color = 'error'
      icon = 'mdi-lan-disconnect'
      text = 'Офлайн'
    } else if (secondsSinceLastSeen < heartbeatInterval) {
      quality = 'excellent'
      color = 'success'
      icon = 'mdi-check-circle'
      text = 'Онлайн'
    } else if (secondsSinceLastSeen < timeout * 0.7) {
      quality = 'good'
      color = 'success'
      icon = 'mdi-check-circle'
      text = 'Онлайн'
    } else {
      quality = 'poor'
      color = 'warning'
      icon = 'mdi-alert-circle'
      text = 'Подключение слабое'
    }

    // Иконка качества связи
    const connectionQualityIcon =
      quality === 'excellent' || quality === 'good'
        ? 'mdi-wifi'
        : quality === 'poor'
        ? 'mdi-wifi-off'
        : 'mdi-lan-disconnect'

    const connectionQualityColor =
      quality === 'excellent' || quality === 'good'
        ? 'success'
        : quality === 'poor'
        ? 'warning'
        : 'error'

    // Текст времени последнего контакта
    const lastSeenText = this.formatLastSeen(secondsSinceLastSeen)

    // Проверки доступности действий
    const canPerformActions = isOnline && secondsSinceLastSeen < heartbeatInterval * 2
    const canRunPumps = canPerformActions && !(node.isPumpRunning ?? false)

    return {
      isOnline,
      color,
      icon,
      text,
      quality,
      secondsSinceLastSeen,
      timeUntilOffline,
      lastSeenText,
      canPerformActions,
      canRunPumps,
      connectionQualityIcon,
      connectionQualityColor,
      uptimePercent: isOnline ? 100 : 0, // TODO: расчет на основе исторических данных
      totalUptime: isOnline ? '24:00:00' : '00:00:00', // TODO: расчет на основе исторических данных
      avgResponseTime: 0, // TODO: расчет на основе исторических данных
    }
  }

  /**
   * Подписаться на изменения статуса узла
   */
  subscribe(nodeId: string, callback: (status: NodeStatusData) => void): () => void {
    if (!this.subscribers.has(nodeId)) {
      this.subscribers.set(nodeId, new Set())
    }
    this.subscribers.get(nodeId)!.add(callback)

    // Возвращаем функцию отписки
    return () => {
      const callbacks = this.subscribers.get(nodeId)
      if (callbacks) {
        callbacks.delete(callback)
        if (callbacks.size === 0) {
          this.subscribers.delete(nodeId)
        }
      }
    }
  }

  /**
   * Обновить статус узла и уведомить подписчиков
   */
  updateNodeStatus(nodeId: string, node: Node): void {
    const status = this.calculateStatus(node)
    const callbacks = this.subscribers.get(nodeId)
    if (callbacks) {
      callbacks.forEach((callback) => callback(status))
    }
  }

  /**
   * Форматировать время последнего контакта
   */
  private formatLastSeen(seconds: number): string {
    if (seconds === Infinity) return 'Никогда'
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
  }

  /**
   * Получить статус по умолчанию
   */
  private getDefaultStatus(): NodeStatusData {
    return {
      isOnline: false,
      color: 'grey',
      icon: 'mdi-help-circle',
      text: 'Неизвестно',
      quality: 'offline',
      secondsSinceLastSeen: Infinity,
      timeUntilOffline: 0,
      lastSeenText: 'Никогда',
      canPerformActions: false,
      canRunPumps: false,
      connectionQualityIcon: 'mdi-help-circle',
      connectionQualityColor: 'grey',
      uptimePercent: 0,
      totalUptime: '00:00:00',
      avgResponseTime: 0,
    }
  }
}

// Экспортируем singleton экземпляр
export const nodeStatusManager = new NodeStatusManager()
