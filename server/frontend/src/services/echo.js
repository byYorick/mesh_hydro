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
  const wsPort = Number(import.meta.env.VITE_WS_PORT || 8080) // Laravel Reverb runs on 8080
  const key = import.meta.env.VITE_PUSHER_KEY || 'hydro-app-key'

  // Debug: показываем конфигурацию
  console.log('🔧 Echo configuration:', {
    wsHost,
    wsPort,
    key,
    env: {
      VITE_WS_HOST: import.meta.env.VITE_WS_HOST,
      VITE_WS_PORT: import.meta.env.VITE_WS_PORT,
      VITE_PUSHER_KEY: import.meta.env.VITE_PUSHER_KEY,
    }
  })

  try {
    echoInstance = new Echo({
      broadcaster: 'pusher',
      key,
      wsHost,
      wsPort,
      forceTLS: false,
      encrypted: false,
      disableStats: true,
      enabledTransports: ['ws', 'wss'],
      // Явно отключаем cluster для самостоятельного Reverb сервера
      cluster: '',  // Пустая строка отключает проверку cluster
      auth: {
        headers: {
          Authorization: `Bearer ${localStorage.getItem('auth_token') || ''}`,
        },
      },
    })
    console.log('✅ Echo instance created successfully')
  } catch (error) {
    console.error('❌ Echo initialization failed:', {
      message: error?.message || 'undefined error',
      error: error,
      stack: error?.stack,
      type: typeof error
    })
    // Start fallback mode immediately
    startFallbackPolling()
    return null
  }

  // Connection event handlers - обернуты в try-catch
  try {
    // Проверяем что socket существует
    if (echoInstance?.connector?.socket) {
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
    } else {
      console.warn('⚠️ Echo created but socket not available, using fallback polling')
      startFallbackPolling()
    }
  } catch (error) {
    console.warn('⚠️ Echo initialization failed, continuing without WebSocket:', error)
    startFallbackPolling()
  }

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
  
  // Polling каждые 10 секунд (уменьшаем нагрузку на API)
  fallbackInterval = setInterval(async () => {
    try {
      // Импортируем API сервис динамически чтобы избежать циклических зависимостей
      const { default: api } = await import('./api.js')
      
      // Получаем свежие данные используя методы API
      const [nodesResponse, eventsResponse] = await Promise.allSettled([
        api.getNodes(),
        api.getEvents({ limit: 10 })
      ])
      
      // Эмулируем события для совместимости с Echo
      if (nodesResponse.status === 'fulfilled') {
        window.dispatchEvent(new CustomEvent('echo:fallback', {
          detail: {
            channel: 'hydro.nodes',
            event: 'NodeStatusChanged',
            data: nodesResponse.value
          }
        }))
      }
      
      if (eventsResponse.status === 'fulfilled') {
        window.dispatchEvent(new CustomEvent('echo:fallback', {
          detail: {
            channel: 'hydro.events',
            event: 'EventCreated',
            data: eventsResponse.value
          }
        }))
      }
    } catch (error) {
      console.warn('Fallback polling error:', error)
    }
  }, 10000)
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

