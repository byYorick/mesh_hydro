import { createApp } from 'vue'
import { createPinia } from 'pinia'
import router from './router'
import vuetify from './plugins/vuetify'
import App from './App.vue'
import { initializeEcho } from './services/echo'

// Styles
import './assets/main.css'

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

