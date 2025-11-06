import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vuetify from 'vite-plugin-vuetify'
import { VitePWA } from 'vite-plugin-pwa'
import VueDevtools from 'vite-plugin-vue-devtools'
import Components from 'unplugin-vue-components/vite'
import AutoImport from 'unplugin-auto-import/vite'
import { fileURLToPath, URL } from 'node:url'

export default defineConfig(({ mode }) => ({
  plugins: [
    vue(),
    // Vue Devtools отключен из-за проблем с парсером
    // mode === 'development' && VueDevtools(),
    vuetify({ autoImport: true }),

    // Auto-import components
    Components({
      dts: true,
      dirs: ['src/components'],
      extensions: ['vue'],
      include: [/\.vue$/, /\.vue\?vue/]
    }),

    // Auto-import composables and utilities
    AutoImport({
      dts: true,
      imports: [
        'vue',
        'vue-router',
        'pinia',
        '@vueuse/core',
        {
          'vue': ['defineProps', 'defineEmits', 'defineExpose', 'withDefaults'],
          'vue-router': ['useRouter', 'useRoute'],
          'pinia': ['defineStore', 'storeToRefs']
        }
      ],
      dirs: [
        'src/composables',
        'src/stores',
        'src/utils'
      ],
      vueTemplate: true
    })

    // PWA - DISABLED (no caching)
    // ...(mode === 'development' ? [] : [VitePWA({
    //   registerType: 'autoUpdate',
    //   injectRegister: 'auto',
    //   workbox: {
    //     globPatterns: ['**/*.{js,css,html,ico,png,svg,woff2}'],
    //     runtimeCaching: [
    //       {
    //         urlPattern: /^https:\/\/api\./,
    //         handler: 'NetworkFirst',
    //         options: {
    //           cacheName: 'api-cache',
    //           networkTimeoutSeconds: 10,
    //           cacheableResponse: {
    //             statuses: [0, 200]
    //           }
    //         }
    //       },
    //       {
    //         urlPattern: /\.(?:png|jpg|jpeg|svg|gif|webp)$/,
    //         handler: 'CacheFirst',
    //         options: {
    //           cacheName: 'images-cache',
    //           expiration: {
    //             maxEntries: 100,
    //             maxAgeSeconds: 60 * 60 * 24 * 30 // 30 days
    //           }
    //         }
    //       }
    //     ],
    //     skipWaiting: true,
    //     clientsClaim: true
    //   },
    //   includeAssets: ['favicon.svg', 'apple-touch-icon.png', 'masked-icon.svg'],
    //   manifest: {
    //     name: 'Mesh Hydro System',
    //     short_name: 'Hydro System',
    //     description: 'Mesh Hydro System Dashboard',
    //     theme_color: '#1976d2',
    //     background_color: '#ffffff',
    //     display: 'standalone',
    //     orientation: 'portrait',
    //     scope: '/',
    //     start_url: '/',
    //     icons: [
    //       {
    //         src: 'pwa-192x192.png',
    //         sizes: '192x192',
    //         type: 'image/png'
    //       },
    //       {
    //         src: 'pwa-512x512.png',
    //         sizes: '512x512',
    //         type: 'image/png'
    //       }
    //     ]
    //   }
    // })]).filter(Boolean)
  ],

  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
      '@/components': fileURLToPath(new URL('./src/components', import.meta.url)),
      '@/stores': fileURLToPath(new URL('./src/stores', import.meta.url)),
      '@/utils': fileURLToPath(new URL('./src/utils', import.meta.url)),
      '@/types': fileURLToPath(new URL('./src/types', import.meta.url)),
      '@/services': fileURLToPath(new URL('./src/services', import.meta.url)),
      '@/composables': fileURLToPath(new URL('./src/composables', import.meta.url)),
      '@/views': fileURLToPath(new URL('./src/views', import.meta.url)),
      '@/assets': fileURLToPath(new URL('./src/assets', import.meta.url))
    }
  },

  server: {
    host: '0.0.0.0',
    port: 5173,
    strictPort: true,
    hmr: {
      host: 'localhost',
      port: 5173,
      clientPort: 5173
    },
    watch: {
      usePolling: true, // Для лучшей работы на Windows
      interval: 1000,
      ignored: ['**/node_modules/**', '**/.git/**']
    },
    headers: {
      'Cache-Control': 'no-store, no-cache, must-revalidate, proxy-revalidate, max-age=0',
      'Pragma': 'no-cache',
      'Expires': '0',
      'X-Accel-Buffering': 'no'
    },
    proxy: {
      '/api': {
        target: 'http://backend:8000',
        changeOrigin: true,
        secure: false,
        ws: false,
        configure: (proxy, options) => {
          proxy.on('error', (err, req, res) => {
            console.error('Proxy error:', err);
          });
          proxy.on('proxyReq', (proxyReq, req, res) => {
            console.log('Proxying request:', req.method, req.url);
          });
        },
        headers: {
          'Cache-Control': 'no-cache, no-store, must-revalidate',
          'Pragma': 'no-cache',
          'Expires': '0'
        }
      },
      '/app': {
        target: 'http://reverb:8080',
        ws: true,
        changeOrigin: true
      }
    }
  },

  build: {
    target: 'esnext',
    minify: mode !== 'development', // Отключить минификацию для development
    sourcemap: true, // Включить source maps для отладки
    rollupOptions: {
      output: {
        manualChunks: {
          'vendor-vue': ['vue', 'vue-router', 'pinia'],
          'vendor-ui': ['vuetify'],
          'vendor-charts': ['chart.js', 'vue-chartjs', 'chartjs-adapter-date-fns', 'chartjs-plugin-zoom'],
          'vendor-utils': ['axios', 'date-fns', '@vueuse/core'],
          'vendor-realtime': ['laravel-echo', 'pusher-js', 'socket.io-client']
        },
        // Добавляем timestamp для предотвращения кеширования в development
        ...(mode === 'development' ? {
          entryFileNames: `assets/[name]-[hash].js`,
          chunkFileNames: `assets/[name]-[hash].js`,
          assetFileNames: `assets/[name]-[hash].[ext]`
        } : {
          entryFileNames: 'assets/[name].js',
          chunkFileNames: 'assets/[name].js',
          assetFileNames: 'assets/[name].[ext]'
        })
      }
    },
    chunkSizeWarningLimit: 1000,
    reportCompressedSize: true,
    cssCodeSplit: true
  },

  define: {
    __VUE_OPTIONS_API__: true,
    __VUE_PROD_DEVTOOLS__: true  // Разрешить Devtools в production
  },

  optimizeDeps: {
    include: [
      'vue',
      'vue-router',
      'pinia',
      'vuetify',
      'axios',
      'chart.js',
      'vue-chartjs',
      'date-fns',
      '@vueuse/core'
    ],
    force: mode === 'development', // Принудительно пересобирать зависимости в development
    esbuildOptions: {
      // Отключить кеширование для development
      ...(mode === 'development' && {
        logOverride: { 'this-is-undefined-in-esm': 'silent' }
      })
    }
  },

  // Отключить кеширование для development - используем временную директорию
  ...(mode === 'development' && {
    cacheDir: '.vite-dev-cache' // Временная директория для кеша в development
  })
}))
