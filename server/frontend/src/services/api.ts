import { AxiosResponse, AxiosError } from 'axios'
import axios from 'axios'

// Create axios instance
const api = axios.create({
  baseURL: import.meta.env.VITE_API_URL || '/api',
  timeout: 15000, // 15 seconds - увеличен для медленных запросов (status, dashboard)
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
    // Отключить кеш для dev режима
    config.headers['Cache-Control'] = 'no-cache, no-store, must-revalidate'
    config.headers['Pragma'] = 'no-cache'
    config.headers['Expires'] = '0'
    // Добавляем timestamp для предотвращения кеширования
    if (config.method === 'get') {
      config.params = { ...config.params, _t: Date.now() }
    }
    return config
  },
  (error) => {
    return Promise.reject(error)
  }
)

// Response interceptor with improved retry logic
api.interceptors.response.use(
  (response) => {
    return response.data
  },
  async (error) => {
    const config = error.config
    
    // Retry logic для network errors
    if (!config || !config.retry) {
      config.retry = { count: 0, limit: 3, delay: 1000 }
    }
    
    // Не retry для определенных статусов
    const noRetryStatuses: any[] = [400, 401, 403, 404, 422]
    try {
      // Дополнительная проверка на undefined/null перед вызовом includes
      if (error && error.response && error.response.status && noRetryStatuses && Array.isArray(noRetryStatuses) && noRetryStatuses.includes(error.response.status)) {
        return handleError(error)
      }
    } catch (includesError: any) {
      console.error('api.js: retry logic - Error in includes (noRetryStatuses):', includesError)
      console.error('api.js: retry logic - error.response.status:', error?.response?.status, typeof error?.response?.status)
      console.error('api.js: retry logic - noRetryStatuses:', noRetryStatuses, typeof noRetryStatuses)
    }
    
    // Retry только для определенных 5xx ошибок
    const retryableStatuses: any[] = [500, 502, 503, 504]
    let isRetryableError = false
    try {
      // Дополнительная проверка на undefined/null перед вызовом includes
      if (!error || !error.response) {
        isRetryableError = false
      } else if (error.response.status >= 500 && error.response.status && retryableStatuses && Array.isArray(retryableStatuses)) {
        isRetryableError = retryableStatuses.includes(error.response.status)
      } else {
        isRetryableError = false
      }
    } catch (includesError: any) {
      console.error('api.js: retry logic - Error in includes (retryableStatuses):', includesError)
      console.error('api.js: retry logic - error.response.status:', error?.response?.status, typeof error?.response?.status)
      console.error('api.js: retry logic - retryableStatuses:', retryableStatuses, typeof retryableStatuses)
      isRetryableError = false
    }
    
    if (isRetryableError) {
      config.retry.count += 1
      
      if (config.retry.count < config.retry.limit) {
        console.log(`🔄 Retrying request (${config.retry.count}/${config.retry.limit})...`)
        
        // Exponential backoff with jitter
        const baseDelay = config.retry.delay * Math.pow(2, config.retry.count - 1)
        const jitter = Math.random() * 1000 // Добавляем случайность
        const delay = Math.min(baseDelay + jitter, 10000) // Максимум 10 секунд
        
        await new Promise(resolve => setTimeout(resolve, delay))
        
        return api.request(config)
      } else {
        console.error(`❌ Max retries exceeded for ${config.url}`)
      }
    }
    
    return handleError(error)
  }
)

