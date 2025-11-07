<template>
  <v-container fluid>
    <v-row>
      <v-col cols="12">
        <div class="d-flex align-center justify-space-between mb-4">
          <div>
            <h1 class="text-h4 mb-1">Управление зонами</h1>
            <p class="text-medium-emphasis">Управление зонами выращивания и их узлами</p>
          </div>
          <v-btn
            color="primary"
            prepend-icon="mdi-plus"
            @click="showAddDialog = true"
          >
            Добавить зону
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
              <v-icon icon="mdi-map-marker" color="primary" size="large" class="mr-3"></v-icon>
              <div>
                <div class="text-caption text-medium-emphasis">Всего зон</div>
                <div class="text-h5 font-weight-bold">{{ zones.length }}</div>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="d-flex align-center">
              <v-icon icon="mdi-check-circle" color="success" size="large" class="mr-3"></v-icon>
              <div>
                <div class="text-caption text-medium-emphasis">Активных</div>
                <div class="text-h5 font-weight-bold">{{ activeZonesCount }}</div>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="d-flex align-center">
              <v-icon icon="mdi-account-check" color="info" size="large" class="mr-3"></v-icon>
              <div>
                <div class="text-caption text-medium-emphasis">Доступных</div>
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
              <v-icon icon="mdi-alert-circle" color="warning" size="large" class="mr-3"></v-icon>
              <div>
                <div class="text-caption text-medium-emphasis">Занятых</div>
                <div class="text-h5 font-weight-bold">{{ busyZonesCount }}</div>
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
                  v-model="filters.zone_type"
                  :items="zoneTypes"
                  label="Тип системы"
                  clearable
                  variant="outlined"
                  density="compact"
                ></v-select>
              </v-col>
              <v-col cols="12" md="4">
                <v-select
                  v-model="filters.status"
                  :items="statusOptions"
                  label="Статус"
                  clearable
                  variant="outlined"
                  density="compact"
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

    <!-- Список зон -->
    <v-row v-if="loading">
      <v-col cols="12">
        <div class="text-center py-8">
          <v-progress-circular indeterminate color="primary" size="64"></v-progress-circular>
          <p class="mt-4 text-medium-emphasis">Загрузка зон...</p>
        </div>
      </v-col>
    </v-row>

    <v-row v-else-if="filteredZones.length === 0">
      <v-col cols="12">
        <v-card variant="outlined">
          <v-card-text class="text-center py-8">
            <v-icon icon="mdi-map-marker-off" size="64" color="grey" class="mb-4"></v-icon>
            <h3 class="mb-2">Нет зон</h3>
            <p class="text-medium-emphasis mb-4">Создайте первую зону для начала работы</p>
            <v-btn
              color="primary"
              prepend-icon="mdi-plus"
              @click="showAddDialog = true"
            >
              Добавить зону
            </v-btn>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row v-else>
      <v-col
        v-for="zone in filteredZones"
        :key="zone.id"
        cols="12"
        md="6"
        lg="4"
      >
        <ZoneCard
          :zone="zone"
          @select="viewZone"
          @view="viewZone"
          @edit="editZone"
          @create-cycle="createCycle"
        />
      </v-col>
    </v-row>

    <!-- Диалог добавления зоны -->
    <AddZoneDialog
      v-model="showAddDialog"
      @created="handleZoneCreated"
    />

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
import { useZonesStore, type Zone } from '@/stores/zones'
import ZoneCard from './ZoneCard.vue'
import AddZoneDialog from './AddZoneDialog.vue'

const router = useRouter()
const zonesStore = useZonesStore()

const loading = ref(false)
const showAddDialog = ref(false)
const searchQuery = ref('')

const filters = ref<{
  zone_type?: string
  status?: string
}>({})

const zoneTypes = [
  { title: 'NFT', value: 'nft' },
  { title: 'DWC', value: 'dwc' },
  { title: 'Капельный полив', value: 'drip' },
  { title: 'Ebb & Flow', value: 'ebb_flow' },
  { title: 'Аэропоника', value: 'aeroponics' },
  { title: 'Другое', value: 'other' },
]

const statusOptions = [
  { title: 'Активные', value: 'active' },
  { title: 'Неактивные', value: 'inactive' },
  { title: 'Доступные', value: 'available' },
  { title: 'Занятые', value: 'busy' },
]

const snackbar = ref({
  show: false,
  message: '',
  color: 'success',
})

const zones = computed(() => zonesStore.zones)
const activeZonesCount = computed(() => zonesStore.activeZones.length)
const availableZonesCount = computed(() => zonesStore.availableZones.length)
const busyZonesCount = computed(() => zonesStore.busyZones.length)

const filteredZones = computed(() => {
  let result = zones.value

  if (filters.value.zone_type) {
    result = result.filter(z => z.zone_type === filters.value.zone_type)
  }

  if (filters.value.status === 'active') {
    result = result.filter(z => z.is_active)
  } else if (filters.value.status === 'inactive') {
    result = result.filter(z => !z.is_active)
  } else if (filters.value.status === 'available') {
    result = result.filter(z => z.is_active && !z.current_cycle_id)
  } else if (filters.value.status === 'busy') {
    result = result.filter(z => z.current_cycle_id !== null)
  }

  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    result = result.filter(z =>
      z.name.toLowerCase().includes(query) ||
      z.location?.toLowerCase().includes(query) ||
      z.description?.toLowerCase().includes(query)
    )
  }

  return result.sort((a, b) => {
    // Активные зоны в начале
    if (a.is_active && !b.is_active) return -1
    if (!a.is_active && b.is_active) return 1
    // Затем по названию
    return a.name.localeCompare(b.name)
  })
})

function viewZone(zone: Zone) {
  router.push({ name: 'ZoneDetail', params: { id: zone.id } })
}

function editZone(zone: Zone) {
  // TODO: Открыть диалог редактирования
  showSnackbar('Редактирование зоны (в разработке)', 'info')
}

function createCycle(zone: Zone) {
  router.push({ name: 'GrowthPlanner', query: { zone_id: zone.id } })
}

function handleZoneCreated(zone: Zone) {
  showSnackbar('Зона успешно создана!', 'success')
  loadZones()
}

async function loadZones() {
  loading.value = true
  try {
    await zonesStore.fetchZones()
  } catch (error) {
    showSnackbar('Ошибка при загрузке зон', 'error')
  } finally {
    loading.value = false
  }
}

function showSnackbar(message: string, color: string = 'success') {
  snackbar.value = { show: true, message, color }
}

onMounted(() => {
  loadZones()
})
</script>

