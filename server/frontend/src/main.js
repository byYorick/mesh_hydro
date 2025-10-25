import { createApp } from 'vue'
import { createPinia } from 'pinia'
import router from './router'
import vuetify from './plugins/vuetify'
import App from './App.vue'
import { initializeEcho } from './services/echo'

// Styles
import './assets/main.css'


// Global error handler for includes() errors
window.addEventListener('error', (event) => {
  console.log('🔍 GLOBAL ERROR CAUGHT:', event.message)
  console.log('🔍 Error type:', typeof event.message)
  console.log('🔍 Event object:', event)
  
  // Проверяем на ошибку includes
  if (event && event.message && typeof event.message === 'string' && event.message.includes("Cannot read properties of undefined (reading 'includes')")) {
    console.error('🚨 GLOBAL ERROR: includes() called on undefined')
    console.error('Error details:', event.error)
    console.error('Stack trace:', event.error?.stack)
    console.error('Event filename:', event.filename)
    console.error('Event lineno:', event.lineno)
    console.error('Event colno:', event.colno)
    console.error('Event target:', event.target)
    console.error('Event currentTarget:', event.currentTarget)
  }
})

// Дополнительный обработчик для unhandled promise rejections
window.addEventListener('unhandledrejection', (event) => {
  console.log('🔍 UNHANDLED PROMISE REJECTION:', event.reason)
  if (event.reason && event.reason.message && typeof event.reason.message === 'string' && event.reason.message.includes("Cannot read properties of undefined (reading 'includes')")) {
    console.error('🚨 PROMISE REJECTION: includes() called on undefined')
    console.error('Promise rejection details:', event.reason)
  }
})

// Override Array.prototype.includes to catch undefined calls
const originalIncludes = Array.prototype.includes
Array.prototype.includes = function(searchElement, fromIndex) {
  console.log('🔍 ARRAY INCLUDES CALLED:', {
    this: this,
    thisType: typeof this,
    isArray: Array.isArray(this),
    searchElement: searchElement,
    fromIndex: fromIndex,
    stack: new Error().stack
  })
  
  if (this === undefined || this === null) {
    console.error('🚨 ARRAY INCLUDES ERROR: Array is undefined/null')
    console.error('searchElement:', searchElement)
    console.error('fromIndex:', fromIndex)
    console.error('this:', this)
    console.error('Stack trace:', new Error().stack)
    return false
  }
  
  if (!Array.isArray(this)) {
    console.error('🚨 ARRAY INCLUDES ERROR: this is not an array:', this, typeof this)
    console.error('Stack trace:', new Error().stack)
    return false
  }
  
  try {
    const result = originalIncludes.call(this, searchElement, fromIndex)
    console.log('🔍 ARRAY INCLUDES RESULT:', result)
    return result
  } catch (error) {
    console.error('🚨 ARRAY INCLUDES EXCEPTION:', error)
    console.error('Stack trace:', error.stack)
    return false
  }
}

const app = createApp(App)

// Initialize Pinia
const pinia = createPinia()
app.use(pinia)

// Initialize Echo (WebSocket) with error handling
let echo = null
try {
  echo = initializeEcho()
  console.log('✅ Echo initialized')
} catch (error) {
  console.warn('⚠️ Echo initialization failed, continuing without WebSocket:', error)
}
app.provide('echo', echo)

app.use(router)
app.use(vuetify)

app.mount('#app')

