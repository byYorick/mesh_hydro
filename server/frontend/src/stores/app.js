import { defineStore } from 'pinia'
import api from '@/services/api'

export const useAppStore = defineStore('app', {
  state: () => ({
    // Theme
    theme: localStorage.getItem('theme') || 'light',
    
    // Drawer (navigation)
    drawer: true,
    
    // System status
    systemStatus: null,
    
    // Dashboard summary
    dashboardSummary: null,
    
    // Loading states
    loading: false,
    
    // Snackbar
    snackbar: {
      show: false,
      message: '',
      color: 'info',
      timeout: 3000,
    },
    
    // Connection status
    connected: true,
    backendConnected: false,
    mqttConnected: false,
  }),

  getters: {
    isDarkTheme: (state) => state.theme === 'dark',
  },

  actions: {
    // Toggle theme
    toggleTheme() {
      this.theme = this.theme === 'light' ? 'dark' : 'light'
      localStorage.setItem('theme', this.theme)
    },

    // Toggle drawer
    toggleDrawer() {
      this.drawer = !this.drawer
    },

    // Show snackbar
    showSnackbar(message, color = 'info', timeout = 3000) {
      this.snackbar = {
        show: true,
        message,
        color,
        timeout,
      }
    },

    // Hide snackbar
    hideSnackbar() {
      this.snackbar.show = false
    },

    // Fetch system status
    async fetchSystemStatus() {
      try {
        this.systemStatus = await api.getStatus()
        
        // Update connection statuses
        this.backendConnected = this.systemStatus.database === 'ok'
        this.mqttConnected = this.systemStatus.mqtt === 'connected'
        
        return this.systemStatus
      } catch (error) {
        this.backendConnected = false
        console.error('Error fetching system status:', error)
        throw error
      }
    },

    // Fetch dashboard summary
    async fetchDashboardSummary() {
      this.loading = true
      
      try {
        this.dashboardSummary = await api.getDashboardSummary()
        return this.dashboardSummary
      } catch (error) {
        console.error('Error fetching dashboard summary:', error)
        this.showSnackbar('Ошибка загрузки данных', 'error')
        throw error
      } finally {
        this.loading = false
      }
    },

    // Health check
    async healthCheck() {
      try {
        await api.health()
        this.connected = true
        return true
      } catch (error) {
        this.connected = false
        return false
      }
    },

    // Централизованная обработка ошибок
    handleError(error, context = '') {
      const errorType = this.detectErrorType(error)
      const message = this.getErrorMessage(error, context)
      
      // Логирование с контекстом
      console.error(`[${context}] ${errorType}:`, error)
      
      // Показ snackbar
      this.showSnackbar(message, 'error', 5000)
      
      // Отправка critical errors на backend (опционально)
      if (errorType === 'critical') {
        this.reportError(error, context)
      }
      
      return errorType
    },

    // Определение типа ошибки
    detectErrorType(error) {
      if (!error) return 'unknown'
      
      if (error.response) {
        // HTTP ошибки
        if (error.response.status >= 500) return 'server'
        if (error.response.status === 422) return 'validation'
        if (error.response.status === 401 || error.response.status === 403) return 'auth'
        if (error.response.status === 404) return 'not_found'
        return 'client'
      }
      
      if (error.request) {
        // Network ошибки
        return 'network'
      }
      
      // Критичные ошибки
      if (error.message && typeof error.message === 'string' && error.message.includes('critical')) {
        return 'critical'
      }
      
      return 'unknown'
    },

    // Получить понятное сообщение об ошибке
    getErrorMessage(error, context) {
      const errorType = this.detectErrorType(error)
      
      // Кастомные сообщения для разных типов
      const messages = {
        network: 'Нет подключения к серверу. Проверьте интернет-соединение.',
        server: 'Ошибка сервера. Попробуйте позже.',
        validation: 'Проверьте правильность введенных данных.',
        auth: 'Требуется авторизация.',
        not_found: 'Ресурс не найден.',
        critical: `Критичная ошибка в ${context}. Обратитесь к администратору.`,
        unknown: 'Произошла ошибка. Попробуйте еще раз.',
      }
      
      // Попытка извлечь сообщение от сервера
      if (error.response?.data?.message) {
        return error.response.data.message
      }
      
      if (error.response?.data?.error) {
        return error.response.data.error
      }
      
      if (error.message) {
        return error.message
      }
      
      return messages[errorType] || messages.unknown
    },

    // Отправка critical errors на backend
    async reportError(error, context) {
      try {
        // Можно добавить endpoint для error reporting
        // await api.post('/errors/report', {
        //   error: error.message,
        //   context,
        //   stack: error.stack,
        //   timestamp: new Date().toISOString(),
        // })
        console.log('Critical error reported:', { error, context })
      } catch (e) {
        console.error('Failed to report error:', e)
      }
    },
  },
})

