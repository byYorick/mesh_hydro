import { defineStore } from 'pinia'
import api from '@/services/api'

export const useTelemetryStore = defineStore('telemetry', {
  state: () => ({
    telemetry: [],
    latestTelemetry: [],
    aggregatedData: {},
    loading: false,
    error: null,
  }),

  getters: {
    // Get telemetry for specific node
    getTelemetryByNode: (state) => (nodeId) => {
      return state.telemetry.filter(t => t.node_id === nodeId)
    },

    // Get latest telemetry for specific node
    getLatestByNode: (state) => (nodeId) => {
      return state.latestTelemetry.find(t => t.node_id === nodeId)
    },
  },

  actions: {
    async fetchTelemetry(params = {}) {
      this.loading = true
      this.error = null
      
      try {
        const response = await api.getTelemetry(params)
        this.telemetry = response.data || response
        return this.telemetry
      } catch (error) {
        this.error = error.message
        console.error('Error fetching telemetry:', error)
        throw error
      } finally {
        this.loading = false
      }
    },

    async fetchLatestTelemetry() {
      this.loading = true
      this.error = null
      
      try {
        this.latestTelemetry = await api.getLatestTelemetry()
        return this.latestTelemetry
      } catch (error) {
        this.error = error.message
        console.error('Error fetching latest telemetry:', error)
        throw error
      } finally {
        this.loading = false
      }
    },

    async fetchAggregatedData(nodeId, field, hours = 24, interval = '1hour') {
      this.loading = true
      this.error = null
      
      try {
        const data = await api.getAggregateTelemetry(nodeId, field, hours, interval)
        
        // Store in aggregatedData by key
        const key = `${nodeId}_${field}_${hours}_${interval}`
        this.aggregatedData[key] = data
        
        return data
      } catch (error) {
        this.error = error.message
        console.error('Error fetching aggregated data:', error)
        throw error
      } finally {
        this.loading = false
      }
    },

    async exportTelemetry(params = {}) {
      try {
        const blob = await api.exportTelemetry(params)
        
        // Create download link
        const url = window.URL.createObjectURL(blob)
        const link = document.createElement('a')
        link.href = url
        link.setAttribute('download', `telemetry_export_${Date.now()}.csv`)
        document.body.appendChild(link)
        link.click()
        link.remove()
        window.URL.revokeObjectURL(url)
      } catch (error) {
        console.error('Error exporting telemetry:', error)
        throw error
      }
    },

    // Add telemetry in real-time (from WebSocket)
    addTelemetryRealtime(data) {
      // Add to telemetry array
      this.telemetry.unshift(data)
      
      // Keep only last 1000 items
      if (this.telemetry.length > 1000) {
        this.telemetry = this.telemetry.slice(0, 1000)
      }
      
      // Update latest telemetry
      const index = this.latestTelemetry.findIndex(t => t.node_id === data.node_id)
      if (index !== -1) {
        this.latestTelemetry[index] = data
      } else {
        this.latestTelemetry.push(data)
      }
    },

    clearTelemetry() {
      this.telemetry = []
      this.aggregatedData = {}
    },
  },
})

