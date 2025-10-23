import { createVuetify } from 'vuetify'
import * as components from 'vuetify/components'
import * as directives from 'vuetify/directives'
import { aliases, mdi } from 'vuetify/iconsets/mdi'
import '@mdi/font/css/materialdesignicons.css'
import 'vuetify/styles'

export default createVuetify({
  components,
  directives,
  icons: {
    defaultSet: 'mdi',
    aliases,
    sets: {
      mdi,
    },
  },
  theme: {
    defaultTheme: 'light',
    themes: {
      light: {
        dark: false,
        colors: {
          primary: '#2196F3',      // Modern blue
          secondary: '#00BCD4',    // Cyan
          accent: '#FF4081',       // Pink accent
          error: '#F44336',        // Red
          info: '#9C27B0',         // Purple
          success: '#4CAF50',      // Green
          warning: '#FF9800',      // Orange
          background: '#F5F7FA',   // Light gray background
          surface: '#FFFFFF',      // White surface
          'on-surface': '#1A1A1A', // Dark text
        },
      },
      dark: {
        dark: true,
        colors: {
          primary: '#42A5F5',      // Lighter blue for dark
          secondary: '#26C6DA',    // Lighter cyan
          accent: '#FF4081',       // Same pink
          error: '#EF5350',        // Lighter red
          info: '#AB47BC',         // Lighter purple
          success: '#66BB6A',      // Lighter green
          warning: '#FFA726',      // Lighter orange
          background: '#0A0E27',   // Deep dark blue
          surface: '#1A1F3A',      // Dark blue surface
          'surface-variant': '#2A2F4A', // Lighter surface
          'on-surface': '#E0E0E0', // Light text
        },
      },
    },
  },
  defaults: {
    VCard: {
      elevation: 2,
      rounded: 'lg',
    },
    VBtn: {
      rounded: 'lg',
      elevation: 0,
    },
    VSheet: {
      rounded: 'lg',
    },
    VTextField: {
      variant: 'outlined',
      density: 'comfortable',
      color: 'primary',
    },
    VSelect: {
      variant: 'outlined',
      density: 'comfortable',
      color: 'primary',
    },
  },
})

