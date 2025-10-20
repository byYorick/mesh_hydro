import Echo from 'laravel-echo'
import Pusher from 'pusher-js'

/**
 * Laravel Echo instance для real-time обновлений
 */
let echoInstance = null
let fallbackInterval = null
let isWebSocketConnected = false
let fallbackMode = false

export function initializeEcho() {
  if (echoInstance) {
    return echoInstance
  }

  // Reverb (Pusher protocol) settings
  window.Pusher = Pusher
  const wsHost = import.meta.env.VITE_WS_HOST || window.location.hostname
  const wsPort = Number(import.meta.env.VITE_WS_PORT || 6001)
  const key = import.meta.env.VITE_PUSHER_KEY || 'hydro-app-key'

  echoInstance = new Echo({
    broadcaster: 'pusher',
    key,
    wsHost,
    wsPort,
    forceTLS: false,
    encrypted: false,
    disableStats: true,
    enabledTransports: ['ws', 'wss'],
    auth: {
      headers: {
        Authorization: `Bearer ${localStorage.getItem('auth_token') || ''}`,
      },
    },
  })

  // Connection event handlers
  echoInstance.connector.socket.on('connect', () => {
    console.log('✅ WebSocket connected')
    isWebSocketConnected = true
    fallbackMode = false
    stopFallbackPolling()
  })

  echoInstance.connector.socket.on('disconnect', (reason) => {
    console.warn('⚠️ WebSocket disconnected:', reason)
    isWebSocketConnected = false
    startFallbackPolling()
  })

  echoInstance.connector.socket.on('reconnect', (attemptNumber) => {
    console.log(`🔄 WebSocket reconnected (attempt ${attemptNumber})`)
    isWebSocketConnected = true
    fallbackMode = false
    stopFallbackPolling()
  })

  echoInstance.connector.socket.on('reconnect_error', (error) => {
    console.error('❌ WebSocket reconnect error:', error)
    isWebSocketConnected = false
    startFallbackPolling()
  })

  echoInstance.connector.socket.on('connect_error', (error) => {
    console.error('❌ WebSocket connection error:', error)
    isWebSocketConnected = false
    startFallbackPolling()
  })

  return echoInstance
}

export function getEcho() {
  if (!echoInstance) {
    return initializeEcho()
  }
  return echoInstance
}

export function disconnectEcho() {
  if (echoInstance) {
    echoInstance.disconnect()
    echoInstance = null
    console.log('👋 WebSocket disconnected')
  }
  stopFallbackPolling()
  isWebSocketConnected = false
  fallbackMode = false
}

/**
 * Fallback polling при недоступности WebSocket
 */
function startFallbackPolling() {
  if (fallbackMode || fallbackInterval) {
    return
  }
  
  console.log('🔄 Starting fallback polling mode')
  fallbackMode = true
  
  // Polling каждые 5 секунд
  fallbackInterval = setInterval(async () => {
    try {
      // Импортируем API сервис динамически чтобы избежать циклических зависимостей
      const { default: api } = await import('./api.js')
      
      // Получаем свежие данные
      const [nodesResponse, eventsResponse] = await Promise.allSettled([
        api.get('/api/nodes'),
        api.get('/api/events?limit=10')
      ])
      
      // Эмулируем события для совместимости с Echo
      if (nodesResponse.status === 'fulfilled') {
        window.dispatchEvent(new CustomEvent('echo:fallback', {
          detail: {
            channel: 'hydro.nodes',
            event: 'NodeStatusChanged',
            data: nodesResponse.value.data
          }
        }))
      }
      
      if (eventsResponse.status === 'fulfilled') {
        window.dispatchEvent(new CustomEvent('echo:fallback', {
          detail: {
            channel: 'hydro.events',
            event: 'EventCreated',
            data: eventsResponse.value.data
          }
        }))
      }
    } catch (error) {
      console.warn('Fallback polling error:', error)
    }
  }, 5000)
}

function stopFallbackPolling() {
  if (fallbackInterval) {
    clearInterval(fallbackInterval)
    fallbackInterval = null
    console.log('✅ Fallback polling stopped')
  }
}

/**
 * Проверка статуса соединения
 */
export function getConnectionStatus() {
  return {
    isWebSocketConnected,
    fallbackMode,
    hasEcho: !!echoInstance
  }
}

export default {
  initialize: initializeEcho,
  get: getEcho,
  disconnect: disconnectEcho,
  getConnectionStatus,
}

