<template>
  <v-card>
    <v-card-title>
      {{ title }}
      
      <v-spacer></v-spacer>
      
      <!-- Time range selector -->
      <v-btn-toggle
        v-model="selectedRange"
        mandatory
        density="compact"
        variant="outlined"
      >
        <v-btn value="1h">1ч</v-btn>
        <v-btn value="6h">6ч</v-btn>
        <v-btn value="24h">24ч</v-btn>
        <v-btn value="7d">7д</v-btn>
      </v-btn-toggle>
    </v-card-title>

    <v-card-text>
      <div class="chart-container">
        <Line
          v-if="chartData"
          :data="chartData"
          :options="chartOptions"
        />
        <div v-else class="text-center pa-4 text-disabled">
          <v-progress-circular indeterminate></v-progress-circular>
          <div class="mt-2">Загрузка данных...</div>
        </div>
      </div>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { ref, computed, watch, onMounted } from 'vue'
import { Line } from 'vue-chartjs'
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  TimeScale,
} from 'chart.js'
import 'chartjs-adapter-date-fns'

// Register Chart.js components
ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  TimeScale
)

const props = defineProps({
  title: {
    type: String,
    default: 'Телеметрия',
  },
  data: {
    type: Array,
    default: () => [],
  },
  fields: {
    type: Array,
    default: () => ['value'],
  },
  yAxisLabel: {
    type: String,
    default: '',
  },
})

const selectedRange = ref('24h')

// Chart data
const chartData = computed(() => {
  if (!props.data || props.data.length === 0) return null

  const datasets = props.fields.map((field, index) => ({
    label: field,
    data: props.data.map(item => ({
      x: new Date(item.received_at || item.time_bucket),
      y: item.data?.[field] || item[field],
    })),
    borderColor: getColor(index),
    backgroundColor: getColor(index, 0.1),
    tension: 0.4,
    fill: true,
  }))

  return {
    datasets,
  }
})

// Chart options
const chartOptions = computed(() => ({
  responsive: true,
  maintainAspectRatio: false,
  interaction: {
    mode: 'index',
    intersect: false,
  },
  plugins: {
    legend: {
      display: props.fields.length > 1,
      position: 'top',
    },
    tooltip: {
      callbacks: {
        title: (context) => {
          return new Date(context[0].parsed.x).toLocaleString('ru-RU')
        },
      },
    },
  },
  scales: {
    x: {
      type: 'time',
      time: {
        unit: getTimeUnit(),
      },
      title: {
        display: false,
      },
    },
    y: {
      title: {
        display: !!props.yAxisLabel,
        text: props.yAxisLabel,
      },
    },
  },
}))

// Get color for dataset
function getColor(index, alpha = 1) {
  const colors = [
    `rgba(33, 150, 243, ${alpha})`,  // Blue
    `rgba(255, 152, 0, ${alpha})`,   // Orange
    `rgba(76, 175, 80, ${alpha})`,   // Green
    `rgba(156, 39, 176, ${alpha})`,  // Purple
    `rgba(244, 67, 54, ${alpha})`,   // Red
  ]
  return colors[index % colors.length]
}

// Get time unit based on selected range
function getTimeUnit() {
  switch (selectedRange.value) {
    case '1h':
    case '6h':
      return 'minute'
    case '24h':
      return 'hour'
    case '7d':
      return 'day'
    default:
      return 'hour'
  }
}

// Watch for range changes
watch(selectedRange, (newRange) => {
  emit('range-changed', newRange)
})

const emit = defineEmits(['range-changed'])
</script>

<style scoped>
.chart-container {
  position: relative;
  height: 300px;
  width: 100%;
}
</style>

