import { useAppStore } from '@/stores/app'

export function useSnackbar() {
  const appStore = useAppStore()
  
  function showSuccess(message) {
    appStore.showSnackbar(message, 'success')
  }
  
  function showError(message) {
    appStore.showSnackbar(message, 'error')
  }
  
  function showInfo(message) {
    appStore.showSnackbar(message, 'info')
  }
  
  function showWarning(message) {
    appStore.showSnackbar(message, 'warning')
  }
  
  return {
    showSuccess,
    showError,
    showInfo,
    showWarning
  }
}
