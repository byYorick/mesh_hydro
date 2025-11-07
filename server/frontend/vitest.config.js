import { defineConfig } from 'vitest/config'
import vue from '@vitejs/plugin-vue'
import vuetify from 'vite-plugin-vuetify'
import { fileURLToPath } from 'node:url'

// Плагин для перехвата CSS импортов до загрузки Node.js
const cssHandler = () => ({
  name: 'css-handler',
  enforce: 'pre',
  resolveId(id) {
    if (id && typeof id === 'string' && (id.endsWith('.css') || id.endsWith('.scss') || id.endsWith('.sass'))) {
      return `\0css:${id}`
    }
    return null
  },
  load(id) {
    if (id && typeof id === 'string' && id.startsWith('\0css:')) {
      return ''
    }
    return null
  }
})

export default defineConfig({
  plugins: [
    cssHandler(), // Должен быть первым
    vue(),
    vuetify({ 
      autoImport: true
    })
  ],
  test: {
    globals: true,
    environment: 'happy-dom',
    setupFiles: ['./src/__tests__/setup.js'],
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json', 'html'],
      exclude: [
        'node_modules/',
        'src/__tests__/',
        '*.config.js',
        'src/main.js'
      ]
    }
  },
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    }
  },
  css: {
    modules: {
      classNameStrategy: 'non-scoped'
    }
  },
  optimizeDeps: {
    exclude: ['vuetify']
  }
})
