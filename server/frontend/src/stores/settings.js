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

    // PID Settings
    pid: {
      ph: {
        // Основные PID параметры
        kp: parseFloat(localStorage.getItem('pid_ph_kp') || '0.3'),
        ki: parseFloat(localStorage.getItem('pid_ph_ki') || '0.01'),
        kd: parseFloat(localStorage.getItem('pid_ph_kd') || '0.0'),
        setpoint: parseFloat(localStorage.getItem('pid_ph_setpoint') || '5.8'),
        deadband: parseFloat(localStorage.getItem('pid_ph_deadband') || '0.05'),
        
        // Тайминги
        doseMinInterval: parseInt(localStorage.getItem('pid_ph_doseMinInterval') || '90'),
        mixDelay: parseInt(localStorage.getItem('pid_ph_mixDelay') || '45'),
        
        // Anti-windup
        integralMax: parseFloat(localStorage.getItem('pid_ph_integralMax') || '10.0'),
        integralMin: parseFloat(localStorage.getItem('pid_ph_integralMin') || '-10.0'),
        
        // Ограничения выхода
        outputMax: parseFloat(localStorage.getItem('pid_ph_outputMax') || '5.0'),
        outputMin: parseFloat(localStorage.getItem('pid_ph_outputMin') || '-5.0'),
        
        // Фильтрация
        filterAlpha: parseFloat(localStorage.getItem('pid_ph_filterAlpha') || '0.25'),
        
        // Управление
        enabled: localStorage.getItem('pid_ph_enabled') !== 'false',
      },
      ec: {
        // Основные PID параметры
        kp: parseFloat(localStorage.getItem('pid_ec_kp') || '0.2'),
        ki: parseFloat(localStorage.getItem('pid_ec_ki') || '0.01'),
        kd: parseFloat(localStorage.getItem('pid_ec_kd') || '0.0'),
        setpoint: parseFloat(localStorage.getItem('pid_ec_setpoint') || '1.6'),
        deadband: parseFloat(localStorage.getItem('pid_ec_deadband') || '0.08'),
        
        // Тайминги
        doseMinInterval: parseInt(localStorage.getItem('pid_ec_doseMinInterval') || '180'),
        mixDelay: parseInt(localStorage.getItem('pid_ec_mixDelay') || '60'),
        componentABDelay: parseInt(localStorage.getItem('pid_ec_componentABDelay') || '30'),
        
        // Anti-windup
        integralMax: parseFloat(localStorage.getItem('pid_ec_integralMax') || '8.0'),
        integralMin: parseFloat(localStorage.getItem('pid_ec_integralMin') || '-8.0'),
        
        // Ограничения выхода
        outputMax: parseFloat(localStorage.getItem('pid_ec_outputMax') || '10.0'),
        outputMin: parseFloat(localStorage.getItem('pid_ec_outputMin') || '0.0'),
        
        // Фильтрация
        filterAlpha: parseFloat(localStorage.getItem('pid_ec_filterAlpha') || '0.4'),
        
        // Управление
        enabled: localStorage.getItem('pid_ec_enabled') !== 'false',
      },
    },

    // Safety Settings
    safety: {
      dosing: {
        maxDailyPhDown: parseInt(localStorage.getItem('safe_maxDailyPhDown') || '500'),
        maxDailyPhUp: parseInt(localStorage.getItem('safe_maxDailyPhUp') || '300'),
        maxDailyNutrients: parseInt(localStorage.getItem('safe_maxDailyNutrients') || '2000'),
        minWaterLevel: parseInt(localStorage.getItem('safe_minWaterLevel') || '20'),
        tempLockout: localStorage.getItem('safe_tempLockout') !== 'false',
      },
      sensors: {
        phCalibrationDueDays: parseInt(localStorage.getItem('safe_phCalibrationDueDays') || '14'),
        ecCalibrationDueDays: parseInt(localStorage.getItem('safe_ecCalibrationDueDays') || '30'),
        tempCompensation: localStorage.getItem('safe_tempCompensation') !== 'false',
      },
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

    // PID setters
    setPid(group, key, value) {
      this.pid[group][key] = value
      localStorage.setItem(`pid_${group}_${key}`, value)
    },

    // Safety setters
    setSafety(group, key, value) {
      this.safety[group][key] = value
      localStorage.setItem(`safe_${key}`, value)
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

