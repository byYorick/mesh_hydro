<template>
  <v-card
    class="new-node-card"
    variant="tonal"
    :color="cardColor"
  >
    <v-card-title class="d-flex justify-space-between align-center">
      <div class="d-flex flex-column">
        <span class="text-h6 d-flex align-center">
          <v-icon
            :icon="nodeIcon"
            class="mr-2"
            :color="node.is_root ? 'warning' : 'info'"
          ></v-icon>
          {{ nodeLabel }}
        </span>
        <span class="text-caption text-medium-emphasis">
          MAC: {{ node.mac_address }}
        </span>
      </div>

      <div class="d-flex align-center ga-2">
        <v-chip
          size="small"
          :color="statusColor"
          variant="flat"
          prepend-icon="mdi-radar"
        >
          {{ statusLabel }}
        </v-chip>
        <v-chip
          size="small"
          color="primary"
          variant="tonal"
          prepend-icon="mdi-shield-key"
        >
          PIN {{ node.pin }}
        </v-chip>
      </div>
    </v-card-title>

    <v-card-text>
      <v-row dense>
        <v-col cols="12" md="6">
          <div class="text-caption text-medium-emphasis mb-1">Mesh ID</div>
          <div class="text-body-2 font-weight-medium">
            {{ node.metadata?.mesh_network_id || node.temp_mesh_id || '—' }}
          </div>
        </v-col>
        <v-col cols="12" md="6">
          <div class="text-caption text-medium-emphasis mb-1">Обнаружена</div>
          <div class="text-body-2 font-weight-medium">
            {{ discoveredAgo }}
          </div>
        </v-col>

        <v-col cols="12" md="6">
          <div class="text-caption text-medium-emphasis mb-1">
            IP адрес
          </div>
          <div class="text-body-2 font-weight-medium">
            {{ node.metadata?.ip_address || '—' }}
          </div>
        </v-col>

        <v-col cols="12" md="6" v-if="node.metadata?.firmware_version">
          <div class="text-caption text-medium-emphasis mb-1">
            Прошивка
          </div>
          <div class="text-body-2 font-weight-medium">
            {{ node.metadata?.firmware_version }}
          </div>
        </v-col>

        <v-col cols="12">
          <v-alert
            v-if="node.status === 'configuring'"
            type="info"
            variant="tonal"
            density="comfortable"
            class="mt-2"
            border="start"
          >
            Конфигурация отправлена. Ожидание подтверждения от устройства...
          </v-alert>

          <v-alert
            v-else-if="!node.is_online"
            type="warning"
            variant="tonal"
            density="comfortable"
            class="mt-2"
            border="start"
          >
            Устройство офлайн. Проверьте питание и Wi-Fi.
          </v-alert>
        </v-col>
      </v-row>
    </v-card-text>

    <v-card-actions class="justify-space-between">
      <div class="text-caption text-medium-emphasis">
        Последний сигнал: {{ lastHeartbeatAgo }}
      </div>

      <div class="d-flex align-center ga-2">
        <v-btn
          color="primary"
          prepend-icon="mdi-cog-play"
          :loading="loading"
          :disabled="disabled || node.status === 'configuring'"
          @click="$emit('configure', node)"
        >
          Настроить
        </v-btn>

        <v-btn
          color="error"
          variant="text"
          prepend-icon="mdi-delete"
          :disabled="loading || disabled"
          @click="$emit('remove', node)"
        >
          Удалить
        </v-btn>
      </div>
    </v-card-actions>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { NewNode } from '@/types/newNode'
import { formatDistanceToNow } from '@/utils/time'

const props = defineProps<{
  node: NewNode
  loading?: boolean
  disabled?: boolean
}>()

defineEmits<{
  (event: 'configure', node: NewNode): void
  (event: 'remove', node: NewNode): void
}>()

const nodeLabel = computed(() => {
  if (!props.node) return 'Узел'
  if (props.node.is_root) return 'ROOT узел'

  const labels: Record<string, string> = {
    climate: 'Климатическая нода',
    ph: 'pH сенсор',
    ec: 'EC сенсор',
    ph_ec: 'pH/EC сенсор',
    relay: 'Релейный блок',
    water: 'Водный модуль',
    display: 'Дисплей',
  }

  return labels[props.node.node_type] || props.node.node_type || 'Неизвестный узел'
})

const nodeIcon = computed(() => {
  if (props.node.is_root) return 'mdi-router-wireless'
  const icons: Record<string, string> = {
    climate: 'mdi-weather-partly-cloudy',
    ph: 'mdi-beaker',
    ec: 'mdi-chemical-weapon',
    ph_ec: 'mdi-water-percent',
    relay: 'mdi-power-socket-eu',
    water: 'mdi-water-pump',
    display: 'mdi-monitor-dashboard',
  }
  return icons[props.node.node_type] || 'mdi-chip'
})

const statusLabel = computed(() => {
  if (props.node.status === 'configuring') return 'Отправлена конфигурация'
  if (!props.node.is_online) return 'Офлайн'
  return 'Готов к настройке'
})

const statusColor = computed(() => {
  if (props.node.status === 'configuring') return 'info'
  if (!props.node.is_online) return 'warning'
  return 'success'
})

const cardColor = computed(() => {
  if (props.node.is_root) return 'rgba(56, 189, 248, 0.08)'
  return 'rgba(148, 163, 184, 0.05)'
})

const discoveredAgo = computed(() => {
  return props.node.discovered_at
    ? formatDistanceToNow(props.node.discovered_at)
    : '—'
})

const lastHeartbeatAgo = computed(() => {
  if (!props.node.last_heartbeat_at) return 'нет данных'
  return formatDistanceToNow(props.node.last_heartbeat_at)
})
</script>

<style scoped>
.new-node-card {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.new-node-card .v-card-text {
  flex: 1;
}
</style>


