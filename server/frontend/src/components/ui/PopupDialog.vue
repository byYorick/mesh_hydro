<template>
  <!-- Рендерим только активный диалог (верхний в стеке) -->
  <v-dialog
    :model-value="true"
    :max-width="dialog.maxWidth || 500"
    :persistent="dialog.persistent || dialog.loading"
    :z-index="dialog.zIndex"
    scrollable
    @update:model-value="handleClose"
  >
    <v-card :style="{ zIndex: dialog.zIndex }" class="popup-dialog-card">
      <!-- Верхняя граница с градиентом -->
      <div class="dialog-top-border"></div>
      
      <!-- Заголовок с иконкой -->
      <v-card-title class="dialog-header">
        <div class="d-flex align-center">
          <div v-if="dialog.icon" class="dialog-icon-wrapper">
            <v-icon
              :icon="dialog.icon"
              class="dialog-icon"
            ></v-icon>
          </div>
          <span v-if="dialog.title" class="dialog-title">{{ dialog.title }}</span>
        </div>
        <v-spacer></v-spacer>
        <v-btn
          icon="mdi-close"
          variant="text"
          size="small"
          class="dialog-close-btn"
          :disabled="dialog.loading"
          @click="handleClose"
        ></v-btn>
      </v-card-title>

      <v-divider></v-divider>

      <!-- Содержимое -->
      <v-card-text class="dialog-content pt-6" style="overflow-y: auto; flex: 1 1 auto;">
        <!-- Custom component -->
        <component
          v-if="dialog.component"
          :is="dialog.component"
          v-bind="dialog.props || {}"
        />
        
        <!-- Standard message -->
        <template v-else>
          <div v-if="dialog.message" class="text-body-1 dialog-message">
            {{ dialog.message }}
          </div>
        </template>

      <!-- Error message -->
      <v-alert
        v-if="dialog.error"
        type="error"
        density="compact"
        variant="tonal"
        class="mt-4 dialog-error-alert"
        icon="mdi-alert-circle"
      >
        {{ dialog.error }}
      </v-alert>

        <!-- Loading overlay -->
        <v-overlay
          v-if="dialog.loading"
          contained
          class="align-center justify-center loading-overlay"
        >
          <div class="loading-content">
            <v-progress-circular
              indeterminate
              size="64"
              :color="getColor(dialog)"
            ></v-progress-circular>
          </div>
        </v-overlay>
      </v-card-text>

      <v-divider v-if="!dialog.loading"></v-divider>

      <!-- Действия -->
      <v-card-actions v-if="!dialog.loading" class="dialog-actions" style="flex-shrink: 0;">
        <v-spacer></v-spacer>
        
        <!-- Cancel button -->
        <v-btn
          v-if="dialog.cancelText && (dialog.type === 'confirm' || dialog.type === 'prompt')"
          variant="outlined"
          class="dialog-cancel-btn"
          :disabled="dialog.loading"
          @click="handleClose"
        >
          {{ dialog.cancelText }}
        </v-btn>

        <!-- Confirm button -->
        <v-btn
          v-if="dialog.confirmText"
          :color="getColor(dialog)"
          variant="elevated"
          class="dialog-confirm-btn"
          :disabled="dialog.type === 'prompt' && !getModelValue(dialog)"
          @click="handleConfirm"
        >
          {{ dialog.confirmText }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import type { DialogItem } from '@/composables/usePopup'

defineProps<{
  dialog: DialogItem
}>()

const emit = defineEmits<{
  confirm: []
  cancel: []
}>()

const getModelValue = (dialog: DialogItem) => {
  return (dialog.props?.modelValue as any)?.value || ''
}

const getColor = (dialog: DialogItem) => {
  if (dialog.color) return dialog.color
  
  switch (dialog.type) {
    case 'confirm':
      return 'primary'
    case 'prompt':
      return 'primary'
    default:
      return 'primary'
  }
}

const handleClose = () => {
  emit('cancel')
}

const handleConfirm = () => {
  emit('confirm')
}
</script>

<style scoped>
/* Основная карточка диалога */
.popup-dialog-card {
  border-radius: 20px !important;
  overflow: hidden;
  position: relative;
  animation: dialogEnter 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.15);
  background: rgba(255, 255, 255, 1) !important;
  max-height: calc(100vh - 96px);
  display: flex;
  flex-direction: column;
}

