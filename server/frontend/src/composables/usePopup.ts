import { ref, reactive, computed } from 'vue'

// ==================== INTERFACES ====================

export interface DialogItem {
  id: string
  type: 'confirm' | 'prompt' | 'custom'
  title?: string
  message?: string
  component?: any
  props?: Record<string, any>
  confirmText?: string
  cancelText?: string
  persistent?: boolean
  maxWidth?: number
  color?: string
  icon?: string
  zIndex: number
  loading: boolean
  error: string | null
  onConfirm?: () => void | Promise<void>
  onCancel?: () => void
}

export interface ToastItem {
  id: string
  level: 'success' | 'error' | 'warning' | 'info' | 'critical' | 'emergency'
  message: string
  nodeId?: string
  data?: Record<string, any>
  timeout: number
  show: boolean
  timestamp: number
  priority: number
  grouped?: boolean
  count?: number
}

interface PopupState {
  dialogs: DialogItem[]
  toasts: ToastItem[]
}

// ==================== STATE ====================

const popupState = reactive<PopupState>({
  dialogs: [],
  toasts: []
})

const baseZIndex = {
  dialog: 2000,
  toast: 10000
}

// ==================== UTILITIES ====================

const generateId = (prefix: string = 'popup'): string => {
  return `${prefix}-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`
}

const getTimeout = (level: ToastItem['level']): number => {
  const timeouts: Record<string, number> = {
    success: 3000,
    info: 5000,
    warning: 10000,
    error: 8000,
    critical: 10000,
    emergency: 15000
  }
  return timeouts[level] || 5000
}

const getPriority = (level: ToastItem['level']): number => {
  const priorities: Record<string, number> = {
    success: 1,
    info: 2,
    warning: 3,
    error: 4,
    critical: 5,
    emergency: 5
  }
  return priorities[level] || 2
}

const recalculateDialogIndexes = () => {
  popupState.dialogs.forEach((dialog, index) => {
    dialog.zIndex = baseZIndex.dialog + index * 10
  })
}

// ==================== MAIN COMPOSABLE ====================

