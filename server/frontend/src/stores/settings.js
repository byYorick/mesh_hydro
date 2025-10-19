import { defineStore } from 'pinia'

export const useSettingsStore = defineStore('settings', {
  state: () => ({
    // UI Settings
    ui: {
      theme: localStorage.getItem('theme') || 'light',
      language: localStorage.getItem('language') || 'ru',
      autoRefresh: localStorage.getItem('autoRefresh') !== 'false',
      refreshInterval: parseInt(localStorage.getItem('refreshInterval') || '30'),
      compactView: localStorage.getItem('compactView') === 'true',
      showGridLines: localStorage.getItem('showGridLines') !== 'false',
      animationsEnabled: localStorage.getItem('animationsEnabled') !== 'false',
    },

    // System Settings
    system: {
      nodeOfflineTimeout: 30,
      telemetryRetentionDays: 365,
      eventAutoResolveHours: 24,
      emergencyNoAutoResolve: true,
    },

    // Notifications Settings
    notifications: {
      enabled: true,
      sound: localStorage.getItem('notificationSound') !== 'false',
      desktop: localStorage.getItem('desktopNotifications') === 'true',
      levels: {
        info: false,
        warning: true,
        critical: true,
        emergency: true,
      },
    },

    // Chart Settings
    charts: {
      defaultPeriod: localStorage.getItem('chartPeriod') || '24h',
      pointSize: parseInt(localStorage.getItem('chartPointSize') || '2'),
      lineWidth: parseInt(localStorage.getItem('chartLineWidth') || '2'),
      fillArea: localStorage.getItem('chartFillArea') !== 'false',
      showLegend: localStorage.getItem('chartShowLegend') !== 'false',
    },

    // MQTT Settings (read-only from backend)
    mqtt: {
      host: 'localhost',
      port: 1883,
      connected: false,
    },

    // Telegram Settings (read-only from backend)
    telegram: {
      enabled: false,
      connected: false,
    },
  }),

  actions: {
    // UI Settings
    setTheme(theme) {
      this.ui.theme = theme
      localStorage.setItem('theme', theme)
    },

    setLanguage(lang) {
      this.ui.language = lang
      localStorage.setItem('language', lang)
    },

    setAutoRefresh(enabled) {
      this.ui.autoRefresh = enabled
      localStorage.setItem('autoRefresh', enabled)
    },

    setRefreshInterval(interval) {
      this.ui.refreshInterval = interval
      localStorage.setItem('refreshInterval', interval)
    },

    setCompactView(enabled) {
      this.ui.compactView = enabled
      localStorage.setItem('compactView', enabled)
    },

    toggleAnimations() {
      this.ui.animationsEnabled = !this.ui.animationsEnabled
      localStorage.setItem('animationsEnabled', this.ui.animationsEnabled)
    },

    // Notifications
    setNotificationSound(enabled) {
      this.notifications.sound = enabled
      localStorage.setItem('notificationSound', enabled)
    },

    setDesktopNotifications(enabled) {
      this.notifications.desktop = enabled
      localStorage.setItem('desktopNotifications', enabled)
      
      if (enabled && 'Notification' in window) {
        Notification.requestPermission()
      }
    },

    setNotificationLevel(level, enabled) {
      this.notifications.levels[level] = enabled
      localStorage.setItem(`notification_${level}`, enabled)
    },

    // Charts
    setChartDefaults(settings) {
      Object.assign(this.charts, settings)
      
      Object.keys(settings).forEach(key => {
        localStorage.setItem(`chart${key.charAt(0).toUpperCase() + key.slice(1)}`, settings[key])
      })
    },

    // Reset to defaults
    resetToDefaults() {
      localStorage.clear()
      window.location.reload()
    },

    // Export settings
    exportSettings() {
      const settings = {
        ui: this.ui,
        notifications: this.notifications,
        charts: this.charts,
        exportedAt: new Date().toISOString(),
        version: '2.0.0',
      }
      
      const blob = new Blob([JSON.stringify(settings, null, 2)], { type: 'application/json' })
      const url = window.URL.createObjectURL(blob)
      const link = document.createElement('a')
      link.href = url
      link.download = `hydro_settings_${Date.now()}.json`
      link.click()
      window.URL.revokeObjectURL(url)
    },

    // Import settings
    importSettings(settingsJson) {
      try {
        const settings = JSON.parse(settingsJson)
        
        if (settings.ui) Object.assign(this.ui, settings.ui)
        if (settings.notifications) Object.assign(this.notifications, settings.notifications)
        if (settings.charts) Object.assign(this.charts, settings.charts)
        
        // Save to localStorage
        Object.keys(this.ui).forEach(key => {
          localStorage.setItem(key, this.ui[key])
        })
        
        return true
      } catch (e) {
        console.error('Import failed:', e)
        return false
      }
    },
  },
})

