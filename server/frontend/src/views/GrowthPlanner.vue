<template>
  <v-container fluid>
    <v-row>
      <v-col cols="12">
        <div class="d-flex align-center justify-space-between mb-4">
          <div>
            <h1 class="text-h4 mb-1">Планировщик циклов роста</h1>
            <p class="text-medium-emphasis">Управление циклами выращивания растений в зонах</p>
          </div>
          <v-btn
            color="success"
            prepend-icon="mdi-plus"
            @click="showCreateDialog = true"
          >
            Создать цикл
          </v-btn>
        </div>
      </v-col>
    </v-row>

    <!-- Статистика -->
    <v-row class="mb-4">
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="d-flex align-center">
              <v-icon icon="mdi-play-circle" color="success" size="large" class="mr-3"></v-icon>
              <div>
                <div class="text-caption text-medium-emphasis">Активных циклов</div>
                <div class="text-h5 font-weight-bold">{{ activeCyclesCount }}</div>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="d-flex align-center">
              <v-icon icon="mdi-check-circle" color="primary" size="large" class="mr-3"></v-icon>
              <div>
                <div class="text-caption text-medium-emphasis">Завершённых</div>
                <div class="text-h5 font-weight-bold">{{ completedCyclesCount }}</div>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="d-flex align-center">
              <v-icon icon="mdi-map-marker" color="info" size="large" class="mr-3"></v-icon>
              <div>
                <div class="text-caption text-medium-emphasis">Доступных зон</div>
                <div class="text-h5 font-weight-bold">{{ availableZonesCount }}</div>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="d-flex align-center">
              <v-icon icon="mdi-book-open-variant" color="warning" size="large" class="mr-3"></v-icon>
              <div>
                <div class="text-caption text-medium-emphasis">Пресетов</div>
                <div class="text-h5 font-weight-bold">{{ presetsCount }}</div>
              </div>
            </div>
          </v-card-text>
        </v-card>
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
                  v-model="filters.status"
                  :items="statusOptions"
                  label="Статус"
                  clearable
                  variant="outlined"
                  density="compact"
                  @update:model-value="loadCycles"
                ></v-select>
              </v-col>
              <v-col cols="12" md="4">
                <v-select
                  v-model="filters.zone_id"
                  :items="zones"
                  label="Зона"
                  item-title="name"
                  item-value="id"
                  clearable
                  variant="outlined"
                  density="compact"
                  @update:model-value="loadCycles"
                ></v-select>
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field
                  v-model="searchQuery"
                  label="Поиск"
                  prepend-inner-icon="mdi-magnify"
                  variant="outlined"
                  density="compact"
                  clearable
                ></v-text-field>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Список циклов -->
    <v-row v-if="loading">
      <v-col cols="12">
        <div class="text-center py-8">
          <v-progress-circular indeterminate color="primary" size="64"></v-progress-circular>
          <p class="mt-4 text-medium-emphasis">Загрузка циклов...</p>
        </div>
      </v-col>
    </v-row>

    <v-row v-else-if="filteredCycles.length === 0">
      <v-col cols="12">
        <v-card variant="outlined">
          <v-card-text class="text-center py-8">
            <v-icon icon="mdi-sprout-outline" size="64" color="grey" class="mb-4"></v-icon>
            <h3 class="mb-2">Нет циклов роста</h3>
            <p class="text-medium-emphasis mb-4">Создайте первый цикл роста для начала работы</p>
            <v-btn
              color="success"
              prepend-icon="mdi-plus"
              @click="showCreateDialog = true"
            >
              Создать цикл
            </v-btn>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row v-else>
      <v-col
        v-for="cycle in filteredCycles"
        :key="cycle.id"
        cols="12"
        md="6"
        lg="4"
      >
        <CycleCard
          :cycle="cycle"
          @select="viewCycle"
          @view="viewCycle"
          @harvest="handleHarvest"
          @cancel="handleCancel"
        />
      </v-col>
    </v-row>

    <!-- Диалог создания цикла -->
    <CreateCycleDialog
      v-model="showCreateDialog"
      @created="handleCycleCreated"
    />

    <!-- Снекбар для уведомлений -->
    <v-snackbar
      v-model="snackbar.show"
      :color="snackbar.color"
      :timeout="3000"
    >
      {{ snackbar.message }}
      <template v-slot:actions>
        <v-btn variant="text" @click="snackbar.show = false">Закрыть</v-btn>
      </template>
    </v-snackbar>
  </v-container>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'
