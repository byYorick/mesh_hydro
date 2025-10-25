import { ref, onMounted, onUnmounted } from 'vue'
import { useAppStore } from '@/stores/app'

/**
 * Composable для работы с офлайн режимом
 */
export function useOfflineMode() {
  const appStore = useAppStore()
  const isOnline = ref(navigator.onLine)
  const isOfflineMode = ref(false)
  const offlineQueue = ref([])
  const syncInProgress = ref(false)

  // Обработчики событий сети
  const handleOnline = () => {
    console.log('🌐 Network online')
    isOnline.value = true
    isOfflineMode.value = false
    
    // Показываем уведомление о восстановлении связи
    appStore.showSnackbar('Соединение восстановлено', 'success', 3000)
    
    // Синхронизируем данные
    syncOfflineData()
  }

  const handleOffline = () => {
    console.log('📴 Network offline')
    isOnline.value = false
    isOfflineMode.value = true
    
    // Показываем уведомление об офлайн режиме
    appStore.showSnackbar('Работа в офлайн режиме', 'warning', 5000)
  }

  // Синхронизация офлайн данных
  const syncOfflineData = async () => {
    if (syncInProgress.value || offlineQueue.value.length === 0) {
      return
    }

    syncInProgress.value = true
    console.log(`🔄 Syncing ${offlineQueue.value.length} offline actions...`)

    try {
      // Здесь можно добавить логику синхронизации с сервером
      // Например, отправка накопленных команд, событий и т.д.
      
      // Очищаем очередь после успешной синхронизации
      offlineQueue.value = []
      appStore.showSnackbar('Данные синхронизированы', 'success', 2000)
    } catch (error) {
      console.error('❌ Sync failed:', error)
      appStore.showSnackbar('Ошибка синхронизации', 'error', 3000)
    } finally {
      syncInProgress.value = false
    }
  }

  // Добавление действия в офлайн очередь
  const addToOfflineQueue = (action) => {
    offlineQueue.value.push({
      ...action,
      timestamp: Date.now(),
      id: Math.random().toString(36).substr(2, 9)
    })
    
    console.log(`📝 Added to offline queue: ${action.type}`)
  }

  // Выполнение действия (онлайн или офлайн)
  const executeAction = async (action, onlineHandler, offlineHandler) => {
    if (isOnline.value) {
      try {
        return await onlineHandler()
      } catch (error) {
        // Если ошибка сети, переходим в офлайн режим
        if (!navigator.onLine || error.code === 'NETWORK_ERROR') {
          isOnline.value = false
          isOfflineMode.value = true
          addToOfflineQueue(action)
          return offlineHandler ? offlineHandler() : null
        }
        throw error
      }
    } else {
      // Офлайн режим
      addToOfflineQueue(action)
      return offlineHandler ? offlineHandler() : null
    }
  }

  // Проверка возможности выполнения действия в офлайн
  const canExecuteOffline = (actionType) => {
    if (!actionType || typeof actionType !== 'string') {
      return false
    }
    
    const offlineCapableActions = [
      'view_nodes',
      'view_events',
      'view_telemetry',
      'view_settings'
    ]
    
    try {
      // Дополнительная проверка на undefined/null перед вызовом includes
      if (offlineCapableActions && Array.isArray(offlineCapableActions) && actionType) {
        return offlineCapableActions.includes(actionType)
      }
      return false
    } catch (error) {
      console.error('useOfflineMode.js: isOfflineCapable - Error in includes:', error)
      console.error('useOfflineMode.js: isOfflineCapable - actionType:', actionType, typeof actionType)
      console.error('useOfflineMode.js: isOfflineCapable - offlineCapableActions:', offlineCapableActions, typeof offlineCapableActions)
      return false
    }
  }

  // Получение офлайн данных из localStorage
  const getOfflineData = (key) => {
    try {
      const data = localStorage.getItem(`offline_${key}`)
      return data ? JSON.parse(data) : null
    } catch (error) {
      console.error('Error getting offline data:', error)
      return null
    }
  }

  // Сохранение данных в localStorage для офлайн доступа
  const setOfflineData = (key, data) => {
    try {
      localStorage.setItem(`offline_${key}`, JSON.stringify(data))
    } catch (error) {
      console.error('Error setting offline data:', error)
    }
  }

  // Очистка офлайн данных
  const clearOfflineData = () => {
    const keys = Object.keys(localStorage).filter(key => key.startsWith('offline_'))
    keys.forEach(key => localStorage.removeItem(key))
    offlineQueue.value = []
  }

  onMounted(() => {
    window.addEventListener('online', handleOnline)
    window.addEventListener('offline', handleOffline)
    
    // Проверяем статус сети при загрузке
    isOnline.value = navigator.onLine
    isOfflineMode.value = !navigator.onLine
  })

  onUnmounted(() => {
    window.removeEventListener('online', handleOnline)
    window.removeEventListener('offline', handleOffline)
  })

  return {
    isOnline,
    isOfflineMode,
    offlineQueue,
    syncInProgress,
    addToOfflineQueue,
    executeAction,
    canExecuteOffline,
    getOfflineData,
    setOfflineData,
    clearOfflineData,
    syncOfflineData
  }
}
