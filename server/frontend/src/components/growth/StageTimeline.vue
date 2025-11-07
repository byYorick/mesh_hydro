<template>
  <div class="stage-timeline">
    <v-timeline
      :density="density"
      :side="side"
      :truncate-line="timelineTruncateLine"
    >
      <v-timeline-item
        v-for="(stage, index) in stages"
        :key="stage.id"
        :dot-color="getStageColor(stage, index)"
        :size="getStageSize(stage)"
        :icon="stage.icon || getStageIcon(stage)"
      >
        <template v-if="showConnector" v-slot:opposite>
          <div class="text-caption text-medium-emphasis">
            {{ getStageDuration(stage) }}
          </div>
        </template>

        <v-card
          :class="['stage-card', { 'stage-card--active': isActive(stage), 'stage-card--completed': isCompleted(stage) }]"
          :variant="isActive(stage) ? 'elevated' : 'outlined'"
        >
          <v-card-title class="d-flex align-center">
            <v-icon
              :icon="stage.icon || getStageIcon(stage)"
              :color="getStageColor(stage, index)"
              class="mr-2"
            ></v-icon>
            <span class="text-subtitle-1">{{ stage.name }}</span>
            <v-spacer></v-spacer>
            <v-chip
              v-if="isActive(stage)"
              size="small"
              color="success"
              variant="flat"
            >
              Активна
            </v-chip>
            <v-chip
              v-else-if="isCompleted(stage)"
              size="small"
              color="primary"
              variant="flat"
            >
              Завершена
            </v-chip>
          </v-card-title>

          <v-card-text v-if="showDetails">
            <div class="mb-2">
              <span class="text-caption text-medium-emphasis">Длительность: </span>
              <span class="text-body-2 font-weight-medium">{{ stage.duration_days }} дней</span>
            </div>

            <div v-if="stage.description" class="mb-2">
              <div class="text-caption text-medium-emphasis mb-1">Описание:</div>
              <div class="text-body-2">{{ stage.description }}</div>
            </div>

            <div v-if="showTargetParams && stage.target_params" class="target-params mt-2">
              <div class="text-caption text-medium-emphasis mb-2">Целевые параметры</div>
              <v-row dense>
                <v-col
                  v-for="(value, key) in stage.target_params"
                  :key="key"
                  cols="6"
                  md="4"
                >
                  <div class="text-caption text-medium-emphasis">{{ formatParamName(key) }}</div>
                  <div class="text-body-2 font-weight-medium">{{ formatParamValue(value) }}</div>
                </v-col>
              </v-row>
            </div>
          </v-card-text>
        </v-card>
      </v-timeline-item>
    </v-timeline>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { GrowthStage } from '@/stores/growth'

interface Props {
  stages: GrowthStage[]
  currentStageId?: number
  completedStageIds?: number[]
  density?: 'default' | 'compact' | 'comfortable'
  side?: 'start' | 'end'
  truncateLine?: boolean
  showDetails?: boolean
  showTargetParams?: boolean
  showConnector?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  density: 'default',
  side: 'end',
  truncateLine: true,
  showDetails: true,
  showTargetParams: true,
  showConnector: true,
})

const timelineTruncateLine = computed(() => {
  if (props.truncateLine === true) {
    return 'both'
  }
  if (props.truncateLine === false) {
    return undefined
  }
  return props.truncateLine
})

function isActive(stage: GrowthStage): boolean {
  return props.currentStageId === stage.id
}

function isCompleted(stage: GrowthStage): boolean {
  return props.completedStageIds?.includes(stage.id) || false
}

function getStageColor(stage: GrowthStage, index: number): string {
  if (isActive(stage)) return 'success'
  if (isCompleted(stage)) return 'primary'
  
  const colors = ['primary', 'secondary', 'info', 'warning', 'error']
  return colors[index % colors.length]
}

function getStageSize(stage: GrowthStage): number {
  if (isActive(stage)) return 48
  if (isCompleted(stage)) return 40
  return 32
}

function getStageIcon(stage: GrowthStage): string {
  const name = stage.name.toLowerCase()
  if (name.includes('проращ') || name.includes('seed')) return 'mdi-seed'
  if (name.includes('вегет') || name.includes('vegetative')) return 'mdi-leaf'
  if (name.includes('цвет') || name.includes('flower')) return 'mdi-flower'
  if (name.includes('плод') || name.includes('fruit')) return 'mdi-fruit-grapes'
  if (name.includes('сбор') || name.includes('harvest')) return 'mdi-basket'
  return 'mdi-sprout-outline'
}

function getStageDuration(stage: GrowthStage): string {
  return `${stage.duration_days} дн.`
}

function formatParamName(key: string): string {
  const map: Record<string, string> = {
    ph_min: 'pH мин',
    ph_max: 'pH макс',
    ec_min: 'EC мин',
    ec_max: 'EC макс',
    temp_min: 'Темп. мин',
    temp_max: 'Темп. макс',
    humidity_min: 'Влажн. мин',
    humidity_max: 'Влажн. макс',
    light_intensity: 'Освещение',
    light_hours: 'Часов света',
  }
  return map[key] || key
}

function formatParamValue(value: any): string {
  if (typeof value === 'number') {
    if (value < 10) return value.toFixed(1)
    return value.toString()
  }
  return String(value)
}
</script>

<style scoped>
.stage-timeline {
  width: 100%;
}

.stage-card {
  transition: all 0.2s ease;
}

.stage-card--active {
  border: 2px solid rgb(var(--v-theme-success));
  box-shadow: 0 4px 12px rgba(var(--v-theme-success), 0.3);
}

.stage-card--completed {
  opacity: 0.8;
}
</style>

