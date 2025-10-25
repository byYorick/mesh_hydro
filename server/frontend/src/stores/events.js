import { defineStore } from 'pinia'
import api from '@/services/api'

export const useEventsStore = defineStore('events', {
  state: () => ({
    events: [],
    statistics: null,
    loading: false,
    error: null,
  }),

  getters: {
    // Get active events
    activeEvents: (state) => {
      return state.events.filter(e => !e.resolved_at)
    },

    // Get resolved events
    resolvedEvents: (state) => {
      return state.events.filter(e => e.resolved_at)
    },

    // Get critical events
    criticalEvents: (state) => {
      console.log('🔍 events.js: criticalEvents getter called')
      console.log('🔍 events.js: state.events:', state.events)
      
      if (!state.events || !Array.isArray(state.events)) {
        console.log('🔍 events.js: state.events is not valid array')
        return []
      }
      
      const criticalLevels = ['critical', 'emergency']
      console.log('🔍 events.js: criticalLevels:', criticalLevels)
      
      try {
        console.log('🔍 events.js: Starting filter operation')
        const result = state.events.filter((e, index) => {
          console.log(`🔍 events.js: Processing event ${index}:`, e)
          
          if (!e) {
            console.warn('events.js: criticalEvents - event is null/undefined:', e)
            return false
          }
          if (!e.level) {
            console.warn('events.js: criticalEvents - event.level is null/undefined:', e)
            return false
          }
          if (typeof e.level !== 'string') {
            console.warn('events.js: criticalEvents - event.level is not string:', e.level, typeof e.level)
            return false
          }
          if (!criticalLevels || !Array.isArray(criticalLevels)) {
            console.error('events.js: criticalEvents - criticalLevels is not array:', criticalLevels)
            return false
          }
          
          console.log(`🔍 events.js: Checking includes for level: ${e.level}`)
          let includesResult = false
          try {
            // Дополнительная проверка на undefined/null перед вызовом includes
            if (criticalLevels && Array.isArray(criticalLevels) && e.level) {
              includesResult = criticalLevels.includes(e.level)
            }
            console.log(`🔍 events.js: includes result:`, includesResult)
          } catch (includesError) {
            console.error('events.js: criticalEvents - Error in includes:', includesError)
            console.error('events.js: criticalEvents - criticalLevels:', criticalLevels, typeof criticalLevels)
            console.error('events.js: criticalEvents - e.level:', e.level, typeof e.level)
            includesResult = false
          }
          
          return includesResult && !e.resolved_at
        })
        console.log('🔍 events.js: Filter completed, result:', result)
        return result
      } catch (error) {
        console.error('events.js: criticalEvents - Error in filter:', error)
        console.error('events.js: criticalEvents - state.events:', state.events)
        console.error('events.js: criticalEvents - criticalLevels:', criticalLevels)
        return []
      }
    },

    // Get events by level
    eventsByLevel: (state) => (level) => {
      return state.events.filter(e => e.level === level)
    },

    // Get events by node
    eventsByNode: (state) => (nodeId) => {
      return state.events.filter(e => e.node_id === nodeId)
    },

    // Count active critical events
    criticalCount: (state) => {
      if (!state.events || !Array.isArray(state.events)) return 0
      const criticalLevels = ['critical', 'emergency']
      try {
        return state.events.filter(e => {
          if (!e) {
            console.warn('events.js: criticalCount - event is null/undefined:', e)
            return false
          }
          if (!e.level) {
            console.warn('events.js: criticalCount - event.level is null/undefined:', e)
            return false
          }
          if (typeof e.level !== 'string') {
            console.warn('events.js: criticalCount - event.level is not string:', e.level, typeof e.level)
            return false
          }
          if (!criticalLevels || !Array.isArray(criticalLevels)) {
            console.error('events.js: criticalCount - criticalLevels is not array:', criticalLevels)
            return false
          }
          try {
            // Дополнительная проверка на undefined/null перед вызовом includes
            if (criticalLevels && Array.isArray(criticalLevels) && e.level) {
              return criticalLevels.includes(e.level) && !e.resolved_at
            }
            return false
          } catch (includesError) {
            console.error('events.js: criticalCount - Error in includes:', includesError)
            console.error('events.js: criticalCount - criticalLevels:', criticalLevels, typeof criticalLevels)
            console.error('events.js: criticalCount - e.level:', e.level, typeof e.level)
            return false
          }
        }).length
      } catch (error) {
        console.error('events.js: criticalCount - Error in filter:', error)
        console.error('events.js: criticalCount - state.events:', state.events)
        console.error('events.js: criticalCount - criticalLevels:', criticalLevels)
        return 0
      }
    },
  },

  actions: {
    async fetchEvents(params = {}) {
      this.loading = true
      this.error = null
      
      try {
        this.events = await api.getEvents(params)
        return this.events
      } catch (error) {
        this.error = error.message
        console.error('Error fetching events:', error)
        throw error
      } finally {
        this.loading = false
      }
    },

    async fetchStatistics(hours = 24) {
      try {
        this.statistics = await api.getEventStatistics(hours)
        return this.statistics
      } catch (error) {
        console.error('Error fetching event statistics:', error)
        throw error
      }
    },

    async resolveEvent(eventId, resolvedBy = 'manual') {
      try {
        const result = await api.resolveEvent(eventId, resolvedBy)
        
        // Update in store
        const event = this.events.find(e => e.id === eventId)
        if (event) {
          event.resolved_at = new Date()
          event.resolved_by = resolvedBy
        }
        
        return result
      } catch (error) {
        console.error('Error resolving event:', error)
        throw error
      }
    },

    async resolveBulk(eventIds, resolvedBy = 'manual') {
      try {
        const result = await api.resolveBulkEvents(eventIds, resolvedBy)
        
        // Update in store
        const now = new Date()
        eventIds.forEach(id => {
          const event = this.events.find(e => e.id === id)
          if (event) {
            event.resolved_at = now
            event.resolved_by = resolvedBy
          }
        })
        
        return result
      } catch (error) {
        console.error('Error resolving bulk events:', error)
        throw error
      }
    },

    async deleteEvent(eventId) {
      try {
        await api.deleteEvent(eventId)
        
        // Remove from store
        this.events = this.events.filter(e => e.id !== eventId)
      } catch (error) {
        console.error('Error deleting event:', error)
        throw error
      }
    },

    // Add event in real-time (from WebSocket)
    addEventRealtime(event) {
      this.events.unshift(event)
      
      // Keep only last 500 events
      if (this.events.length > 500) {
        this.events = this.events.slice(0, 500)
      }
    },
  },
})

