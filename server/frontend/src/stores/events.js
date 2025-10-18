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
      return state.events.filter(e => 
        ['critical', 'emergency'].includes(e.level) && !e.resolved_at
      )
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
      return state.events.filter(e => 
        ['critical', 'emergency'].includes(e.level) && !e.resolved_at
      ).length
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

