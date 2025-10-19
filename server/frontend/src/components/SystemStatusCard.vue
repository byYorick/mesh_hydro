<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-monitor-dashboard" class="mr-2"></v-icon>
      Статус системы
      
      <v-spacer></v-spacer>
      
      <v-chip
        :color="overallStatus"
        variant="flat"
      >
        <v-icon :icon="overallStatusIcon" start></v-icon>
        {{ overallStatusText }}
      </v-chip>
    </v-card-title>

    <v-card-text>
      <v-row>
        <!-- Database Status -->
        <v-col cols="12" sm="6" md="3">
          <v-card variant="tonal" :color="dbColor">
            <v-card-text class="text-center">
              <v-icon :icon="dbIcon" size="48"></v-icon>
              <div class="text-h6 mt-2">База данных</div>
              <div class="text-caption">{{ status?.database || 'N/A' }}</div>
            </v-card-text>
          </v-card>
        </v-col>

        <!-- MQTT Status -->
        <v-col cols="12" sm="6" md="3">
          <v-card variant="tonal" :color="mqttColor">
            <v-card-text class="text-center">
              <v-icon :icon="mqttIcon" size="48"></v-icon>
              <div class="text-h6 mt-2">MQTT</div>
              <div class="text-caption">{{ status?.mqtt || 'N/A' }}</div>
            </v-card-text>
          </v-card>
        </v-col>

        <!-- Telegram Status -->
        <v-col cols="12" sm="6" md="3">
          <v-card variant="tonal" :color="telegramColor">
            <v-card-text class="text-center">
              <v-icon :icon="telegramIcon" size="48"></v-icon>
              <div class="text-h6 mt-2">Telegram</div>
              <div class="text-caption">{{ status?.telegram || 'disabled' }}</div>
            </v-card-text>
          </v-card>
        </v-col>

        <!-- Overall Status -->
        <v-col cols="12" sm="6" md="3">
          <v-card variant="tonal" :color="overallStatus">
            <v-card-text class="text-center">
              <v-icon :icon="overallStatusIcon" size="48"></v-icon>
              <div class="text-h6 mt-2">Система</div>
              <div class="text-caption">{{ overallStatusText }}</div>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>

      <!-- System Info -->
      <v-divider class="my-4"></v-divider>

      <v-row v-if="status?.system" dense>
        <v-col cols="12" sm="6" md="4">
          <v-list-item density="compact">
            <v-list-item-title>
              <v-icon icon="mdi-language-php" size="small" class="mr-1"></v-icon>
              PHP {{ status.system.php_version }}
            </v-list-item-title>
          </v-list-item>
        </v-col>
        <v-col cols="12" sm="6" md="4">
          <v-list-item density="compact">
            <v-list-item-title>
              <v-icon icon="mdi-laravel" size="small" class="mr-1"></v-icon>
              Laravel {{ status.system.laravel_version }}
            </v-list-item-title>
          </v-list-item>
        </v-col>
        <v-col cols="12" sm="6" md="4">
          <v-list-item density="compact">
            <v-list-item-title>
              <v-icon icon="mdi-clock" size="small" class="mr-1"></v-icon>
              {{ status.system.server_time }}
            </v-list-item-title>
          </v-list-item>
        </v-col>
      </v-row>
    </v-card-text>

    <v-card-actions>
      <v-btn
        prepend-icon="mdi-refresh"
        @click="$emit('refresh')"
        :loading="loading"
      >
        Обновить
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  status: Object,
  loading: Boolean,
})

defineEmits(['refresh'])

const dbColor = computed(() => {
  return props.status?.database === 'ok' ? 'success' : 'error'
})

const dbIcon = computed(() => {
  return props.status?.database === 'ok' ? 'mdi-database-check' : 'mdi-database-alert'
})

const mqttColor = computed(() => {
  return props.status?.mqtt === 'connected' ? 'success' : 'error'
})

const mqttIcon = computed(() => {
  return props.status?.mqtt === 'connected' ? 'mdi-access-point' : 'mdi-access-point-off'
})

const telegramColor = computed(() => {
  if (props.status?.telegram === 'ok') return 'success'
  if (props.status?.telegram === 'disabled') return 'grey'
  return 'warning'
})

const telegramIcon = computed(() => {
  return props.status?.telegram === 'ok' ? 'mdi-send-check' : 'mdi-send-outline'
})

const overallStatus = computed(() => {
  if (!props.status) return 'grey'
  
  const db = props.status.database === 'ok'
  const mqtt = props.status.mqtt === 'connected'
  
  if (db && mqtt) return 'success'
  if (db || mqtt) return 'warning'
  return 'error'
})

const overallStatusIcon = computed(() => {
  switch(overallStatus.value) {
    case 'success': return 'mdi-check-circle'
    case 'warning': return 'mdi-alert-circle'
    case 'error': return 'mdi-close-circle'
    default: return 'mdi-help-circle'
  }
})

const overallStatusText = computed(() => {
  switch(overallStatus.value) {
    case 'success': return 'Все работает'
    case 'warning': return 'Частичный сбой'
    case 'error': return 'Ошибка'
    default: return 'Неизвестно'
  }
})
</script>

