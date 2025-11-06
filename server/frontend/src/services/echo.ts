import { AxiosResponse, AxiosError } from 'axios'
import Echo from 'laravel-echo'
import Pusher from 'pusher-js'

/**
 * Laravel Echo instance для real-time обновлений
 */
let echoInstance = null
let fallbackInterval = null
let isWebSocketConnected = false
let fallbackMode = false
let reconnectAttempts = 0
let maxReconnectAttempts = 5
let reconnectTimeout = null

export function initializeEcho(): any {
  if (echoInstance) {
    return echoInstance
  }

  // Reverb (Pusher protocol) settings
  window.Pusher = Pusher
  
  // В dev режиме используем прокси Vite для WebSocket (через /app)
  // Прокси Vite автоматически перенаправляет WebSocket запросы на reverb:8080
  const isDev = import.meta.env.DEV
  const key = import.meta.env.VITE_PUSHER_KEY || 'hydro-app-key'
  
  // В dev режиме подключаемся через прокси Vite на localhost:5173/app
  // В production подключаемся напрямую к Reverb серверу
  let wsHost: string
  let wsPort: number | string
  let wsPath: string
  
  if (isDev) {
    // Используем прокси Vite - подключаемся к localhost:5173, прокси перенаправит на reverb:8080
    // Pusher автоматически добавляет /app перед ключом при использовании Laravel Reverb
    // Поэтому wsPath должен быть пустым, чтобы Pusher создал путь /app/{key}
    // Прокси Vite настроен на /app -> http://reverb:8080, поэтому путь /app/{key} будет проксирован правильно
    wsHost = window.location.hostname
    wsPort = window.location.port || 5173
    wsPath = ''  // Пустой - Pusher сам добавит /app/{key}
  } else {
    // Production - прямое подключение к Reverb
    wsHost = import.meta.env.VITE_WS_HOST || 'localhost'
    wsPort = Number(import.meta.env.VITE_WS_PORT || 8080)
    wsPath = ''
  }

  // Debug: показываем конфигурацию
  console.log('🔧 Echo configuration:', {
    wsHost,
    wsPort,
    wsPath,
    key,
    isDev,
    env: {
      VITE_WS_HOST: import.meta.env.VITE_WS_HOST,
      VITE_WS_PORT: import.meta.env.VITE_WS_PORT,
      VITE_PUSHER_KEY: import.meta.env.VITE_PUSHER_KEY,
      DEV: import.meta.env.DEV,
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
      enabledTransports: ['ws'],
      cluster: '',
      authEndpoint: '/broadcasting/auth',
      auth: {
        headers: {
          Authorization: `Bearer ${localStorage.getItem('auth_token') || ''}`,
        },
      },
      // Дополнительные настройки для Reverb
      // wsPath не используется для Laravel Reverb - Pusher автоматически добавляет /app
      // Прокси Vite настроен на /app -> http://reverb:8080
      activityTimeout: 30000,
      pongTimeout: 6000,
    })
    console.log('✅ Echo instance created successfully')
    
    // Дополнительная диагностика
    console.log('🔍 Echo instance details:', {
      broadcaster: echoInstance.broadcaster,
      key: echoInstance.key,
      wsHost: echoInstance.wsHost,
      wsPort: echoInstance.wsPort,
      connector: !!echoInstance.connector,
      connectorType: typeof echoInstance.connector
    })
    
    // Принудительно инициализируем соединение
    if (echoInstance.connector) {
      console.log('🔧 Echo connector found:', {
        type: typeof echoInstance.connector,
        hasConnect: typeof echoInstance.connector.connect === 'function',
        hasSocket: !!echoInstance.connector.socket,
        connectorKeys: Object.keys(echoInstance.connector)
      })
      
      // Попробуем разные способы инициализации
      if (typeof echoInstance.connector.connect === 'function') {
        console.log('🔧 Triggering connector.connect()')
        try {
          echoInstance.connector.connect()
          console.log('✅ Manual connection triggered')
        } catch (connectError: any) {
          console.error('❌ Manual connection failed:', connectError)
        }
      }
      
      // Если есть socket, попробуем его инициализировать
      if (echoInstance.connector.socket && typeof echoInstance.connector.socket.connect === 'function') {
        console.log('🔧 Triggering socket.connect()')
        try {
          echoInstance.connector.socket.connect()
          console.log('✅ Socket connection triggered')
        } catch (socketError: any) {
          console.error('❌ Socket connection failed:', socketError)
        }
      }
      
      // Попробуем создать socket вручную
      if (!echoInstance.connector.socket) {
        console.log('🔧 Creating socket manually')
        try {
          // Исправляем URL - Pusher автоматически добавляет /app, поэтому путь: /app/{key}
          const socketUrl = `ws://${wsHost}:${wsPort}/app/${key}?protocol=7&client=js&version=8.4.0&flash=false`
          console.log('🔍 Manual socket URL:', socketUrl)
          const manualSocket = new WebSocket(socketUrl)
          
          manualSocket.onopen = () => {
            console.log('✅ Manual socket connected')
            // Присваиваем socket к connector
            if (echoInstance.connector) {
              echoInstance.connector.socket = manualSocket
              console.log('✅ Socket assigned to connector')
              
              // WebSocket уже подключен, переключаемся на real-time режим
              console.log('✅ Manual socket: connected event')
              if (fallbackMode) {
                console.log('🔄 Switching from fallback to real-time mode')
                stopFallbackPolling()
                fallbackMode = false
              }
            }
          }
          
          manualSocket.onerror = (error) => {
            console.error('❌ Manual socket failed:', error)
          }
          
          manualSocket.onclose = (event) => {
            console.log('🔍 Manual socket closed:', event.code, event.reason)
          }
        } catch (manualError: any) {
          console.error('❌ Manual socket creation failed:', manualError)
        }
      }
    } else {
      console.warn('⚠️ No connector found in Echo instance')
    }
  } catch (error: any) {
    console.error('❌ Echo initialization failed:', {
      message: error?.message || 'Unknown error',
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
        
        // Автоматическое переподключение
        if (reconnectAttempts < maxReconnectAttempts) {
          scheduleReconnect()
        }
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
      console.log('🔍 Debug info:', {
        echoInstance: !!echoInstance,
        connector: !!echoInstance?.connector,
        socket: !!echoInstance?.connector?.socket,
        connectorType: typeof echoInstance?.connector,
        connectorKeys: echoInstance?.connector ? Object.keys(echoInstance.connector) : 'no connector'
      })
      startFallbackPolling()
    }
  } catch (error: any) {
    console.warn('⚠️ Pusher Echo initialization failed, trying Socket.IO fallback:', error)
    
    // Попробуем Socket.IO как fallback
    try {
      echoInstance = new Echo({
        broadcaster: 'socket.io',
        host: `${wsHost}:${wsPort}`,
        key,
        auth: {
          headers: {
            Authorization: `Bearer ${localStorage.getItem('auth_token') || ''}`,
          },
        },
      })
      console.log('✅ Socket.IO Echo fallback created successfully')
    } catch (socketIoError: any) {
      console.warn('⚠️ Socket.IO Echo also failed, using fallback polling:', socketIoError)
      startFallbackPolling()
    }
  }

  // Добавляем обработчики событий WebSocket
  if (echoInstance?.connector?.socket) {
    echoInstance.connector.socket.on('connect', () => {
      console.log('✅ WebSocket connected successfully')
      if (fallbackMode) {
        console.log('🔄 Switching from fallback to real-time mode')
        stopFallbackPolling()
        fallbackMode = false
      }
    })
    
    echoInstance.connector.socket.on('disconnect', (reason) => {
      console.warn('⚠️ WebSocket disconnected:', reason)
      if (!fallbackMode) {
        console.log('🔄 Switching to fallback mode')
        startFallbackPolling()
      }
    })
    
    echoInstance.connector.socket.on('error', (error) => {
      console.error('❌ WebSocket error:', error)
    })
  }

  // Проверяем соединение через 2 секунды
  setTimeout(() => {
    if (echoInstance?.connector?.socket) {
      // WebSocket использует readyState вместо connected
      const isConnected = echoInstance.connector.socket.readyState === WebSocket.OPEN
      console.log('🔍 WebSocket connection status after 2s:', {
        connected: isConnected,
        readyState: echoInstance.connector.socket.readyState,
        readyStateText: ['CONNECTING', 'OPEN', 'CLOSING', 'CLOSED'][echoInstance.connector.socket.readyState],
        url: echoInstance.connector.socket.url
      })
      
      if (!isConnected && !fallbackMode) {
        console.warn('⚠️ WebSocket not connected after 2s, starting fallback')
        startFallbackPolling()
      } else if (isConnected && fallbackMode) {
        console.log('🔄 WebSocket is connected, stopping fallback')
        stopFallbackPolling()
        fallbackMode = false
      }
    } else {
      console.warn('⚠️ No socket available after 2s, starting fallback')
      
      // Попробуем подключиться вручную для диагностики
      try {
        const testUrl = `ws://${wsHost}:${wsPort}/app/${key}?protocol=7&client=js&version=8.4.0&flash=false`
        console.log('🔍 Testing WebSocket connection to:', testUrl)
        
        const testSocket = new WebSocket(testUrl)
        testSocket.onopen = () => {
          console.log('✅ Direct WebSocket connection successful')
          testSocket.close()
        }
        testSocket.onerror = (error) => {
          console.error('❌ Direct WebSocket connection failed:', error)
        }
        testSocket.onclose = (event) => {
          console.log('🔍 WebSocket closed:', event.code, event.reason)
        }
      } catch (testError: any) {
        console.error('❌ WebSocket test failed:', testError)
      }
      
      startFallbackPolling()
    }
  }, 2000)

  return echoInstance
}

export function getEcho(): any {
  if (!echoInstance) {
    return initializeEcho()
  }
  return echoInstance
}

export function disconnectEcho(): any {
  if (echoInstance) {
    echoInstance.disconnect()
    echoInstance = null
    console.log('👋 WebSocket disconnected')
  }
  stopFallbackPolling()
  isWebSocketConnected = false
  fallbackMode = false
  
  // Очищаем таймер переподключения
  if (reconnectTimeout) {
    clearTimeout(reconnectTimeout)
    reconnectTimeout = null
  }
  reconnectAttempts = 0
}

/**
 * Планирование переподключения
 */
function scheduleReconnect(): any {
  if (reconnectTimeout) {
    clearTimeout(reconnectTimeout)
  }
  
  reconnectAttempts++
  const delay = Math.min(1000 * Math.pow(2, reconnectAttempts - 1), 30000) // Exponential backoff, max 30s
  
  console.log(`🔄 Scheduling reconnect attempt ${reconnectAttempts}/${maxReconnectAttempts} in ${delay}ms`)
  
  reconnectTimeout = setTimeout(() => {
    reconnectWebSocket()
  }, delay)
}

/**
 * Переподключение WebSocket
 */
function reconnectWebSocket(): any {
  if (isWebSocketConnected) {
    console.log('✅ WebSocket already connected, skipping reconnect')
    return
  }
  
  console.log(`🔄 Attempting to reconnect WebSocket (attempt ${reconnectAttempts}/${maxReconnectAttempts})`)
  
  try {
    // Отключаем текущий экземпляр
    if (echoInstance) {
      echoInstance.disconnect()
      echoInstance = null
    }
    
    // Создаем новый экземпляр
    echoInstance = initializeEcho()
    
    if (echoInstance) {
      console.log('✅ WebSocket reconnected successfully')
      reconnectAttempts = 0
      isWebSocketConnected = true
      fallbackMode = false
      stopFallbackPolling()
    } else {
      console.warn('⚠️ WebSocket reconnect failed, continuing with fallback')
      startFallbackPolling()
    }
  } catch (error: any) {
    console.error('❌ WebSocket reconnect error:', error)
    startFallbackPolling()
  }
}

/**
 * Fallback polling при недоступности WebSocket
 */
function startFallbackPolling(): any {
  if (fallbackMode || fallbackInterval) {
    return
  }
  
  console.log('🔄 Starting fallback polling mode')
  fallbackMode = true
  
  // Polling каждые 3 секунды для мгновенного обновления в dev режиме
  fallbackInterval = setInterval(async () => {
    try {
      // Импортируем API сервис динамически чтобы избежать циклических зависимостей
      const { default: api } = await import('./api')
      
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
    } catch (error: any) {
      console.warn('Fallback polling error:', error)
    }
  }, 3000)
}

function stopFallbackPolling(): any {
  if (fallbackInterval) {
    clearInterval(fallbackInterval)
    fallbackInterval = null
    console.log('✅ Fallback polling stopped')
  }
}

/**
 * Проверка статуса соединения
 */
export function getConnectionStatus(): any {
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



