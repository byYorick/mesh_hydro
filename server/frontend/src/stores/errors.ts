import { Ref, ComputedRef } from 'vue'
import { defineStore } from 'pinia'
import api from '@/services/api'

export const useErrorsStore = defineStore('errors', {
  state: () => ({
    errors: [],
    statistics: null,
    selectedError: null,
    loading: false,
    error: null,
  }),

  getters: {
    // Ошибки для конкретного узла
    errorsByNode: (state) => (nodeId) => {
      return state.errors.filter(e => e.node_id === nodeId)
    },

    // Только нерешенные ошибки
    unresolvedErrors: (state) => {
      return state.errors.filter(e => !e.resolved_at)
    },

    // Только критичные ошибки
    criticalErrors: (state) => {
      return state.errors.filter(e => e.severity === 'critical' && !e.resolved_at)
    },

    // Ошибки по severity
    errorsBySeverity: (state) => (severity) => {
      return state.errors.filter(e => e.severity === severity)
    },

    // Ошибки по типу
    errorsByType: (state) => (type) => {
      return state.errors.filter(e => e.error_type === type)
    },

    // Количество критичных ошибок
    criticalCount: (state) => {
      return state.errors.filter(e => e.severity === 'critical' && !e.resolved_at).length
    },

    // Количество нерешенных ошибок
    unresolvedCount: (state) => {
      return state.errors.filter(e => !e.resolved_at).length
    },
  },

  actions: {
    async fetchErrors(params = {}) {
      this.loading = true
      this.error = null
      
      try {
        const response = await api.getErrors(params)
        this.errors = response || []
        return this.errors
      } catch (error: any) {
        this.error = error.message
        console.error('Error fetching errors:', error)
        throw error
      } finally {
        this.loading = false
      }
    },

    async fetchNodeErrors(nodeId, params = {}) {
      this.loading = true
      this.error = null
      
      try {
        const response = await api.getNodeErrors(nodeId, params)
        return response || []
      } catch (error: any) {
        this.error = error.message
        console.error('Error fetching node errors:', error)
        return []
      } finally {
        this.loading = false
      }
    },

    async fetchErrorStatistics(hours = 24) {
      try {
        const response = await api.getErrorStatistics(hours)
        this.statistics = response
        return this.statistics
      } catch (error: any) {
        console.error('Error fetching error statistics:', error)
        throw error
      }
    },

    async fetchError(id) {
      try {
        const response = await api.getError(id)
        this.selectedError = response
        return this.selectedError
      } catch (error: any) {
        console.error('Error fetching error details:', error)
        throw error
      }
    },

    async resolveError(errorId, resolvedBy = 'manual', resolutionNotes = null) {
      try {
        const result = await api.resolveError(errorId, resolvedBy, resolutionNotes)
        
        // Update in store
        const error = this.errors.find(e => e.id === errorId)
        if (error) {
          error.resolved_at = new Date()
          error.resolved_by = resolvedBy
          error.resolution_notes = resolutionNotes
        }
        
        return result.data || result
      } catch (error: any) {
        console.error('Error resolving error:', error)
        throw error
      }
    },

    async resolveBulkErrors(errorIds, resolvedBy = 'manual', resolutionNotes = null) {
      try {
        const result = await api.resolveBulkErrors(errorIds, resolvedBy, resolutionNotes)
        
        // Update in store
        const now = new Date()
        errorIds.forEach(id => {
          const error = this.errors.find(e => e.id === id)
          if (error) {
            error.resolved_at = now
            error.resolved_by = resolvedBy
            error.resolution_notes = resolutionNotes
          }
        })
        
        return result.data || result
      } catch (error: any) {
        console.error('Error resolving bulk errors:', error)
        throw error
      }
    },

    async deleteError(errorId) {
      try {
        await api.deleteError(errorId)
        
        // Remove from store
        this.errors = this.errors.filter(e => e.id !== errorId)
        
        if (this.selectedError?.id === errorId) {
          this.selectedError = null
        }
      } catch (error: any) {
        console.error('Error deleting error:', error)
        throw error
      }
    },

    // Add error in real-time (from WebSocket)
    addErrorRealtime(errorData) {
      this.errors.unshift(errorData)
      
      // Keep only last 500 errors
      if (this.errors.length > 500) {
        this.errors = this.errors.slice(0, 500)
      }
    },

    // Update error in real-time
    updateErrorRealtime(errorData) {
      const index = this.errors.findIndex(e => e.id === errorData.id)
      
      if (index !== -1) {
        this.errors[index] = {
          ...(this as any).errors[index],
          ...(errorData as any),
        }
      }
      
      if (this.selectedError?.id === errorData.id) {
        this.selectedError = {
          ...(this as any).selectedError,
          ...(errorData as any),
        }
      }
    },

    clearErrors() {
      this.errors = []
      this.selectedError = null
    },
  },
})

