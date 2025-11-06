<template>
  <Transition name="toast">
    <div
      v-if="toast.show"
      :style="getToastStyle(index)"
      class="popup-toast"
    >
      <div class="toast-card" :class="[`toast-${toast.level}`, toast.grouped && 'toast-grouped']">
        <!-- Левая граница для визуального акцента -->
        <div class="toast-border"></div>
        
        <!-- Основной контент -->
        <div class="toast-content-wrapper d-flex align-start">
          <!-- Иконка -->
          <div class="toast-icon-wrapper">
            <v-icon
              :icon="getIcon(toast.level)"
              size="large"
              class="toast-icon"
            ></v-icon>
          </div>
          
          <!-- Контент -->
          <div class="flex-grow-1 toast-content">
            <div class="text-body-1 font-weight-medium toast-message">
              {{ toast.message }}
            </div>
            <div v-if="toast.nodeId" class="text-caption toast-node mt-1">
              <v-icon size="x-small" class="mr-1">mdi-chip</v-icon>
              {{ toast.nodeId }}
            </div>
            <div v-if="hasDetails(toast.data)" class="mt-2">
              <v-chip-group v-if="getEventDetails(toast.data).length > 0" column class="event-details-chips">
                <v-chip
                  v-for="(detail, detailIndex) in getEventDetails(toast.data)"
                  :key="detailIndex"
                  size="x-small"
                  variant="outlined"
                  class="mr-1 mb-1 detail-chip"
                >
                  {{ detail.label }}: {{ detail.value }}
                </v-chip>
              </v-chip-group>
            </div>
          </div>
          
          <!-- Кнопка закрытия -->
          <v-btn
            icon="mdi-close"
            variant="text"
            size="small"
            class="toast-close-btn"
            @click="$emit('remove', toast.id)"
          ></v-btn>
        </div>
        
        <!-- Индикатор времени -->
        <div class="toast-timer" :style="{ animationDuration: `${toast.timeout}ms` }"></div>
      </div>
    </div>
  </Transition>
</template>

<script setup lang="ts">
import { watch } from 'vue'
import type { ToastItem } from '@/composables/usePopup'

const props = defineProps<{
  toast: ToastItem
  index: number
}>()

// Отладка
watch(() => props.toast, (toast) => {
  console.log('🔔 Toast changed:', { 
    id: toast.id, 
    show: toast.show, 
    message: toast.message,
    level: toast.level 
  })
}, { immediate: true, deep: true })

defineEmits<{
  remove: [id: string]
}>()

const getIcon = (level: string): string => {
  const icons: Record<string, string> = {
    emergency: 'mdi-alert-octagon',
    critical: 'mdi-alert-circle',
    error: 'mdi-close-circle',
    warning: 'mdi-alert',
    info: 'mdi-information',
    success: 'mdi-check-circle'
  }
  return icons[level] || 'mdi-information'
}

const hasDetails = (data: any): boolean => {
  if (!data) return false
  return !!(
    data.ph !== undefined ||
    data.current_ph !== undefined ||
    data.target !== undefined ||
    data.ph_target !== undefined ||
    data.pid_up !== undefined ||
    data.pid_down !== undefined ||
    data.pid_data !== undefined ||
    data.zone !== undefined ||
    data.error !== undefined ||
    data.emergency_mode !== undefined ||
    data.dose_ml !== undefined ||
    data.duration_ms !== undefined
  )
}

const getEventDetails = (data: any): Array<{ label: string; value: string }> => {
  if (!data) return []
  
  const details: Array<{ label: string; value: string }> = []
  
  if (data.ph !== undefined && data.ph !== null) {
    details.push({
      label: 'pH',
      value: Number(data.ph).toFixed(2)
    })
  }
  
  if (data.target !== undefined && data.target !== null) {
    details.push({
      label: 'Цель',
      value: Number(data.target).toFixed(2)
    })
  }
  
  if ((data.current_ph !== undefined && data.current_ph !== null) && data.ph === undefined) {
    details.push({
      label: 'pH',
      value: Number(data.current_ph).toFixed(2)
    })
  }
  
  if ((data.ph_target !== undefined && data.ph_target !== null) && data.target === undefined) {
    details.push({
      label: 'Цель',
      value: Number(data.ph_target).toFixed(2)
    })
  }
  
  if (data.dose_ml !== undefined && data.dose_ml !== null) {
    details.push({
      label: 'Доза',
      value: `${Number(data.dose_ml).toFixed(1)} мл`
    })
  }
  
  if (data.duration_ms !== undefined && data.duration_ms !== null) {
    details.push({
      label: 'Длительность',
      value: `${(Number(data.duration_ms) / 1000).toFixed(1)}с`
    })
  }
  
  if (data.pump_name) {
    details.push({
      label: 'Насос',
      value: data.pump_name
    })
  }
  
  if (data.pid_up || data.pid_down) {
    const pidData = data.pid_up || data.pid_down
    if (pidData && pidData.error !== undefined && pidData.error !== null) {
      details.push({
        label: 'Ошибка PID',
        value: pidData.error.toFixed(3)
      })
    }
  }
  
  if (data.zone) {
    details.push({
      label: 'Зона',
      value: data.zone
    })
  }
  
  if (data.emergency_mode) {
    details.push({
      label: 'Режим',
      value: 'Аварийный'
    })
  }
  
  return details
}

