<template>
  <v-dialog
    v-model="dialog"
    max-width="800"
    persistent
    scrollable
  >
    <v-card>
      <v-card-title class="d-flex align-center">
        <v-icon icon="mdi-sprout" color="success" class="mr-2"></v-icon>
        <span>Создать новый цикл роста</span>
        <v-spacer></v-spacer>
        <v-btn
          icon="mdi-close"
          variant="text"
          @click="closeDialog"
        ></v-btn>
      </v-card-title>

      <v-divider></v-divider>

      <!-- Шаги -->
      <v-card-text>
        <!-- Индикатор шагов -->
        <v-stepper
          v-model="currentStep"
          :items="steps"
          color="success"
          class="mb-4"
        ></v-stepper>

        <!-- Шаг 1: Выбор зоны -->
        <div v-if="currentStep === 1">
          <h3 class="mb-4">Шаг 1: Выбор зоны</h3>
          <ZoneSelector
            v-model="formData.zone"
            :error-messages="errors.zone"
            @update:modelValue="validateStep(1)"
          />
        </div>

        <!-- Шаг 2: Выбор пресета -->
        <div v-if="currentStep === 2">
          <h3 class="mb-4">Шаг 2: Выбор пресета</h3>
              
              <!-- Фильтры -->
              <v-row dense class="mb-4">
                <v-col cols="12" md="6">
                  <v-select
                    v-model="presetFilters.culture_id"
                    :items="cultures"
                    label="Культура"
                    item-title="name"
                    item-value="id"
                    clearable
                    variant="outlined"
                    density="compact"
                    @update:model-value="loadPresets"
                  ></v-select>
                </v-col>
                <v-col cols="12" md="6">
                  <v-select
                    v-model="presetFilters.type"
                    :items="presetTypes"
                    label="Тип пресета"
                    clearable
                    variant="outlined"
                    density="compact"
                    @update:model-value="loadPresets"
                  ></v-select>
                </v-col>
              </v-row>

              <!-- Список пресетов -->
              <div v-if="loadingPresets" class="text-center py-8">
                <v-progress-circular indeterminate color="primary"></v-progress-circular>
              </div>

              <v-row v-else>
                <v-col
                  v-for="preset in filteredPresets"
                  :key="preset.id"
                  cols="12"
                  md="6"
                >
                  <GrowthPresetCard
                    :preset="preset"
                    :is-selected="formData.preset?.id === preset.id"
                    :show-actions="false"
                    @select="selectPreset"
                  />
                </v-col>
              </v-row>

              <v-alert
                v-if="filteredPresets.length === 0 && !loadingPresets"
                type="info"
                variant="tonal"
                class="mt-4"
              >
                Пресеты не найдены. Попробуйте изменить фильтры.
              </v-alert>
        </div>

        <!-- Шаг 3: Настройки -->
        <div v-if="currentStep === 3">
          <h3 class="mb-4">Шаг 3: Настройки цикла</h3>

              <!-- Предпросмотр выбранного пресета -->
              <v-card variant="outlined" class="mb-4" v-if="formData.preset">
                <v-card-title class="text-subtitle-1">
                  Выбранный пресет: {{ formData.preset.name }}
                </v-card-title>
                <v-card-text>
                  <v-row dense>
                    <v-col cols="6">
                      <div class="text-caption text-medium-emphasis">Культура</div>
                      <div class="text-body-2">{{ formData.preset.culture?.name }}</div>
                    </v-col>
                    <v-col cols="6">
                      <div class="text-caption text-medium-emphasis">Длительность</div>
                      <div class="text-body-2">{{ formData.preset.total_days }} дней</div>
                    </v-col>
                    <v-col cols="6">
                      <div class="text-caption text-medium-emphasis">Стадий</div>
                      <div class="text-body-2">{{ formData.preset.stages?.length || 0 }}</div>
                    </v-col>
                    <v-col cols="6">
                      <div class="text-caption text-medium-emphasis">Сложность</div>
                      <div class="text-body-2">{{ getDifficultyText(formData.preset.difficulty) }}</div>
                    </v-col>
                  </v-row>
                </v-card-text>
              </v-card>

              <!-- Просмотр стадий -->
              <v-expansion-panels v-if="formData.preset?.stages" class="mb-4">
                <v-expansion-panel>
                  <v-expansion-panel-title>
                    <span>Просмотр стадий роста ({{ formData.preset.stages.length }})</span>
                  </v-expansion-panel-title>
                  <v-expansion-panel-text>
                    <StageTimeline
                      :stages="formData.preset.stages"
                      :show-details="true"
                      :show-target-params="true"
                    />
                  </v-expansion-panel-text>
                </v-expansion-panel>
              </v-expansion-panels>

              <!-- Настройки -->
              <v-text-field
                v-model.number="formData.plant_count"
                label="Количество растений"
                type="number"
                min="1"
                variant="outlined"
                density="comfortable"
                prepend-inner-icon="mdi-sprout"
                :error-messages="errors.plant_count"
                hint="Опционально: укажите количество растений для отслеживания урожайности"
                persistent-hint
                class="mb-3"
              ></v-text-field>

              <v-textarea
                v-model="formData.notes"
                label="Заметки"
                variant="outlined"
                density="comfortable"
                prepend-inner-icon="mdi-note-text"
                rows="3"
                hint="Опционально: добавьте заметки о цикле"
                persistent-hint
              ></v-textarea>
        </div>
      </v-card-text>

      <v-divider></v-divider>

      <v-card-actions>
        <v-btn
          v-if="currentStep > 1"
          variant="text"
          prepend-icon="mdi-chevron-left"
          @click="currentStep--"
        >
          Назад
        </v-btn>
        <v-spacer></v-spacer>
        <v-btn
          variant="text"
          @click="closeDialog"
        >
          Отмена
        </v-btn>
        <v-btn
          v-if="currentStep < 3"
          color="primary"
          prepend-icon="mdi-chevron-right"
          @click="nextStep"
          :disabled="!canProceed"
        >
          Далее
        </v-btn>
        <v-btn
          v-else
          color="success"
          prepend-icon="mdi-check"
          @click="createCycle"
          :loading="creating"
          :disabled="!canCreate"
        >
          Создать цикл
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { useGrowthStore, type GrowthPreset, type GrowthCycle } from '@/stores/growth'
import { useZonesStore, type Zone } from '@/stores/zones'
import ZoneSelector from './ZoneSelector.vue'
import GrowthPresetCard from './GrowthPresetCard.vue'
import StageTimeline from './StageTimeline.vue'

