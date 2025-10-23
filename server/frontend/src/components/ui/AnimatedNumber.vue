<template>
  <span>{{ displayValue }}</span>
</template>

<script setup>
import { ref, watch, onMounted } from 'vue'

const props = defineProps({
  value: {
    type: Number,
    required: true,
  },
  duration: {
    type: Number,
    default: 1000,
  },
  decimals: {
    type: Number,
    default: 0,
  },
})

const displayValue = ref(0)

function animateValue(start, end, duration) {
  const startTime = performance.now()
  const diff = end - start

  function update(currentTime) {
    const elapsed = currentTime - startTime
    const progress = Math.min(elapsed / duration, 1)
    
    // Easing function (ease-out)
    const eased = 1 - Math.pow(1 - progress, 3)
    
    const current = start + (diff * eased)
    displayValue.value = current.toFixed(props.decimals)

    if (progress < 1) {
      requestAnimationFrame(update)
    } else {
      displayValue.value = end.toFixed(props.decimals)
    }
  }

  requestAnimationFrame(update)
}

watch(() => props.value, (newValue, oldValue) => {
  if (newValue !== oldValue) {
    animateValue(oldValue || 0, newValue, props.duration)
  }
})

onMounted(() => {
  animateValue(0, props.value, props.duration)
})
</script>

