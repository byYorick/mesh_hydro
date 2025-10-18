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
  },
})