interface Props {
  modelValue: boolean
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  'created': [cycle: GrowthCycle]
}>()

const growthStore = useGrowthStore()
const zonesStore = useZonesStore()

const dialog = computed({
  get: () => props.modelValue,
  set: (value) => emit('update:modelValue', value)
})

const currentStep = ref(1)
const creating = ref(false)
const loadingPresets = ref(false)

const steps = [
  'Зона',
  'Пресет',
  'Настройки',
]

const formData = ref<{
  zone: Zone | null
  preset: GrowthPreset | null
  plant_count?: number
  notes?: string
}>({
  zone: null,
  preset: null,
  plant_count: undefined,
  notes: '',
})

const errors = ref<Record<string, string[]>>({})

const presetFilters = ref<{
  culture_id?: number
  type?: string
}>({})

const presetTypes = [
  { title: 'Системный', value: 'system' },
  { title: 'Пользовательский', value: 'custom' },
]

const cultures = computed(() => growthStore.activeCultures)
const filteredPresets = computed(() => growthStore.presets)

const canProceed = computed(() => {
  if (currentStep.value === 1) return formData.value.zone !== null
  if (currentStep.value === 2) return formData.value.preset !== null
  return false
})

const canCreate = computed(() => {
  return formData.value.zone !== null && formData.value.preset !== null
})

watch(dialog, (isOpen) => {
  if (isOpen) {
    resetForm()
    loadInitialData()
  }
})

function resetForm() {
  currentStep.value = 1
  formData.value = {
    zone: null,
    preset: null,
    plant_count: undefined,
    notes: '',
  }
  errors.value = {}
  presetFilters.value = {}
}

async function loadInitialData() {
  if (zonesStore.zones.length === 0) {
    await zonesStore.fetchZones()
  }
  if (growthStore.cultures.length === 0) {
    await growthStore.fetchCultures()
  }
  await loadPresets()
}

async function loadPresets() {
  loadingPresets.value = true
  try {
    await growthStore.fetchPresets({
      culture_id: presetFilters.value.culture_id,
      type: presetFilters.value.type,
    })
  } finally {
    loadingPresets.value = false
  }
}

function validateStep(step: number): boolean {
  errors.value = {}
  
  if (step === 1) {
    if (!formData.value.zone) {
      errors.value.zone = ['Необходимо выбрать зону']
      return false
    }
    if (formData.value.zone.current_cycle_id) {
      errors.value.zone = ['Выбранная зона уже занята активным циклом']
      return false
    }
  }
  
  if (step === 2) {
    if (!formData.value.preset) {
      errors.value.preset = ['Необходимо выбрать пресет']
      return false
    }
  }
  
  return true
}

function nextStep() {
  if (validateStep(currentStep.value)) {
    if (currentStep.value === 2 && formData.value.preset && !formData.value.preset.stages) {
      // Загрузить детали пресета со стадиями
      growthStore.fetchPreset(formData.value.preset.id).then((preset) => {
        formData.value.preset = preset
        currentStep.value++
      })
    } else {
      currentStep.value++
    }
  }
}

function selectPreset(preset: GrowthPreset) {
  formData.value.preset = preset
  validateStep(2)
}

function getDifficultyText(difficulty: string): string {
  const map: Record<string, string> = {
    easy: 'Легко',
    medium: 'Средне',
    hard: 'Сложно',
  }
  return map[difficulty] || difficulty
}

async function createCycle() {
  if (!validateStep(3)) return
  
  creating.value = true
  errors.value = {}
  
  try {
    const cycle = await growthStore.createCycle({
      zone_id: formData.value.zone!.id,
      preset_id: formData.value.preset!.id,
      plant_count: formData.value.plant_count,
      notes: formData.value.notes,
    })
    
    emit('created', cycle)
    closeDialog()
  } catch (error: any) {
    if (error.response?.data?.errors) {
      errors.value = error.response.data.errors
    } else {
      errors.value = { general: [error.response?.data?.message || 'Ошибка при создании цикла'] }
    }
  } finally {
    creating.value = false
  }
}

function closeDialog() {
  dialog.value = false
  resetForm()
}

onMounted(() => {
  if (dialog.value) {
    loadInitialData()
  }
})
</script>

<style scoped>
.v-stepper {
  box-shadow: none;
}
</style>

