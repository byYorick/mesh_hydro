/**
 * Типы для системы статусов узлов
 */

export interface StatusConfig {
  heartbeat_interval: number
  node_offline_timeout: number
}

export interface StatusThresholds {
  excellent: number
  good: number
  poor: number
  offline: number
}

export interface NodeStatusData {
  isOnline: boolean
  color: string
  icon: string
  text: string
  quality: 'excellent' | 'good' | 'poor' | 'offline'
  secondsSinceLastSeen: number
  timeUntilOffline: number
  lastSeenText: string
  canPerformActions: boolean
  canRunPumps: boolean
  connectionQualityIcon: string
  connectionQualityColor: string
  uptimePercent: number
  totalUptime: string
  avgResponseTime: number
}

export interface Node {
  node_id: string
  node_type: string
  online: boolean
  last_seen_at: string | null
  zone?: string | null
  metadata?: Record<string, any> | null
  config?: Record<string, any> | null
  isPumpRunning?: boolean
  [key: string]: any
}
