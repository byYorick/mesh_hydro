<template>
  <v-card
    :class="['gradient-card', gradientClass]"
    v-bind="$attrs"
  >
    <slot></slot>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  gradient: {
    type: String,
    default: 'primary',
    validator: (value) => value && typeof value === 'string' && ['primary', 'success', 'warning', 'error', 'info'].includes(value),
  },
})

const gradientClass = computed(() => `gradient-${props.gradient}`)
</script>

<style scoped>
.gradient-card {
  background: linear-gradient(135deg, var(--gradient-start), var(--gradient-end));
  color: white;
}

.gradient-card.gradient-primary {
  --gradient-start: #667eea;
  --gradient-end: #764ba2;
}

.gradient-card.gradient-success {
  --gradient-start: #11998e;
  --gradient-end: #38ef7d;
}

.gradient-card.gradient-warning {
  --gradient-start: #f093fb;
  --gradient-end: #f5576c;
}

.gradient-card.gradient-error {
  --gradient-start: #fa709a;
  --gradient-end: #fee140;
}

.gradient-card.gradient-info {
  --gradient-start: #4facfe;
  --gradient-end: #00f2fe;
}
</style>

