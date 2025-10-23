<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-timeline-clock" class="mr-2"></v-icon>
      История ошибок
      
      <v-spacer></v-spacer>
      
      <v-chip size="small" color="error" v-if="unresolvedCount > 0">
        {{ unresolvedCount }} активных
      </v-chip>
    </v-card-title>

    <v-card-text>
      <v-timeline
        v-if="errors.length > 0"
        side="end"
        density="compact"
        :truncate-line="isMobile ? 'start' : 'both'"
      >
        <v-timeline-item
          v-for="error in errors"
          :key="error.id"
          :dot-color="getSeverityColor(error.severity)"
          :icon="getTypeIcon(error.error_type)"
          size="small"
        >
          <template v-slot:opposite>
            <div class="text-caption text-medium-emphasis">
              {{ formatTime(error.occurred_at) }}
            </div>
          </template>

          <v-card :color="error.resolved_at ? 'surface' : getSeverityColor(error.severity)" variant="tonal">
            <v-card-text class="py-3">
              <div class="d-flex align-center mb-2">
                <v-chip
                  :color="getSeverityColor(error.severity)"
                  size="x-small"
                  class="mr-2"
                >
                  {{ error.severity }}
                </v-chip>
                
                <v-chip
                  size="x-small"
                  variant="outlined"
                >
                  {{ error.error_code }}
                </v-chip>
                
                <v-spacer></v-spacer>
                
                <v-chip
                  v-if="error.resolved_at"
                  color="success"
                  size="x-small"
                  prepend-icon="mdi-check"
                >
                  Решено
                </v-chip>
              </div>

              <div class="text-body-2 mb-2">
                {{ error.message }}
              </div>

              <!-- Diagnostics preview -->
              <div v-if="error.diagnostics" class="text-caption text-medium-emphasis">
                <v-chip-group>
                  <v-chip
                    v-if="error.diagnostics.heap_free"
                    size="x-small"
                    variant="outlined"
                  >
                    <v-icon icon="mdi-memory" start size="x-small"></v-icon>
                    {{ formatBytes(error.diagnostics.heap_free) }}
                  </v-chip>
                  
                  <v-chip
                    v-if="error.diagnostics.uptime"
                    size="x-small"
                    variant="outlined"
                  >
                    <v-icon icon="mdi-clock" start size="x-small"></v-icon>
                    {{ formatUptime(error.diagnostics.uptime) }}
                  </v-chip>
                  
                  <v-chip
                    v-if="error.diagnostics.retry_count"
                    size="x-small"
                    variant="outlined"
                  >
                    <v-icon icon="mdi-refresh" start size="x-small"></v-icon>
                    {{ error.diagnostics.retry_count }} попыток
                  </v-chip>
                </v-chip-group>
              </div>

              <!-- Actions -->
              <div class="mt-2">
                <v-btn
                  size="small"
                  variant="text"
                  prepend-icon="mdi-eye"
                  @click="$emit('view-details', error)"
                >
                  Детали
                </v-btn>
                
                <v-btn
                  v-if="!error.resolved_at"
                  size="small"
                  variant="text"
                  color="success"
                  prepend-icon="mdi-check"
                  @click="$emit('resolve', error.id)"
                >
                  Решить
                </v-btn>
              </div>
            </v-card-text>
          </v-card>
        </v-timeline-item>
      </v-timeline>

      <!-- Empty state -->
      <div v-else class="text-center pa-8">
        <v-icon icon="mdi-check-circle" size="64" color="success"></v-icon>
        <div class="mt-4 text-h6">Ошибок не обнаружено</div>
        <div class="text-caption text-medium-emphasis">
          Узел работает без проблем
        </div>
      </div>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'
import { useResponsive } from '@/composables/useResponsive'
import { formatDateTime } from '@/utils/time'

const { isMobile } = useResponsive()

const props = defineProps({
  errors: {
    type: Array,
    default: () => [],
  },
})

defineEmits(['view-details', 'resolve'])

const unresolvedCount = computed(() => {
  if (!Array.isArray(props.errors)) {
    return 0
  }
  return props.errors.filter(e => !e.resolved_at).length
})

function getSeverityColor(severity) {
  const colors = {
    low: 'info',
    medium: 'warning',
    high: 'orange',
    critical: 'error',
  }
  return colors[severity] || 'grey'
}

function getTypeIcon(type) {
  const icons = {
    hardware: 'mdi-chip',
    software: 'mdi-bug',
    network: 'mdi-wifi-alert',
    sensor: 'mdi-thermometer-alert',
  }
  return icons[type] || 'mdi-alert-circle'
}

function formatTime(timestamp) {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now - date
  
  if (diff < 3600000) { // < 1 hour
    return `${Math.floor(diff / 60000)}м назад`
  } else if (diff < 86400000) { // < 1 day
    return `${Math.floor(diff / 3600000)}ч назад`
  } else {
    return formatDateTime(timestamp)
  }
}

function formatBytes(bytes) {
  if (bytes < 1024) return `${bytes}B`
  if (bytes < 1048576) return `${(bytes / 1024).toFixed(1)}KB`
  return `${(bytes / 1048576).toFixed(1)}MB`
}

function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400)
  const hours = Math.floor((seconds % 86400) / 3600)
  
  if (days > 0) return `${days}д ${hours}ч`
  if (hours > 0) return `${hours}ч`
  return `${Math.floor(seconds / 60)}м`
}
</script>