const getToastStyle = (index: number) => {
  // Расчет смещения для вертикального стека (снизу вверх)
  const offset = index * 110 // ~110px между уведомлениями
  const zIndex = 10000 + index
  
  // Минимальный отступ от низа экрана
  const minBottom = 24
  
  // Проверяем высоту экрана, чтобы попап не выходил за границы
  const maxBottom = typeof window !== 'undefined' ? window.innerHeight - 100 : 1000
  const calculatedBottom = minBottom + offset
  
  const style = {
    bottom: `${Math.min(calculatedBottom, maxBottom)}px`,
    right: '24px',
    position: 'fixed' as const,
    zIndex: zIndex.toString(),
    maxHeight: 'calc(100vh - 48px)',
    overflowY: 'auto' as const,
    display: 'block' as const,
    visibility: 'visible' as const,
    pointerEvents: 'auto' as const
  }
  
  console.log('🔔 Toast style:', { index, style, toast: props.toast })
  
  return style
}
</script>

<style scoped>
/* Основная карточка тоста */
.toast-card {
  position: relative;
  width: 100%;
  min-width: 400px;
  max-width: 600px;
  border-radius: 16px;
  overflow: hidden;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.12);
  animation: toastEnter 0.4s cubic-bezier(0.34, 1.56, 0.64, 1);
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  background: rgba(255, 255, 255, 0.95);
}

.v-theme--dark .toast-card {
  background: rgba(26, 31, 58, 0.95);
}

/* Градиентный overlay для цветного фона */
.toast-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  pointer-events: none;
  border-radius: 16px;
  opacity: 0;
  transition: opacity 0.3s ease;
}

.toast-success::before {
  background: linear-gradient(135deg, rgba(34, 197, 94, 0.1) 0%, rgba(74, 222, 128, 0.05) 100%);
  opacity: 1;
}

.toast-info::before {
  background: linear-gradient(135deg, rgba(59, 130, 246, 0.1) 0%, rgba(96, 165, 250, 0.05) 100%);
  opacity: 1;
}

.toast-warning::before {
  background: linear-gradient(135deg, rgba(245, 158, 11, 0.1) 0%, rgba(251, 191, 36, 0.05) 100%);
  opacity: 1;
}

.toast-error::before {
  background: linear-gradient(135deg, rgba(239, 68, 68, 0.1) 0%, rgba(248, 113, 113, 0.05) 100%);
  opacity: 1;
}

.toast-critical::before {
  background: linear-gradient(135deg, rgba(220, 38, 38, 0.15) 0%, rgba(248, 113, 113, 0.08) 100%);
  opacity: 1;
}

.toast-emergency::before {
  background: linear-gradient(135deg, rgba(190, 18, 60, 0.2) 0%, rgba(251, 113, 133, 0.1) 100%);
  opacity: 1;
}

/* Левая цветная граница */
.toast-border {
  position: absolute;
  left: 0;
  top: 0;
  bottom: 0;
  width: 4px;
  transition: width 0.3s ease;
  z-index: 2;
}

.toast-card:hover .toast-border {
  width: 6px;
}

/* Обертка контента */
.toast-content-wrapper {
  padding: 24px;
  gap: 16px;
  position: relative;
  z-index: 1;
}

/* Иконка */
.toast-icon-wrapper {
  display: flex;
  align-items: flex-start;
  padding-top: 2px;
}

.toast-icon {
  filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.1));
}

/* Контент */
.toast-content {
  width: 100%;
  display: flex;
  flex-direction: column;
  color: rgba(0, 0, 0, 0.87) !important;
}

.toast-content :deep(.text-body-1),
.toast-content :deep(.text-caption) {
  color: rgba(0, 0, 0, 0.87) !important;
}

.toast-message {
  line-height: 1.5;
  color: rgba(0, 0, 0, 0.87) !important;
}

.toast-node {
  opacity: 0.8;
  font-weight: 500;
  color: rgba(0, 0, 0, 0.7) !important;
}

.toast-node :deep(.v-icon) {
  color: rgba(0, 0, 0, 0.7) !important;
}

