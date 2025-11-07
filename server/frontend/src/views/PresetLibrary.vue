<template>
  <v-container fluid>
    <v-row>
      <v-col cols="12">
        <div class="d-flex align-center justify-space-between mb-4">
          <div>
            <h1 class="text-h4 mb-1">Библиотека пресетов</h1>
            <p class="text-medium-emphasis">Системные и пользовательские пресеты циклов роста</p>
          </div>
          <v-btn
            color="primary"
            prepend-icon="mdi-content-copy"
            @click="showCloneDialog = true"
            :disabled="!selectedPreset"
          >
            Клонировать
          </v-btn>
        </div>
      </v-col>
    </v-row>

    <!-- Фильтры -->
    <v-row class="mb-4">
      <v-col cols="12">
        <v-card variant="outlined">
          <v-card-text>
            <v-row dense>
              <v-col cols="12" md="4">
                <v-select
                  v-model="filters.culture_id"
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
              <v-col cols="12" md="4">
                <v-select
                  v-model="filters.type"
                  :items="presetTypes"
                  label="Тип пресета"
                  clearable
                  variant="outlined"
                  density="compact"
                  @update:model-value="loadPresets"
                ></v-select>
              </v-col>
              <v-col cols="12" md="4">
                <v-select
                  v-model="filters.system"
                  :items="systemTypes"
                  label="Система"
                  clearable
                  variant="outlined"
                  density="compact"
                  @update:model-value="loadPresets"
                ></v-select>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Список пресетов -->
    <v-row v-if="loading">
      <v-col cols="12">
        <div class="text-center py-8">
          <v-progress-circular indeterminate color="primary" size="64"></v-progress-circular>
          <p class="mt-4 text-medium-emphasis">Загрузка пресетов...</p>
        </div>
      </v-col>
    </v-row>

    <v-row v-else-if="presets.length === 0">
      <v-col cols="12">
        <v-card variant="outlined">
          <v-card-text class="text-center py-8">
            <v-icon icon="mdi-book-open-variant" size="64" color="grey" class="mb-4"></v-icon>
            <h3 class="mb-2">Пресеты не найдены</h3>
            <p class="text-medium-emphasis">Попробуйте изменить фильтры</p>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row v-else>
      <v-col
        v-for="preset in presets"
        :key="preset.id"
        cols="12"
        md="6"
        lg="4"
      >
        <GrowthPresetCard
          :preset="preset"
          :is-selected="selectedPreset?.id === preset.id"
          @select="selectPreset"
          @clone="handleClone"
          @view="viewPreset"
        />
      </v-col>
    </v-row>

    <!-- Диалог клонирования -->
    <v-dialog v-model="showCloneDialog" max-width="500">
      <v-card>
        <v-card-title>Клонировать пресет</v-card-title>
        <v-card-text>
          <v-text-field
            v-model="cloneName"
            label="Название нового пресета"
            variant="outlined"
            :rules="[v => !!v || 'Название обязательно']"
          ></v-text-field>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn variant="text" @click="showCloneDialog = false">Отмена</v-btn>
          <v-btn color="primary" @click="clonePreset" :loading="cloning">
            Клонировать
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Снекбар -->
    <v-snackbar
      v-model="snackbar.show"
      :color="snackbar.color"
      :timeout="3000"
    >
      {{ snackbar.message }}
    </v-snackbar>
  </v-container>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useGrowthStore, type GrowthPreset } from '@/stores/growth'
import GrowthPresetCard from '@/components/growth/GrowthPresetCard.vue'

const router = useRouter()
const growthStore = useGrowthStore()

const loading = ref(false)
const cloning = ref(false)
const selectedPreset = ref<GrowthPreset | null>(null)
const showCloneDialog = ref(false)
const cloneName = ref('')

const filters = ref<{
  culture_id?: number
  type?: string
  system?: string
}>({})

const presetTypes = [
  { title: 'Системный', value: 'system' },
  { title: 'Пользовательский', value: 'custom' },
]

const systemTypes = [
  { title: 'NFT', value: 'nft' },
  { title: 'DWC', value: 'dwc' },
  { title: 'Капельный полив', value: 'drip' },
  { title: 'Ebb & Flow', value: 'ebb_flow' },
  { title: 'Любая', value: 'any' },
]

const snackbar = ref({
  show: false,
  message: '',
  color: 'success',
})

const cultures = computed(() => growthStore.activeCultures)
const presets = computed(() => growthStore.presets)

async function loadPresets() {
  loading.value = true
  try {
    await growthStore.fetchPresets(filters.value)
  } catch (error) {
    showSnackbar('Ошибка при загрузке пресетов', 'error')
  } finally {
    loading.value = false
  }
}

function selectPreset(preset: GrowthPreset) {
  selectedPreset.value = preset
}

function viewPreset(preset: GrowthPreset) {
  router.push({ name: 'PresetDetail', params: { id: preset.id } })
}

async function handleClone(preset: GrowthPreset) {
  selectedPreset.value = preset
  cloneName.value = `${preset.name} (копия)`
  showCloneDialog.value = true
}

async function clonePreset() {
  if (!selectedPreset.value || !cloneName.value) return

  cloning.value = true
  try {
    await growthStore.clonePreset(selectedPreset.value.id, cloneName.value)
    showSnackbar('Пресет успешно клонирован!', 'success')
    showCloneDialog.value = false
    cloneName.value = ''
    await loadPresets()
  } catch (error) {
    showSnackbar('Ошибка при клонировании пресета', 'error')
  } finally {
    cloning.value = false
  }
}

function showSnackbar(message: string, color: string = 'success') {
  snackbar.value = { show: true, message, color }
}

onMounted(async () => {
  if (growthStore.cultures.length === 0) {
    await growthStore.fetchCultures()
  }
  await loadPresets()
})
</script>

