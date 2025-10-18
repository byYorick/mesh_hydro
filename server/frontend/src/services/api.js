import axios from 'axios'

// Create axios instance
const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL || '/api',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
})

// Request interceptor
api.interceptors.request.use(
  (config) => {
    // Add auth token if available
    const token = localStorage.getItem('auth_token')
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  (error) => {
    return Promise.reject(error)
  }
)

// Response interceptor
api.interceptors.response.use(
  (response) => {
    return response.data
  },
  (error) => {
    if (error.response) {
      // Server responded with error status
      console.error('API Error:', error.response.data)
      
      if (error.response.status === 401) {
        // Unauthorized - redirect to login
        localStorage.removeItem('auth_token')
        window.location.href = '/login'
      }
    } else if (error.request) {
      // Request made but no response
      console.error('Network Error:', error.request)
    } else {
      console.error('Error:', error.message)
    }
    
    return Promise.reject(error)
  }
)

// API methods
export default {
  // Health check
  health() {
    return api.get('/health')
  },

  // System status
  getStatus() {
    return api.get('/status')
  },

  // Dashboard
  getDashboardSummary() {
    return api.get('/dashboard/summary')
  },

  // Nodes
  getNodes(params = {}) {
    return api.get('/nodes', { params })
  },

  getNode(nodeId) {
    return api.get(`/nodes/${nodeId}`)
  },

  createNode(data) {
    return api.post('/nodes', data)
  },

  updateNodeConfig(nodeId, config) {
    return api.put(`/nodes/${nodeId}/config`, { config })
  },

  sendCommand(nodeId, command, params = {}) {
    return api.post(`/nodes/${nodeId}/command`, { command, params })
  },

  getNodeStatistics(nodeId, hours = 24) {
    return api.get(`/nodes/${nodeId}/statistics`, { params: { hours } })
  },

  deleteNode(nodeId) {
    return api.delete(`/nodes/${nodeId}`)
  },

  // Telemetry
  getTelemetry(params = {}) {
    return api.get('/telemetry', { params })
  },

  getLatestTelemetry() {
    return api.get('/telemetry/latest')
  },

  getAggregateTelemetry(nodeId, field, hours = 24, interval = '1hour') {
    return api.get('/telemetry/aggregate', {
      params: { node_id: nodeId, field, hours, interval }
    })
  },

  exportTelemetry(params = {}) {
    return api.get('/telemetry/export', {
      params,
      responseType: 'blob',
    })
  },

  // Events
  getEvents(params = {}) {
    return api.get('/events', { params })
  },

  getEvent(id) {
    return api.get(`/events/${id}`)
  },

  resolveEvent(id, resolvedBy = 'manual') {
    return api.post(`/events/${id}/resolve`, { resolved_by: resolvedBy })
  },

  resolveBulkEvents(eventIds, resolvedBy = 'manual') {
    return api.post('/events/resolve-bulk', {
      event_ids: eventIds,
      resolved_by: resolvedBy,
    })
  },

  getEventStatistics(hours = 24) {
    return api.get('/events/statistics', { params: { hours } })
  },

  deleteEvent(id) {
    return api.delete(`/events/${id}`)
  },
}

// Export axios instance for direct use if needed
export { api as axios }