.event-details-chips {
  width: 100%;
  flex-wrap: wrap;
}

.detail-chip {
  border-radius: 8px !important;
  color: rgba(0, 0, 0, 0.87) !important;
}

.detail-chip :deep(.v-chip__content) {
  color: rgba(0, 0, 0, 0.87) !important;
}

.detail-chip :deep(.v-icon) {
  color: rgba(0, 0, 0, 0.87) !important;
}

/* Кнопка закрытия */
.toast-close-btn {
  align-self: flex-start;
  opacity: 0.7;
  transition: opacity 0.2s ease;
}

.toast-close-btn:hover {
  opacity: 1;
}

/* Индикатор времени */
.toast-timer {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  height: 3px;
  animation: toastTimer linear forwards;
  transform-origin: left;
  z-index: 2;
}

/* Цветовые стили для разных уровней */
.toast-success .toast-border {
  background: linear-gradient(180deg, #4ade80 0%, #22c55e 100%);
}

.toast-success {
  border: 1px solid rgba(34, 197, 94, 0.2);
}

.toast-success .toast-icon {
  color: #22c55e;
}

.toast-success .toast-timer {
  background: linear-gradient(90deg, #22c55e, #4ade80);
}

.toast-info .toast-border {
  background: linear-gradient(180deg, #60a5fa 0%, #3b82f6 100%);
}

.toast-info {
  border: 1px solid rgba(59, 130, 246, 0.2);
}

.toast-info .toast-icon {
  color: #3b82f6;
}

.toast-info .toast-timer {
  background: linear-gradient(90deg, #3b82f6, #60a5fa);
}

.toast-warning .toast-border {
  background: linear-gradient(180deg, #fbbf24 0%, #f59e0b 100%);
}

.toast-warning {
  border: 1px solid rgba(245, 158, 11, 0.2);
}

.toast-warning .toast-icon {
  color: #f59e0b;
}

.toast-warning .toast-timer {
  background: linear-gradient(90deg, #f59e0b, #fbbf24);
}

.toast-error .toast-border {
  background: linear-gradient(180deg, #f87171 0%, #ef4444 100%);
}

.toast-error {
  border: 1px solid rgba(239, 68, 68, 0.2);
}

.toast-error .toast-icon {
  color: #ef4444;
}

.toast-error .toast-timer {
  background: linear-gradient(90deg, #ef4444, #f87171);
}

.toast-critical .toast-border {
  background: linear-gradient(180deg, #f87171 0%, #dc2626 100%);
}

.toast-critical {
  border: 1px solid rgba(220, 38, 38, 0.3);
  box-shadow: 0 8px 32px rgba(220, 38, 38, 0.15);
}

.toast-critical .toast-icon {
  color: #dc2626;
  animation: pulse 2s ease-in-out infinite;
}

.toast-critical .toast-timer {
  background: linear-gradient(90deg, #dc2626, #f87171);
}

.toast-emergency .toast-border {
  background: linear-gradient(180deg, #fb7185 0%, #be123c 100%);
  width: 6px;
}

.toast-emergency {
  border: 1px solid rgba(190, 18, 60, 0.4);
  box-shadow: 0 12px 40px rgba(190, 18, 60, 0.2);
}

.toast-emergency .toast-icon {
  color: #be123c;
  animation: pulse 1.5s ease-in-out infinite;
}

.toast-emergency .toast-timer {
  background: linear-gradient(90deg, #be123c, #fb7185);
}

/* Группированные тосты */
.toast-grouped {
  opacity: 0.95;
}


/* Анимации */
@keyframes toastEnter {
  from {
    opacity: 0;
    transform: translateX(100%) scale(0.9);
  }
  to {
    opacity: 1;
    transform: translateX(0) scale(1);
  }
}

@keyframes toastTimer {
  from {
    transform: scaleX(1);
  }
  to {
    transform: scaleX(0);
  }
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
    transform: scale(1);
  }
  50% {
    opacity: 0.8;
    transform: scale(1.05);
  }
}
</style>

<style>
.popup-toast {
  z-index: 10000 !important;
  position: fixed !important;
  right: 24px !important;
  min-width: 400px;
  max-width: 600px;
  max-height: calc(100vh - 48px);
  overflow-y: auto;
  pointer-events: auto;
}

/* Анимации для появления/исчезновения */
.toast-enter-active {
  transition: all 0.4s cubic-bezier(0.34, 1.56, 0.64, 1);
}

.toast-leave-active {
  transition: all 0.3s ease-in-out;
}

.toast-enter-from {
  opacity: 0;
  transform: translateX(100%) scale(0.9);
}

.toast-leave-to {
  opacity: 0;
  transform: translateX(100%) scale(0.9);
}
</style>

