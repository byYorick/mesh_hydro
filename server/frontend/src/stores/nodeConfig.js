import { defineStore } from 'pinia'
import api from '@/services/api'

export const useNodeConfigStore = defineStore('nodeConfig', {
  state: () => ({
    nodeId: null,
    loading: false,
    saving: false,
    calibrating: false,
    config: null,
    history: [],
    calibrations: [],
    error: null,
  }),

  getters: {
    hasConfig: (state) => state.config !== null,
    isOnline: (state) => false, // TODO: integrate with nodes store
    hasHistory: (state) => state.history.length > 0,
    hasCalibrations: (state) => state.calibrations.length > 0,
  },

  actions: {
    async loadConfig(nodeIdParam) {
      if (!nodeIdParam) return
      
      this.nodeId = nodeIdParam
      this.loading = true
      this.error = null

      try {
        const response = await api.get(`/nodes/${nodeIdParam}`)
        this.config = response.config || {}
        console.log('✅ Config loaded for', nodeIdParam, this.config)
      } catch (err) {
        this.error = err.response?.data?.message || 'Failed to load config'
        console.error('❌ Failed to load config:', err)
        throw err
      } finally {
        this.loading = false
      }
    },

    async requestConfig(nodeIdParam) {
      if (!nodeIdParam) return
      
      this.saving = true
      this.error = null

      try {
        const response = await api.get(`/nodes/${nodeIdParam}/config/request`)
        console.log('✅ Config requested:', response)
        
        await this.loadConfig(nodeIdParam)
      } catch (err) {
        this.error = err.response?.data?.message || 'Failed to request config'
        console.error('❌ Failed to request config:', err)
        throw err
      } finally {
        this.saving = false
      }
    },

    async saveConfig(nodeIdParam, changes, comment = '') {
      if (!nodeIdParam) return
      
      this.saving = true
      this.error = null

      try {
        const response = await api.put(`/nodes/${nodeIdParam}/config`, {
          config: changes,
          comment: comment
        })
        
        console.log('✅ Config saved:', response)
        await this.loadConfig(nodeIdParam)
        
        return response
      } catch (err) {
        this.error = err.response?.data?.message || 'Failed to save config'
        console.error('❌ Failed to save config:', err)
        throw err
      } finally {
        this.saving = false
      }
    },

    async calibratePump(nodeIdParam, params) {
      if (!nodeIdParam) return
      
      this.calibrating = true
      this.error = null

      try {
        const response = await api.post(`/nodes/${nodeIdParam}/pump/calibrate`, {
          pump_id: params.pump_id,
          duration_sec: params.duration_sec,
          volume_ml: params.volume_ml
        })
        
        console.log('✅ Pump calibrated:', response)
        
        await Promise.all([
          this.loadConfig(nodeIdParam),
          this.loadCalibrations(nodeIdParam)
        ])
        
        return response
      } catch (err) {
        this.error = err.response?.data?.message || 'Failed to calibrate pump'
        console.error('❌ Failed to calibrate pump:', err)
        throw err
      } finally {
        this.calibrating = false
      }
    },

    async runPumpManually(nodeIdParam, params) {
      if (!nodeIdParam) return
      
      this.calibrating = true
      this.error = null

      try {
        const response = await api.post(`/nodes/${nodeIdParam}/pump/run`, {
          pump_id: params.pump_id,
          duration_sec: params.duration_sec
        })
        
        console.log('✅ Pump running:', response)
        return response
      } catch (err) {
        this.error = err.response?.data?.message || 'Failed to run pump'
        console.error('❌ Failed to run pump:', err)
        throw err
      } finally {
        this.calibrating = false
      }
    },

    async loadHistory(nodeIdParam) {
      if (!nodeIdParam) return
      
      try {
        const response = await api.get(`/nodes/${nodeIdParam}/config/history`)
        this.history = response.history || []
        
        console.log('✅ History loaded:', this.history.length, 'items')
      } catch (err) {
        console.error('❌ Failed to load history:', err)
        this.history = []
      }
    },

    async loadCalibrations(nodeIdParam) {
      if (!nodeIdParam) return
      
      try {
        const response = await api.get(`/nodes/${nodeIdParam}/pump/calibrations`)
        this.calibrations = response.calibrations || []
        
        console.log('✅ Calibrations loaded:', this.calibrations.length, 'items')
      } catch (err) {
        console.error('❌ Failed to load calibrations:', err)
        this.calibrations = []
      }
    },

    reset() {
      this.nodeId = null
      this.loading = false
      this.saving = false
      this.calibrating = false
      this.config = null
      this.history = []
      this.calibrations = []
      this.error = null
    }
  }
})
