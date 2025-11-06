import { Ref, computed, ref, watchEffect, type ComputedRef } from 'vue'
import { nodeStatusManager } from '@/services/NodeStatusManager'
import type { Node, NodeStatusData } from '@/types/nodeStatus'

/**
 * Новый composable для работы со статусом узла
 * Использует централизованный NodeStatusManager
 */
export function useNodeStatusV2(node: Ref<Node | null> | (() => Node | null)) {
  const manager = nodeStatusManager
  const status = ref<NodeStatusData | null>(null)

  // Получить значение узла (поддержка как Ref, так и функции)
  const getNodeValue = (): Node | null => {
    if (typeof node === 'function') {
      return node()
    }
    return node.value
  }

  // Автоматически подписаться на обновления
  let unsubscribe: (() => void) | null = null

  watchEffect(() => {
    const nodeValue = getNodeValue()

    if (!nodeValue?.node_id) {
      status.value = manager.calculateStatus(null)
      if (unsubscribe) {
        unsubscribe()
        unsubscribe = null
      }
      return
    }

    // Первичный расчет статуса
    status.value = manager.calculateStatus(nodeValue)

    // Отписаться от предыдущей подписки если есть
    if (unsubscribe) {
      unsubscribe()
    }

    // Подписаться на обновления
    unsubscribe = manager.subscribe(nodeValue.node_id, (newStatus) => {
      status.value = newStatus
    })
  })

  // Вычисляемые свойства для удобства использования
  return {
    // Основные статусы
    isOnline: computed(() => status.value?.isOnline ?? false),
    isPumpRunning: computed(() => {
      const nodeValue = getNodeValue()
      return nodeValue?.isPumpRunning ?? false
    }),

    // UI свойства
    statusColor: computed(() => status.value?.color ?? 'grey'),
    statusIcon: computed(() => status.value?.icon ?? 'mdi-help-circle'),
    statusText: computed(() => status.value?.text ?? 'Неизвестно'),
    lastSeenText: computed(() => status.value?.lastSeenText ?? 'Никогда'),

    // Качество связи
    connectionQuality: computed(() => status.value?.quality ?? 'offline'),
    connectionQualityIcon: computed(() => status.value?.connectionQualityIcon ?? 'mdi-help-circle'),
    connectionQualityColor: computed(() => status.value?.connectionQualityColor ?? 'grey'),

    // Детальная информация
    secondsSinceLastSeen: computed(() => status.value?.secondsSinceLastSeen ?? Infinity),
    timeUntilOffline: computed(() => status.value?.timeUntilOffline ?? 0),
    uptimePercent: computed(() => status.value?.uptimePercent ?? 0),

    // Проверки доступности
    canPerformActions: computed(() => status.value?.canPerformActions ?? false),
    canRunPumps: computed(() => status.value?.canRunPumps ?? false),

    // Статистика
    totalUptime: computed(() => status.value?.totalUptime ?? '00:00:00'),
    avgResponseTime: computed(() => status.value?.avgResponseTime ?? 0),

    // Полный объект статуса для расширенного использования
    status: computed(() => status.value),
  }
}