.v-theme--dark .popup-dialog-card {
  background: rgba(26, 31, 58, 1) !important;
}

/* Верхняя граница с градиентом */
.dialog-top-border {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 4px;
  background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
  z-index: 1;
}

/* Заголовок */
.dialog-header {
  padding: 24px 28px 20px !important;
  min-height: 64px;
}

.dialog-icon-wrapper {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  border-radius: 12px;
  background: linear-gradient(135deg, rgba(102, 126, 234, 0.1), rgba(118, 75, 162, 0.1));
  margin-right: 12px;
}

.dialog-icon {
  font-size: 24px;
  color: rgba(0, 0, 0, 0.87) !important;
}

.dialog-title {
  font-size: 1.25rem;
  font-weight: 600;
  color: rgba(0, 0, 0, 0.87) !important;
}

.dialog-close-btn {
  opacity: 0.6;
  transition: opacity 0.2s ease;
}

.dialog-close-btn:hover {
  opacity: 1;
}

/* Контент */
.dialog-content {
  padding: 0 28px 24px !important;
  min-height: 80px;
  position: relative;
  color: rgba(0, 0, 0, 0.87) !important;
  overflow-y: auto;
  flex: 1 1 auto;
}

.dialog-content :deep(.text-body-1),
.dialog-content :deep(.text-caption),
.dialog-content :deep(.text-body-2) {
  color: rgba(0, 0, 0, 0.87) !important;
}

.dialog-message {
  line-height: 1.6;
  color: rgba(0, 0, 0, 0.87) !important;
  font-size: 1rem;
}

.dialog-error-alert {
  color: rgba(0, 0, 0, 0.87) !important;
}

.dialog-error-alert :deep(.v-alert__content) {
  color: rgba(0, 0, 0, 0.87) !important;
}

.dialog-error-alert :deep(.text-body-1),
.dialog-error-alert :deep(.text-caption) {
  color: rgba(0, 0, 0, 0.87) !important;
}

/* Loading overlay */
.loading-overlay {
  background: rgba(255, 255, 255, 0.5) !important;
  backdrop-filter: blur(4px);
}

.v-theme--dark .loading-overlay {
  background: rgba(0, 0, 0, 0.5) !important;
}

.loading-content {
  animation: loadingPulse 2s ease-in-out infinite;
}

/* Действия */
.dialog-actions {
  padding: 16px 28px 24px !important;
  gap: 12px;
}

.dialog-cancel-btn {
  min-width: 100px;
  border-radius: 12px !important;
  text-transform: none;
  font-weight: 500;
}

.dialog-confirm-btn {
  min-width: 120px;
  border-radius: 12px !important;
  text-transform: none;
  font-weight: 600;
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4) !important;
  transition: all 0.3s ease;
}

.dialog-confirm-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(102, 126, 234, 0.5) !important;
}

.dialog-confirm-btn:active {
  transform: translateY(0);
}

/* Анимации */
@keyframes dialogEnter {
  from {
    opacity: 0;
    transform: scale(0.9) translateY(-20px);
  }
  to {
    opacity: 1;
    transform: scale(1) translateY(0);
  }
}

@keyframes loadingPulse {
  0%, 100% {
    opacity: 1;
    transform: scale(1);
  }
  50% {
    opacity: 0.8;
    transform: scale(1.05);
  }
}

/* Темная тема */
.v-theme--dark .popup-dialog-card {
  border: 1px solid rgba(255, 255, 255, 0.1);
}

.v-theme--dark .dialog-icon-wrapper {
  background: linear-gradient(135deg, rgba(102, 126, 234, 0.2), rgba(118, 75, 162, 0.2));
}

/* Адаптивность */
@media (max-width: 600px) {
  .popup-dialog-card {
    border-radius: 16px !important;
    margin: 16px;
  }

  .dialog-header {
    padding: 16px 20px 12px !important;
  }

  .dialog-content {
    padding: 0 20px 16px !important;
  }

  .dialog-actions {
    padding: 12px 20px 16px !important;
  }

  .dialog-icon-wrapper {
    width: 36px;
    height: 36px;
    margin-right: 10px;
  }

  .dialog-title {
    font-size: 1.125rem;
  }
}
</style>

