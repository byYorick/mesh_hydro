<template>
  <v-card
    :class="['cycle-card', { 'cycle-card--active': cycle.status === 'active' }]"
    :variant="cycle.status === 'active' ? 'elevated' : 'outlined'"
    @click="$emit('select', cycle)"
  >
    <v-card-title class="d-flex align-center">
      <v-icon
        :icon="cycle.culture?.icon || 'mdi-sprout'"
        :color="cycle.culture?.category_color || 'primary'"
        class="mr-2"
      ></v-icon>
      <span class="text-h6">{{ cycle.culture?.name || 'Цикл роста' }}</span>
      <v-spacer></v-spacer>
      <v-chip
        :color="cycle.status_color || 'grey'"
        :prepend-icon="cycle.status_icon || 'mdi-help-circle'"
        size="small"
        variant="flat"
      >
        {{ statusText }}
      </v-chip>
    </v-card-title>

    <v-card-subtitle>
      <div class="d-flex align-center">
        <v-icon icon="mdi-map-marker" size="small" class="mr-1"></v-icon>
        <span>{{ cycle.zone?.name || `Зона #${cycle.zone_id}` }}</span>
      </div>
    </v-card-subtitle>

    <v-card-text>
      <!-- Прогресс -->
      <div v-if="cycle.status === 'active'" class="mb-4">
        <div class="d-flex justify-space-between mb-1">
          <span class="text-caption text-medium-emphasis">Прогресс</span>
          <span class="text-caption font-weight-medium">{{ Math.round(progressValue) }}%</span>
        </div>
        <v-progress-linear
          :model-value="progressValue"
          :color="getProgressColor(progressValue)"
          height="8"
          rounded
        ></v-progress-linear>
      </div>

      <v-row dense>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">День цикла</div>
          <div class="text-body-1 font-weight-bold">
            {{ cycle.current_day || 0 }}
          </div>
        </v-col>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">Осталось дней</div>
          <div class="text-body-1 font-weight-bold">
            {{ cycle.remaining_days || 0 }}
          </div>
        </v-col>
        <v-col cols="6" v-if="cycle.plant_count">
          <div class="text-caption text-medium-emphasis">Растений</div>
          <div class="text-body-2 font-weight-medium">
            {{ cycle.plant_count }}
          </div>
        </v-col>
        <v-col cols="6" v-if="cycle.harvest_weight_kg">
          <div class="text-caption text-medium-emphasis">Урожай</div>
          <div class="text-body-2 font-weight-medium">
            {{ cycle.harvest_weight_kg.toFixed(2) }} кг
          </div>
        </v-col>
      </v-row>

      <!-- Текущая стадия -->
      <v-divider class="my-3" v-if="cycle.current_stage"></v-divider>
      <div v-if="cycle.current_stage" class="d-flex align-center">
        <v-icon
          :icon="cycle.current_stage.icon || 'mdi-sprout'"
          :color="cycle.current_stage.color || 'primary'"
          class="mr-2"
        ></v-icon>
        <div>
          <div class="text-caption text-medium-emphasis">Текущая стадия</div>
          <div class="text-body-2 font-weight-medium">{{ cycle.current_stage.name }}</div>
        </div>
      </div>

      <!-- Даты -->
      <v-divider class="my-3"></v-divider>
      <div class="text-caption">
        <div class="mb-1">
          <v-icon icon="mdi-calendar-start" size="x-small" class="mr-1"></v-icon>
          Начало: {{ formatDate(cycle.started_at) }}
        </div>
        <div v-if="cycle.expected_harvest_at">
          <v-icon icon="mdi-calendar-end" size="x-small" class="mr-1"></v-icon>
          Ожидаемый сбор: {{ formatDate(cycle.expected_harvest_at) }}
        </div>
        <div v-if="cycle.actual_harvest_at">
          <v-icon icon="mdi-basket" size="x-small" class="mr-1"></v-icon>
          Собран: {{ formatDate(cycle.actual_harvest_at) }}
        </div>
      </div>
    </v-card-text>

    <v-card-actions v-if="showActions">
      <v-btn
        variant="text"
        prepend-icon="mdi-eye"
        @click.stop="$emit('view', cycle)"
      >
        Подробнее
      </v-btn>
      <v-spacer></v-spacer>
      <v-btn
        v-if="cycle.status === 'active'"
        variant="text"
        color="success"
        prepend-icon="mdi-basket"
        @click.stop="$emit('harvest', cycle)"
      >
        Собрать урожай
      </v-btn>
      <v-btn
        v-if="cycle.status === 'active'"
        variant="text"
        color="error"
        prepend-icon="mdi-cancel"
        @click.stop="$emit('cancel', cycle)"
      >
        Отменить
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { GrowthCycle } from '@/stores/growth'

interface Props {
  cycle: GrowthCycle
  showActions?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  showActions: true,
})

defineEmits<{
  select: [cycle: GrowthCycle]
  view: [cycle: GrowthCycle]
  harvest: [cycle: GrowthCycle]
  cancel: [cycle: GrowthCycle]
}>()

const statusText = computed(() => {
  const map: Record<string, string> = {
    planning: 'Планирование',
    active: 'Активен',
    paused: 'Приостановлен',
    harvested: 'Собран',
    failed: 'Провален',
    cancelled: 'Отменён',
  }
  return map[props.cycle.status] || props.cycle.status
})

const progressValue = computed(() => {
  const raw = props.cycle.progress
  const numeric = typeof raw === 'string' ? Number(raw) : raw
  if (typeof numeric === 'number' && Number.isFinite(numeric)) {
    return Math.max(0, Math.min(100, numeric))
  }
  return 0
})

function getProgressColor(progress: number): string {
  if (progress < 30) return 'error'
  if (progress < 70) return 'warning'
  return 'success'
}

function formatDate(date: string | undefined): string {
  if (!date) return '—'
  return new Date(date).toLocaleDateString('ru-RU', {
    day: '2-digit',
    month: '2-digit',
    year: 'numeric',
  })
}
</script>

<style scoped>
.cycle-card {
  cursor: pointer;
  transition: all 0.2s ease;
}

.cycle-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.cycle-card--active {
  border: 2px solid rgb(var(--v-theme-success));
}
</style>