export function usePopup() {
  // ==================== DIALOG METHODS ====================
  
  const dialog = {
    /**
     * Открыть диалог
     */
    open: (config: Omit<DialogItem, 'id' | 'zIndex' | 'loading' | 'error'>) => {
      const dialogId = generateId('dialog')
      const zIndex = baseZIndex.dialog + popupState.dialogs.length * 10
      
      const dialogItem: DialogItem = {
        ...config,
        id: dialogId,
        zIndex,
        loading: false,
        error: null
      }
      
      popupState.dialogs.push(dialogItem)
    },

    /**
     * Подтверждение
     */
    confirm: (
      message: string,
      title: string = 'Подтверждение',
      onConfirm?: () => void | Promise<void>
    ) => {
      dialog.open({
        type: 'confirm',
        title,
        message,
        confirmText: 'Да',
        cancelText: 'Отмена',
        onConfirm
      })
    },

    /**
     * Ввод данных
     */
    prompt: (
      message: string,
      title: string = 'Ввод',
      onConfirm?: (value: string) => void | Promise<void>
    ) => {
      const valueRef = ref('')
      
      dialog.open({
        type: 'prompt',
        title,
        message,
        confirmText: 'OK',
        cancelText: 'Отмена',
        onConfirm: () => onConfirm?.(valueRef.value),
        props: {
          modelValue: valueRef
        }
      })
    },

    /**
     * Закрыть текущий диалог
     */
    close: () => {
      const currentDialog = dialog.getCurrent()
      
      if (currentDialog) {
        if (currentDialog.onCancel) {
          currentDialog.onCancel()
        }
        
        const index = popupState.dialogs.findIndex(d => d.id === currentDialog.id)
        if (index !== -1) {
          popupState.dialogs.splice(index, 1)
          recalculateDialogIndexes()
        }
      }
    },

    /**
     * Закрыть все диалоги
     */
    closeAll: () => {
      popupState.dialogs.forEach(dialogItem => {
        if (dialogItem.onCancel) {
          dialogItem.onCancel()
        }
      })
      popupState.dialogs = []
    },

    /**
     * Подтвердить текущий диалог
     */
    confirmCurrent: async () => {
      const currentDialog = dialog.getCurrent()
      
      if (!currentDialog) return
      if (!currentDialog.onConfirm) {
        dialog.close()
        return
      }

      const index = popupState.dialogs.findIndex(d => d.id === currentDialog.id)
      if (index !== -1) {
        popupState.dialogs[index].loading = true
        popupState.dialogs[index].error = null
      }

      try {
        await currentDialog.onConfirm()
        dialog.close()
      } catch (error) {
        console.error('Dialog confirm error:', error)
        if (index !== -1) {
          popupState.dialogs[index].error = error instanceof Error ? error.message : 'Произошла ошибка'
        }
      } finally {
        if (index !== -1) {
          popupState.dialogs[index].loading = false
        }
      }
    },

    /**
     * Получить текущий диалог
     */
    getCurrent: (): DialogItem | null => {
      return popupState.dialogs.length > 0 
        ? popupState.dialogs[popupState.dialogs.length - 1] 
        : null
    },

    /**
     * Проверка открыт ли диалог
     */
    isOpen: () => popupState.dialogs.length > 0
  }

  // ==================== TOAST METHODS ====================
  
  const toast = {
    /**
     * Добавить уведомление
     */
    add: (config: string | Omit<ToastItem, 'id' | 'timeout' | 'show' | 'timestamp' | 'priority'>) => {
      // Поддержка разных форматов ввода
      let level: ToastItem['level'] = 'info'
      let message = 'Новое событие'
      let nodeId: string | undefined
      let data: Record<string, any> | undefined
      let configId: string | undefined

      if (typeof config === 'string') {
        message = config
      } else if (config) {
        level = config.level || 'info'
        message = config.message || message
        nodeId = config.nodeId
        data = config.data
        configId = (config as any).id
      }

      const notificationId = configId || generateId('toast')
      const level_str = typeof config === 'string' ? 'info' : (config.level || 'info')
      const priority = getPriority(level_str)
      const timeout = getTimeout(level_str)

      // Проверка дубликатов
      const existingIndex = configId
        ? popupState.toasts.findIndex(t => t.id === configId)
        : popupState.toasts.findIndex(t => 
            t.message === message && t.timestamp > Date.now() - 1000
          )

      if (existingIndex >= 0) {
        // Обновить существующее
        popupState.toasts[existingIndex] = {
          ...popupState.toasts[existingIndex],
          level: level_str,
          message,
          nodeId,
          data,
          show: true,
          timestamp: Date.now(),
          priority
        }
        return
      }

      const toastItem: ToastItem = {
        id: notificationId,
        level: level_str,
        message,
        nodeId,
        data,
        timeout,
        show: true,
        timestamp: Date.now(),
        priority
      }

      popupState.toasts.push(toastItem)

      // Автоудаление
      if (timeout > 0) {
        setTimeout(() => {
          const index = popupState.toasts.findIndex(t => t.id === toastItem.id)
          if (index !== -1) {
            popupState.toasts[index].show = false
            setTimeout(() => {
              toast.remove(toastItem.id)
            }, 300)
          }
        }, timeout)
      }
    },

    /**
     * Успешное уведомление
     */
    success: (message: string) => {
      toast.add({ level: 'success', message })
    },

    /**
     * Ошибка
     */
    error: (message: string) => {
      toast.add({ level: 'error', message })
    },

    /**
     * Предупреждение
     */
    warning: (message: string) => {
      toast.add({ level: 'warning', message })
    },

    /**
     * Информация
     */
    info: (message: string) => {
      toast.add({ level: 'info', message })
    },

    /**
     * Критичное
     */
    critical: (message: string, nodeId?: string, data?: Record<string, any>) => {
      toast.add({ level: 'critical', message, nodeId, data })
    },

    /**
     * Аварийное
     */
    emergency: (message: string, nodeId?: string, data?: Record<string, any>) => {
      toast.add({ level: 'emergency', message, nodeId, data })
    },

    /**
     * Удалить уведомление
     */
    remove: (id: string) => {
      const index = popupState.toasts.findIndex(t => t.id === id)
      if (index !== -1) {
        popupState.toasts.splice(index, 1)
      }
    },

    /**
     * Очистить все
     */
    clear: () => {
      popupState.toasts = []
    }
  }

  // ==================== RETURN API ====================

  return {
    // State
    state: computed(() => popupState),
    dialogs: computed(() => popupState.dialogs),
    toasts: computed(() => popupState.toasts),
    
    // API
    dialog,
    toast
  }
}

// Глобальный экземпляр для удобства
export const popupManager = usePopup()

