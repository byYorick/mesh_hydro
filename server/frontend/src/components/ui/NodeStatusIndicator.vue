<template>
  <div class="node-status-indicator">
    <!-- Основной индикатор -->
    <v-chip
      :color="statusColor"
      :prepend-icon="statusIcon"
      size="small"
      class="mr-2"
    >
      {{ statusText }}
    </v-chip>

    <!-- Индикатор качества связи -->
    <v-tooltip location="top">
      <template v-slot:activator="{ props }">
        <v-icon
          v-bind="props"
          :icon="connectionQualityIcon"
          :color="connectionQualityColor"
          size="small"
          class="mr-1"
        ></v-icon>
      </template>
      <span>{{ qualityText }}</span>
    </v-tooltip>

    <!-- Время последнего контакта -->
    <v-tooltip location="top">
      <template v-slot:activator="{ props }">
        <span v-bind="props" class="text-caption text-disabled">
          {{ lastSeenText }}
        </span>
      </template>
      <div class="text-left">
        <div><strong>Время последнего контакта:</strong></div>
        <div>{{ node?.last_seen_at ? new Date(node.last_seen_at).toLocaleString('ru-RU') : 'Никогда' }}</div>
        <div v-if="secondsSinceLastSeen > 0" class="mt-1">
          <strong>Прошло:</strong> {{ secondsSinceLastSeen }} сек
        </div>
        <div v-if="timeUntilOffline > 0" class="mt-1">
          <strong>До офлайна:</strong> {{ timeUntilOffline }} сек
        </div>
      </div>
    </v-tooltip>

    <!-- Прогресс-бар таймаута (если онлайн) -->
    <v-progress-linear
      v-if="isOnline"
      :model-value="timeUntilOfflinePercent"
      :color="timeUntilOfflineColor"
      height="4"
      class="mt-1"
      rounded
    ></v-progress-linear>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { useNodeStatus } from '@/composables/useNodeStatus'
import { ref } from 'vue'

const props = defineProps({
  node: {
    type: Object,
    required: true
  }
})

// Используем composable для статуса
const nodeRef = ref(props.node)
const status = useNodeStatus(nodeRef)

// Деструктуризация статуса
const {
  isOnline,
  statusColor,
  statusIcon,
  statusText,
  lastSeenText,
  connectionQuality,
  connectionQualityIcon,
  connectionQualityColor,
  secondsSinceLastSeen,
  timeUntilOffline
} = status

// Текст для качества связи
const qualityText = computed(() => {
  const quality = connectionQuality.value
  const seconds = secondsSinceLastSeen.value
  
  switch (quality) {
    case 'excellent':
      return `Отличное соединение (${seconds} сек)`
    case 'good':
      return `Хорошее соединение (${seconds} сек)`
    case 'poor':
      return `Слабое соединение (${seconds} сек)`
    case 'offline':
      return 'Узел офлайн'
  }
})

// Процент времени до офлайна (для прогресс-бара)
const timeUntilOfflinePercent = computed(() => {
  const timeout = 20 // 20 секунд таймаут
  return (timeUntilOffline.value / timeout) * 100
})

// Цвет прогресс-бара
const timeUntilOfflineColor = computed(() => {
  const percent = timeUntilOfflinePercent.value
  if (percent > 75) return 'success'
  if (percent > 50) return 'info'
  if (percent > 25) return 'warning'
  return 'error'
})
</script>

<style scoped>
.node-status-indicator {
  display: flex;
  flex-direction: column;
  gap: 4px;
}
</style>
