<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon :icon="icon" class="mr-2"></v-icon>
      {{ title }}
      
      <v-spacer></v-spacer>
      
      <!-- Chart controls -->
      <v-btn-toggle
        v-model="selectedRange"
        mandatory
        density="compact"
        variant="outlined"
        class="mr-2"
      >
        <v-btn value="1h" size="small">1ч</v-btn>
        <v-btn value="6h" size="small">6ч</v-btn>
        <v-btn value="24h" size="small">24ч</v-btn>
        <v-btn value="7d" size="small">7д</v-btn>
      </v-btn-toggle>

      <v-btn
        icon="mdi-download"
        size="small"
        variant="text"
        @click="exportChart"
      ></v-btn>

      <v-btn
        icon="mdi-refresh"
        size="small"
        variant="text"
        @click="$emit('refresh')"
        :loading="loading"
      ></v-btn>
    </v-card-title>

    <v-card-text>
      <div class="chart-container">
        <Line
          v-if="chartData && chartData.datasets.length > 0"
          :data="chartData"
          :options="chartOptions"
        />
        <div v-else-if="loading" class="text-center pa-8">
          <v-progress-circular indeterminate color="primary"></v-progress-circular>
          <div class="mt-2 text-disabled">Загрузка данных...</div>
        </div>
        <div v-else class="text-center pa-8 text-disabled">
          <v-icon icon="mdi-chart-line-variant" size="64"></v-icon>
          <div class="mt-2">Нет данных для отображения</div>
        </div>
      </div>

      <!-- Statistics -->
      <v-row v-if="statistics" dense class="mt-2">
        <v-col cols="3">
          <v-card variant="tonal" color="info">
            <v-card-text class="text-center pa-2">
              <div class="text-caption">Среднее</div>
              <div class="text-h6">{{ statistics.avg }}</div>
            </v-card-text>
          </v-card>
        </v-col>
        <v-col cols="3">
          <v-card variant="tonal" color="success">
            <v-card-text class="text-center pa-2">
              <div class="text-caption">Мин</div>
              <div class="text-h6">{{ statistics.min }}</div>
            </v-card-text>
          </v-card>
        </v-col>
        <v-col cols="3">
          <v-card variant="tonal" color="error">
            <v-card-text class="text-center pa-2">
              <div class="text-caption">Макс</div>
              <div class="text-h6">{{ statistics.max }}</div>
            </v-card-text>
          </v-card>
        </v-col>
        <v-col cols="3">
          <v-card variant="tonal" color="grey">
            <v-card-text class="text-center pa-2">
              <div class="text-caption">Точек</div>
              <div class="text-h6">{{ statistics.count }}</div>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { ref, computed, watch } from 'vue'
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
  Filler,
} from 'chart.js'
import zoomPlugin from 'chartjs-plugin-zoom'

// Register Chart.js components and plugins
ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  TimeScale,
  Filler,
  zoomPlugin
)

const props = defineProps({
  title: {
    type: String,
    default: 'График',
  },
  icon: {
    type: String,
    default: 'mdi-chart-line',
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
  loading: {
    type: Boolean,
    default: false,
  },
})

const emit = defineEmits(['refresh', 'range-changed'])

const selectedRange = ref('24h')

// Calculate statistics
const statistics = computed(() => {
  if (!props.data || props.data.length === 0) return null

  const values = props.data
    .map(item => item.data?.[props.fields[0]] || item[props.fields[0]])
    .filter(v => v != null)

  if (values.length === 0) return null

  return {
    avg: (values.reduce((a, b) => a + b, 0) / values.length).toFixed(2),
    min: Math.min(...values).toFixed(2),
    max: Math.max(...values).toFixed(2),
    count: values.length,
  }
})

// Chart data
const chartData = computed(() => {
  if (!props.data || props.data.length === 0) return null

  const datasets = props.fields.map((field, index) => ({
    label: field.toUpperCase(),
    data: props.data.map(item => ({
      x: new Date(item.received_at || item.time_bucket || item.created_at),
      y: item.data?.[field] || item[field],
    })),
    borderColor: getColor(index),
    backgroundColor: getColor(index, 0.1),
    tension: 0.4,
    fill: true,
    pointRadius: 2,
    pointHoverRadius: 6,
  }))

  return { datasets }
})

// Chart options with zoom/pan
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
        label: (context) => {
          return `${context.dataset.label}: ${context.parsed.y.toFixed(2)}`
        },
      },
    },
    zoom: {
      pan: {
        enabled: true,
        mode: 'x',
        modifierKey: 'shift',
      },
      zoom: {
        wheel: {
          enabled: true,
        },
        pinch: {
          enabled: true,
        },
        mode: 'x',
      },
    },
  },
  scales: {
    x: {
      type: 'time',
      time: {
        unit: getTimeUnit(),
        displayFormats: {
          minute: 'HH:mm',
          hour: 'HH:mm',
          day: 'DD MMM',
        },
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
      beginAtZero: false,
    },
  },
}))

function getColor(index, alpha = 1) {
  const colors = [
    `rgba(33, 150, 243, ${alpha})`,   // Blue
    `rgba(255, 152, 0, ${alpha})`,    // Orange
    `rgba(76, 175, 80, ${alpha})`,    // Green
    `rgba(156, 39, 176, ${alpha})`,   // Purple
    `rgba(244, 67, 54, ${alpha})`,    // Red
    `rgba(0, 188, 212, ${alpha})`,    // Cyan
  ]
  return colors[index % colors.length]
}

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

watch(selectedRange, (newRange) => {
  emit('range-changed', newRange)
})

function exportChart() {
  // Export chart as PNG
  const canvas = document.querySelector('.chart-container canvas')
  if (canvas) {
    const url = canvas.toDataURL('image/png')
    const link = document.createElement('a')
    link.download = `chart_${Date.now()}.png`
    link.href = url
    link.click()
  }
}
</script>

<style scoped>
.chart-container {
  position: relative;
  height: 350px;
  width: 100%;
}
</style>

