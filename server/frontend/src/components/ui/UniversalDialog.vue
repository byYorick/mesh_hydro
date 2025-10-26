<template>
  <v-dialog
    :model-value="isOpen()"
    :max-width="config()?.maxWidth || 500"
    :persistent="config()?.persistent || config()?.loading"
    scrollable
    @update:model-value="closeDialog"
  >
    <v-card>
      <!-- Заголовок с иконкой -->
      <v-card-title class="d-flex align-center">
        <v-icon
          v-if="config()?.icon"
          :color="getColor()"
          class="mr-3"
        >
          {{ config()?.icon }}
        </v-icon>
        <span v-if="config()?.title">{{ config()?.title }}</span>
        <v-spacer></v-spacer>
        <v-btn
          icon="mdi-close"
          variant="text"
          size="small"
          :disabled="config()?.loading"
          @click="closeDialog"
        ></v-btn>
      </v-card-title>

      <v-divider></v-divider>

      <!-- Содержимое -->
      <v-card-text class="pt-6">
        <!-- Custom component -->
        <component
          v-if="config()?.component"
          :is="config()?.component"
          v-bind="config()?.props || {}"
        />
        
        <!-- Standard message -->
        <template v-else>
          <div v-if="config()?.message" class="text-body-1">
            {{ config()?.message }}
          </div>
        </template>

        <!-- Error message -->
        <v-alert
          v-if="error()"
          type="error"
          density="compact"
          class="mt-4"
        >
          {{ error() }}
        </v-alert>

        <!-- Loading overlay -->
        <v-overlay
          v-if="loading()"
          contained
          class="align-center justify-center"
        >
          <v-progress-circular
            indeterminate
            size="64"
            color="primary"
          ></v-progress-circular>
        </v-overlay>
      </v-card-text>

      <v-divider></v-divider>

      <!-- Действия -->
      <v-card-actions>
        <v-spacer></v-spacer>
        
        <!-- Cancel button -->
        <v-btn
          v-if="config()?.cancelText && config()?.type === 'confirm'"
          variant="text"
          :disabled="loading()"
          @click="closeDialog"
        >
          {{ config()?.cancelText }}
        </v-btn>

        <!-- Confirm button -->
        <v-btn
          v-if="config()?.confirmText"
          :color="getColor()"
          variant="elevated"
          :loading="loading()"
          :disabled="config()?.type === 'prompt' && !modelValue"
          @click="confirmDialog"
        >
          {{ config()?.confirmText }}
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useDialog } from '@/composables/useDialog'

const { isOpen, config, loading, error, closeDialog, confirmDialog } = useDialog()

// Для prompt типа
const modelValue = computed(() => {
  return config()?.props?.modelValue?.value || ''
})

// Цвет по типу диалога
const getColor = () => {
  if (config()?.color) return config()?.color
  
  switch (config()?.type) {
    case 'confirm':
      return 'primary'
    case 'alert':
      return 'info'
    case 'error':
      return 'error'
    case 'success':
      return 'success'
    case 'warning':
      return 'warning'
    default:
      return 'primary'
  }
}
</script>

<style scoped>
.v-card-text {
  min-height: 60px;
}
</style>