function handleError(error: any): any {
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

  // Status thresholds configuration
  getStatusThresholds() {
    return api.get('/status/thresholds')
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

  async createNode(nodeData) {
    console.log('API: Creating node with data:', nodeData)
    try {
      // Note: response interceptor already extracts .data
      const response = await api.post('/nodes', nodeData)
      console.log('API: Node creation response:', response)
      return response
    } catch (error: any) {
      console.error('API: Node creation error:', error.response?.data)
      throw error
    }
  },

  updateNode(nodeId, data) {
    return api.put(`/nodes/${nodeId}`, data)
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

  // Pump operations
  runPump(nodeId, pumpId, durationSec) {
    return api.post(`/nodes/${nodeId}/pump/run`, {
      pump_id: pumpId,
      duration_sec: durationSec
    })
  },

  stopPump(nodeId, pumpId) {
    return api.post(`/nodes/${nodeId}/pump/stop`, {
      pump_id: pumpId
    })
  },

  calibratePump(nodeId, pumpId, durationSec, volumeMl) {
    const data: any = {
      pump_id: pumpId,
      duration_sec: durationSec,
      volume_ml: volumeMl
    }
    console.log('API calibratePump request:', { nodeId, data })
    return api.post(`/nodes/${nodeId}/pump/calibrate`, data)
  },

  getPumpCalibrations(nodeId) {
    return api.get(`/nodes/${nodeId}/pump/calibrations`)
  },

  requestConfig(nodeId) {
    return api.get(`/nodes/${nodeId}/config/request`)
  },

  getConfigHistory(nodeId) {
    return api.get(`/nodes/${nodeId}/config/history`)
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

  // Errors
  getErrors(params = {}) {
    return api.get('/errors', { params })
  },

  getError(id) {
    return api.get(`/errors/${id}`)
  },

  getNodeErrors(nodeId, params = {}) {
    return api.get(`/nodes/${nodeId}/errors`, { params })
  },

  getErrorStatistics(hours = 24) {
    return api.get('/errors/statistics', { params: { hours } })
  },

  resolveError(errorId, resolvedBy = 'manual', resolutionNotes = null) {
    return api.post(`/errors/${errorId}/resolve`, {
      resolved_by: resolvedBy,
      resolution_notes: resolutionNotes,
    })
  },

  resolveBulkErrors(errorIds, resolvedBy = 'manual', resolutionNotes = null) {
    return api.post('/errors/resolve-bulk', {
      error_ids: errorIds,
      resolved_by: resolvedBy,
      resolution_notes: resolutionNotes,
    })
  },

  deleteError(errorId) {
    return api.delete(`/errors/${errorId}`)
  },

  // Settings
  getSettings() {
    return api.get('/settings')
  },

  getTelegramSettings() {
    return api.get('/settings/telegram')
  },

  getTelegramChatId() {
    return api.get('/settings/telegram/chat-id')
  },

  saveTelegramSettings(settings) {
    return api.post('/settings/telegram', settings)
  },

  testTelegram() {
    return api.post('/settings/telegram/test')
  },

  // Docker management
  getDockerStatus() {
    return api.get('/docker/status')
  },

  getDockerLogs() {
    return api.get('/docker/logs')
  },

  restartAllContainers() {
    return api.post('/docker/restart/all')
  },

  restartContainer(containerName) {
    return api.post('/docker/restart/container', { container: containerName })
  },

  startAllContainers() {
    return api.post('/docker/start/all')
  },

  stopAllContainers() {
    return api.post('/docker/stop/all')
  },

  // PID Presets
  getPidPresets() {
    return api.get('/pid-presets')
  },

  getPidPreset(id) {
    return api.get(`/pid-presets/${id}`)
  },

  createPidPreset(data) {
    return api.post('/pid-presets', data)
  },

  updatePidPreset(id, data) {
    return api.put(`/pid-presets/${id}`, data)
  },

  deletePidPreset(id) {
    return api.delete(`/pid-presets/${id}`)
  },

  // Schedules
  getNodeSchedules(nodeId) {
    return api.get(`/schedules/node/${nodeId}`)
  },

  getActiveSchedules(nodeId) {
    return api.get(`/schedules/node/${nodeId}/active`)
  },

  createSchedule(nodeId, data) {
    return api.post(`/schedules/node/${nodeId}`, data)
  },

  updateSchedule(nodeId, scheduleId, data) {
    return api.put(`/schedules/node/${nodeId}/${scheduleId}`, data)
  },

  deleteSchedule(nodeId, scheduleId) {
    return api.delete(`/schedules/node/${nodeId}/${scheduleId}`)
  },

  // System Control Commands
  setPhTarget(nodeId, phTarget) {
    return api.post(`/nodes/${nodeId}/command`, {
      command: 'set_ph_target',
      params: { ph_target: phTarget }
    })
  },

  setEcTarget(nodeId, ecTarget) {
    return api.post(`/nodes/${nodeId}/command`, {
      command: 'set_ec_target',
      params: { ec_target: ecTarget }
    })
  },

  setAutonomousMode(nodeId, enable) {
    return api.post(`/nodes/${nodeId}/command`, {
      command: 'set_autonomous_mode',
      params: { enable: enable }
    })
  },

  setSafetySettings(nodeId, settings) {
    return api.post(`/nodes/${nodeId}/command`, {
      command: 'set_safety_settings',
      params: settings
    })
  },

  forceMockMode(nodeId, enable) {
    return api.post(`/nodes/${nodeId}/command`, {
      command: 'force_mock_mode',
      params: { enable: enable }
    })
  },

  emergencyStop(nodeId) {
    return api.post(`/nodes/${nodeId}/command`, {
      command: 'emergency_stop',
      params: {}
    })
  },

  resetEmergency(nodeId) {
    return api.post(`/nodes/${nodeId}/command`, {
      command: 'reset_emergency',
      params: {}
    })
  },

  getSensorStatus(nodeId) {
    return api.post(`/nodes/${nodeId}/command`, {
      command: 'get_sensor_status',
      params: {}
    })
  },
}

// Export axios instance for direct use if needed
export { api as axios }



