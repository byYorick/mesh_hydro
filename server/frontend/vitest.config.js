import { defineConfig } from 'vitest/config'
import vue from '@vitejs/plugin-vue'
import vuetify from 'vite-plugin-vuetify'
import { fileURLToPath } from 'node:url'

// Простая заглушка для CSS/SCSS импортов в Vitest окружении
const CSS_RE = /\.(css|scss|sass)(?:\?.*)?$/

const cssHandler = () => ({
  name: 'css-handler',
  enforce: 'pre',
  load(id) {
    if (typeof id === 'string' && CSS_RE.test(id)) {
      return 'export default {}'
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
    deps: {
      inline: ['vuetify']
    },
    coverage: {
      provider: 'istanbul',
      reporter: ['text', 'json', 'html'],
      include: ['src/**/*.{ts,tsx,js,jsx,vue}'],
      exclude: [
        '**/node_modules/**',
        'src/__tests__/**',
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
