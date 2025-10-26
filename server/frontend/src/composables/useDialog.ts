import { ref, reactive } from 'vue'

export interface DialogConfig {
  title?: string
  message?: string
  confirmText?: string
  cancelText?: string
  persistent?: boolean
  maxWidth?: string | number
  type?: 'confirm' | 'alert' | 'prompt' | 'custom'
  color?: string
  icon?: string
  onConfirm?: () => void | Promise<void>
  onCancel?: () => void
  component?: any
  props?: Record<string, any>
}

interface DialogState {
  isOpen: boolean
  config: DialogConfig | null
  loading: boolean
  error: string | null
}

const dialogState = reactive<DialogState>({
  isOpen: false,
  config: null,
  loading: false,
  error: null,
})

export function useDialog() {
  const openDialog = (config: DialogConfig) => {
    dialogState.config = config
    dialogState.isOpen = true
    dialogState.loading = false
    dialogState.error = null
  }

  const closeDialog = () => {
    if (dialogState.config?.onCancel) {
      dialogState.config.onCancel()
    }
    dialogState.isOpen = false
    dialogState.config = null
    dialogState.loading = false
    dialogState.error = null
  }

  const confirmDialog = async () => {
    if (!dialogState.config?.onConfirm) {
      closeDialog()
      return
    }

    dialogState.loading = true
    dialogState.error = null

    try {
      await dialogState.config.onConfirm()
      closeDialog()
    } catch (error) {
      console.error('Dialog confirm error:', error)
      dialogState.error = error instanceof Error ? error.message : 'Произошла ошибка'
    } finally {
      dialogState.loading = false
    }
  }

  // Базовые методы для быстрого вызова
  const confirm = (
    message: string,
    title: string = 'Подтверждение',
    onConfirm?: () => void | Promise<void>
  ) => {
    openDialog({
      type: 'confirm',
      title,
      message,
      confirmText: 'Да',
      cancelText: 'Отмена',
      onConfirm,
    })
  }

  const alert = (
    message: string,
    title: string = 'Уведомление',
    onClose?: () => void
  ) => {
    openDialog({
      type: 'alert',
      title,
      message,
      confirmText: 'OK',
      onConfirm: onClose,
    })
  }

  const prompt = (
    message: string,
    title: string = 'Ввод',
    onConfirm?: (value: string) => void | Promise<void>
  ) => {
    const valueRef = ref('')
    
    openDialog({
      type: 'prompt',
      title,
      message,
      confirmText: 'OK',
      cancelText: 'Отмена',
      onConfirm: () => onConfirm?.(valueRef.value),
      props: {
        modelValue: valueRef,
      },
    })
  }

  const error = (
    message: string,
    title: string = 'Ошибка'
  ) => {
    openDialog({
      type: 'alert',
      title,
      message,
      confirmText: 'OK',
      color: 'error',
      icon: 'mdi-alert-circle',
    })
  }

  const success = (
    message: string,
    title: string = 'Успешно'
  ) => {
    openDialog({
      type: 'alert',
      title,
      message,
      confirmText: 'OK',
      color: 'success',
      icon: 'mdi-check-circle',
    })
  }

  const warning = (
    message: string,
    title: string = 'Внимание'
  ) => {
    openDialog({
      type: 'alert',
      title,
      message,
      confirmText: 'OK',
      color: 'warning',
      icon: 'mdi-alert',
    })
  }

  return {
    // State
    isOpen: () => dialogState.isOpen,
    config: () => dialogState.config,
    loading: () => dialogState.loading,
    error: () => dialogState.error,
    
    // Methods
    openDialog,
    closeDialog,
    confirmDialog,
    
    // Shortcuts
    confirm,
    alert,
    prompt,
    error,
    success,
    warning,
  }
}

export const dialogManager = useDialog()
