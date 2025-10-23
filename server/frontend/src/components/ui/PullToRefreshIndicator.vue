<template>
  <transition name="fade">
    <div v-if="isPulling || isRefreshing" class="pull-to-refresh-indicator">
      <div
        class="refresh-circle"
        :style="{
          transform: `translateY(${pullDistance}px) rotate(${rotation}deg)`,
          opacity: Math.min(pullDistance / 80, 1),
        }"
      >
        <v-progress-circular
          v-if="isRefreshing"
          indeterminate
          color="primary"
          size="32"
        ></v-progress-circular>
        
        <v-icon
          v-else
          icon="mdi-arrow-down"
          color="primary"
          size="32"
        ></v-icon>
      </div>
    </div>
  </transition>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  isPulling: Boolean,
  isRefreshing: Boolean,
  pullDistance: {
    type: Number,
    default: 0,
  },
})

// Rotation animation для иконки
const rotation = computed(() => {
  return (props.pullDistance / 120) * 360
})
</script>

<style scoped>
.pull-to-refresh-indicator {
  position: fixed;
  top: 0;
  left: 50%;
  transform: translateX(-50%);
  z-index: 9999;
  pointer-events: none;
}

.refresh-circle {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 48px;
  height: 48px;
  background: rgba(255, 255, 255, 0.9);
  border-radius: 50%;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  transition: transform 0.2s ease-out;
}

.v-theme--dark .refresh-circle {
  background: rgba(26, 31, 58, 0.9);
}
</style>

