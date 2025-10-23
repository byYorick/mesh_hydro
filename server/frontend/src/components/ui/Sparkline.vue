<template>
  <svg
    :width="width"
    :height="height"
    :viewBox="`0 0 ${width} ${height}`"
    class="sparkline"
  >
    <!-- Area fill -->
    <path
      v-if="showArea"
      :d="areaPath"
      :fill="gradientId"
      opacity="0.3"
    />
    
    <!-- Line -->
    <path
      :d="linePath"
      :stroke="color"
      stroke-width="2"
      fill="none"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    
    <!-- Dots -->
    <circle
      v-if="showDots"
      v-for="(point, index) in points"
      :key="index"
      :cx="point.x"
      :cy="point.y"
      r="2"
      :fill="color"
    />
    
    <!-- Gradient definition -->
    <defs>
      <linearGradient :id="gradientId" x1="0" x2="0" y1="0" y2="1">
        <stop offset="0%" :stop-color="color" stop-opacity="0.5"/>
        <stop offset="100%" :stop-color="color" stop-opacity="0"/>
      </linearGradient>
    </defs>
  </svg>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  data: {
    type: Array,
    default: () => [],
  },
  width: {
    type: Number,
    default: 100,
  },
  height: {
    type: Number,
    default: 30,
  },
  color: {
    type: String,
    default: '#2196F3',
  },
  showArea: {
    type: Boolean,
    default: true,
  },
  showDots: {
    type: Boolean,
    default: false,
  },
})

const gradientId = `gradient-${Math.random().toString(36).substr(2, 9)}`

// Calculate points
const points = computed(() => {
  if (!props.data || props.data.length === 0) return []
  
  const values = props.data.map(d => typeof d === 'number' ? d : d.value)
  const min = Math.min(...values)
  const max = Math.max(...values)
  const range = max - min || 1
  
  return values.map((value, index) => {
    const x = (index / (values.length - 1)) * props.width
    const y = props.height - ((value - min) / range) * (props.height - 10) - 5
    
    return { x, y }
  })
})

// Line path
const linePath = computed(() => {
  if (points.value.length === 0) return ''
  
  let path = `M ${points.value[0].x} ${points.value[0].y}`
  
  for (let i = 1; i < points.value.length; i++) {
    path += ` L ${points.value[i].x} ${points.value[i].y}`
  }
  
  return path
})

// Area path (with fill)
const areaPath = computed(() => {
  if (points.value.length === 0) return ''
  
  let path = `M ${points.value[0].x} ${props.height}`
  path += ` L ${points.value[0].x} ${points.value[0].y}`
  
  for (let i = 1; i < points.value.length; i++) {
    path += ` L ${points.value[i].x} ${points.value[i].y}`
  }
  
  path += ` L ${points.value[points.value.length - 1].x} ${props.height}`
  path += ' Z'
  
  return path
})
</script>

<style scoped>
.sparkline {
  display: block;
}
</style>

