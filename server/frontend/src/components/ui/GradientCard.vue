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
    validator: (value) => {
      if (!value || typeof value !== 'string') return false
      const validValues = ['primary', 'success', 'warning', 'error', 'info']
      try {
        // Дополнительная проверка на undefined/null перед вызовом includes
        if (validValues && Array.isArray(validValues) && value) {
          return validValues.includes(value)
        }
        return false
      } catch (error) {
        console.error('GradientCard.vue: validator - Error in includes:', error)
        console.error('GradientCard.vue: validator - value:', value, typeof value)
        console.error('GradientCard.vue: validator - validValues:', validValues, typeof validValues)
        return false
      }
    },
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

