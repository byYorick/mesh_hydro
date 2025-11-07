<template>
  <v-card
    :class="['preset-card', { 'preset-card--selected': isSelected }]"
    :variant="preset.preset_type === 'system' ? 'outlined' : 'elevated'"
    @click="$emit('select', preset)"
  >
    <v-card-title class="d-flex align-center">
      <v-icon
        :icon="preset.culture?.icon || 'mdi-sprout'"
        :color="preset.culture?.category_color || 'primary'"
        class="mr-2"
      ></v-icon>
      <span class="text-h6">{{ preset.name }}</span>
      <v-spacer></v-spacer>
      <v-chip
        v-if="preset.preset_type === 'system'"
        size="small"
        color="primary"
        variant="flat"
      >
        Системный
      </v-chip>
      <v-chip
        v-else
        size="small"
        color="secondary"
        variant="flat"
      >
        Пользовательский
      </v-chip>
    </v-card-title>

    <v-card-subtitle>
      {{ preset.culture?.name || 'Культура не указана' }}
    </v-card-subtitle>

    <v-card-text>
      <div v-if="preset.description" class="mb-3">
        {{ preset.description }}
      </div>

      <v-row dense>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">Длительность</div>
          <div class="text-body-2 font-weight-medium">
            {{ preset.total_days }} дней
          </div>
        </v-col>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">Сложность</div>
          <div class="d-flex align-center">
            <v-icon
              :icon="preset.difficulty_icon"
              :color="preset.difficulty_color"
              size="small"
              class="mr-1"
            ></v-icon>
            <span class="text-body-2 font-weight-medium">
              {{ difficultyText }}
            </span>
          </div>
        </v-col>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">Стадий</div>
          <div class="text-body-2 font-weight-medium">
            {{ preset.stages?.length || 0 }}
          </div>
        </v-col>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">Использований</div>
          <div class="text-body-2 font-weight-medium">
            {{ preset.usage_count }}
          </div>
        </v-col>
      </v-row>

      <v-divider class="my-3" v-if="preset.avg_rating"></v-divider>

      <div v-if="preset.avg_rating" class="d-flex align-center">
        <v-rating
          :model-value="preset.avg_rating"
          readonly
          density="compact"
          half-increments
          size="small"
          class="mr-2"
        ></v-rating>
        <span class="text-caption text-medium-emphasis">
          {{ preset.avg_rating.toFixed(1) }} ({{ preset.usage_count }} использований)
        </span>
      </div>
    </v-card-text>

    <v-card-actions v-if="showActions">
      <v-btn
        variant="text"
        prepend-icon="mdi-content-copy"
        @click.stop="$emit('clone', preset)"
      >
        Клонировать
      </v-btn>
      <v-spacer></v-spacer>
      <v-btn
        variant="text"
        prepend-icon="mdi-eye"
        @click.stop="$emit('view', preset)"
      >
        Подробнее
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { GrowthPreset } from '@/stores/growth'

interface Props {
  preset: GrowthPreset
  isSelected?: boolean
  showActions?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  isSelected: false,
  showActions: true,
})

defineEmits<{
  select: [preset: GrowthPreset]
  clone: [preset: GrowthPreset]
  view: [preset: GrowthPreset]
}>()

const difficultyText = computed(() => {
  const map: Record<string, string> = {
    easy: 'Легко',
    medium: 'Средне',
    hard: 'Сложно',
  }
  return map[props.preset.difficulty] || props.preset.difficulty
})
</script>

<style scoped>
.preset-card {
  cursor: pointer;
  transition: all 0.2s ease;
}

.preset-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.preset-card--selected {
  border: 2px solid rgb(var(--v-theme-primary));
}
</style>