import { useRouter } from 'vue-router'
import { useGrowthStore, type GrowthCycle } from '@/stores/growth'
import { useZonesStore } from '@/stores/zones'
import CycleCard from '@/components/growth/CycleCard.vue'
import CreateCycleDialog from '@/components/growth/CreateCycleDialog.vue'

const router = useRouter()
const growthStore = useGrowthStore()
const zonesStore = useZonesStore()

const loading = ref(false)
const showCreateDialog = ref(false)
const searchQuery = ref('')

const filters = ref<{
  status?: string
  zone_id?: number
}>({})

const statusOptions = [
  { title: 'Планирование', value: 'planning' },
  { title: 'Активен', value: 'active' },
  { title: 'Приостановлен', value: 'paused' },
  { title: 'Собран', value: 'harvested' },
  { title: 'Провален', value: 'failed' },
  { title: 'Отменён', value: 'cancelled' },
]

const snackbar = ref({
  show: false,
  message: '',
  color: 'success',
})

const activeCyclesCount = computed(() => growthStore.activeCycles.length)
const completedCyclesCount = computed(() => growthStore.completedCycles.length)
const availableZonesCount = computed(() => zonesStore.availableZones.length)
const presetsCount = computed(() => growthStore.presets.length)

const zones = computed(() => zonesStore.activeZones)

const filteredCycles = computed(() => {
  let cycles = growthStore.cycles

  if (filters.value.status) {
    cycles = cycles.filter(c => c.status === filters.value.status)
  }

  if (filters.value.zone_id) {
    cycles = cycles.filter(c => c.zone_id === filters.value.zone_id)
  }

  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    cycles = cycles.filter(c =>
      c.culture?.name.toLowerCase().includes(query) ||
      c.zone?.name.toLowerCase().includes(query) ||
      c.preset?.name.toLowerCase().includes(query)
    )
  }

  return cycles.sort((a, b) => {
    // Активные циклы в начале
    if (a.status === 'active' && b.status !== 'active') return -1
    if (a.status !== 'active' && b.status === 'active') return 1
    // Затем по дате начала
    return new Date(b.started_at).getTime() - new Date(a.started_at).getTime()
  })
})

async function loadCycles() {
  loading.value = true
  try {
    await growthStore.fetchCycles(filters.value)
  } catch (error) {
    showSnackbar('Ошибка при загрузке циклов', 'error')
  } finally {
    loading.value = false
  }
}

function viewCycle(cycle: GrowthCycle) {
  router.push({ name: 'CycleDetail', params: { id: cycle.id } })
}

function handleCycleCreated(cycle: GrowthCycle) {
  showSnackbar('Цикл успешно создан!', 'success')
  loadCycles()
}

async function handleHarvest(cycle: GrowthCycle) {
  // TODO: Открыть диалог сбора урожая
  router.push({ name: 'CycleDetail', params: { id: cycle.id }, query: { action: 'harvest' } })
}

async function handleCancel(cycle: GrowthCycle) {
  // TODO: Открыть диалог отмены
  if (confirm('Вы уверены, что хотите отменить этот цикл?')) {
    try {
      await growthStore.cancelCycle(cycle.id, 'Отменено пользователем')
      showSnackbar('Цикл отменён', 'info')
      loadCycles()
    } catch (error) {
      showSnackbar('Ошибка при отмене цикла', 'error')
    }
  }
}

function showSnackbar(message: string, color: string = 'success') {
  snackbar.value = { show: true, message, color }
}

onMounted(async () => {
  if (zonesStore.zones.length === 0) {
    await zonesStore.fetchZones()
  }
  await loadCycles()
})
</script>

