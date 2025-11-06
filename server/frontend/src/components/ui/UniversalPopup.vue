<template>
  <div class="universal-popup-container">
    <!-- Dialogs Section -->
    <PopupDialog
      v-for="dialog in dialogs"
      :key="dialog.id"
      :dialog="dialog"
      @confirm="handleConfirm"
      @cancel="handleCancel"
    />

    <!-- Toasts Section -->
    <PopupToast
      v-for="(toast, index) in toasts"
      :key="toast.id"
      :toast="toast"
      :index="index"
      @remove="handleToastRemove"
    />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { usePopup, type DialogItem } from '@/composables/usePopup'
import PopupDialog from './PopupDialog.vue'
import PopupToast from './PopupToast.vue'

const { dialog, toast, dialogs: dialogsState, toasts: toastsState } = usePopup()

const dialogs = computed(() => dialogsState.value)
const toasts = computed(() => toastsState.value)

const handleConfirm = () => {
  dialog.confirmCurrent()
}

const handleCancel = () => {
  dialog.close()
}

const handleToastRemove = (id: string) => {
  toast.remove(id)
}
</script>

<style scoped>
.universal-popup-container {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
  z-index: 10000;
}

.universal-popup-container :deep(.popup-toast),
.universal-popup-container :deep(.popup-dialog-card) {
  pointer-events: auto;
}
</style>

