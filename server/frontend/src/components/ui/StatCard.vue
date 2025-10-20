<template>
  <v-card :color="color" variant="tonal" class="stat-card elevation-hover">
    <v-card-text class="text-center pa-6">
      <v-icon :icon="icon" size="48" class="mb-2"></v-icon>
      
      <div class="text-h3 font-weight-bold mb-2">
        <AnimatedNumber :value="value" :duration="800" />
      </div>
      
      <div class="text-subtitle-1">{{ label }}</div>
      
      <div v-if="subtitle" class="text-caption text-medium-emphasis mt-1">
        {{ subtitle }}
      </div>
    </v-card-text>
  </v-card>
</template>

<script setup>
import AnimatedNumber from './AnimatedNumber.vue'

defineProps({
  icon: {
    type: String,
    required: true,
  },
  value: {
    type: Number,
    required: true,
  },
  label: {
    type: String,
    required: true,
  },
  subtitle: {
    type: String,
    default: null,
  },
  color: {
    type: String,
    default: 'primary',
  },
})
</script>

<style scoped>
.stat-card {
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.stat-card:hover {
  transform: translateY(-4px);
}
</style>

