<template>
  <v-container fluid class="greenhouses-page">
    <v-row>
      <v-col cols="12">
        <v-card variant="outlined" class="pa-4">
          <div class="d-flex flex-column flex-md-row align-md-center justify-space-between ga-4">
            <div>
              <div class="text-h5 font-weight-medium d-flex align-center ga-2">
                <v-icon color="primary" icon="mdi-leaf-circle"></v-icon>
                Управление теплицами
              </div>
              <div class="text-caption text-medium-emphasis mt-1">
                Статус: {{ greenhousesStore.loading ? 'загрузка...' : `${greenhouseList.length} активных записей` }}
              </div>
            </div>

            <div class="d-flex align-center ga-2 flex-wrap">
              <v-btn
                color="primary"
                prepend-icon="mdi-plus"
                @click="openWizard"
              >
                Добавить теплицу
              </v-btn>
              <v-btn
                variant="tonal"
                prepend-icon="mdi-refresh"
                :loading="refreshing"
                @click="refreshAll"
              >
                Обновить данные
              </v-btn>
            </div>
          </div>
        </v-card>
      </v-col>
    </v-row>

    <v-row v-if="!greenhouseList.length && !greenhousesStore.loading">
      <v-col cols="12">
        <v-alert type="info" variant="tonal" border="start">
          Теплицы пока не добавлены. Нажмите «Добавить теплицу», чтобы создать первую.
        </v-alert>
      </v-col>
    </v-row>

    <v-row v-else>
      <v-col cols="12" md="4">
        <v-card variant="outlined" class="pa-0">
          <v-list nav>
            <v-list-item
              v-for="item in greenhouseList"
              :key="item.id"
              :value="item.id"
              :active="selectedId === item.id"
              @click="selectGreenhouse(item.id)"
            >
              <v-list-item-title>{{ item.name }}</v-list-item-title>
              <v-list-item-subtitle class="text-caption text-medium-emphasis">
                {{ item.description || 'Описание не указано' }}
              </v-list-item-subtitle>
              <template #append>
                <v-chip
                  size="small"
                  :color="item.status === 'active' ? 'success' : item.status === 'maintenance' ? 'warning' : 'info'"
                  variant="tonal"
                >
                  {{ statusLabel(item.status) }}
                </v-chip>
              </template>
            </v-list-item>
          </v-list>
        </v-card>
      </v-col>

      <v-col cols="12" md="8">
        <template v-if="selectedGreenhouse">
          <v-card variant="outlined" class="pa-4 mb-4">
            <div class="d-flex flex-column flex-lg-row ga-4">
              <v-sheet class="pa-4 rounded-xl flex-grow-1" color="surface-variant">
                <div class="text-subtitle-2 text-medium-emphasis mb-1">Общая информация</div>
                <div class="text-body-1 font-weight-medium">{{ selectedGreenhouse.name }}</div>
                <div class="text-caption text-medium-emphasis mt-2">
                  Код: {{ selectedGreenhouse.code }} ·
                  Причислено зон: {{ selectedGreenhouse.zone_count }} ·
                  Узлов: {{ selectedGreenhouse.node_count }}
                </div>
                <div class="text-caption text-medium-emphasis mt-2">
                  Описание: {{ selectedGreenhouse.description || 'не указано' }}
                </div>
                <div class="text-caption text-medium-emphasis mt-1">
                  Профили климата:
                  <template v-if="selectedGreenhouse.climate_profiles?.length">
                    {{ selectedGreenhouse.climate_profiles.map((profile) => profile.name).join(', ') }}
                  </template>
                  <template v-else>
                    не заданы
                  </template>
                </div>
              </v-sheet>

              <v-sheet class="pa-4 rounded-xl flex-grow-1" variant="outlined">
                <div class="text-subtitle-2 text-medium-emphasis mb-2">Сводка</div>
                <div class="d-flex flex-wrap ga-3">
                  <v-chip color="primary" variant="tonal" prepend-icon="mdi-map-marker-radius">
                    Зон: {{ zonesForSelected.length }}
                  </v-chip>
                  <v-chip color="success" variant="tonal" prepend-icon="mdi-access-point-network">
                    Узлов: {{ nodesForSelected.length }}
                  </v-chip>
                  <v-chip color="info" variant="tonal" prepend-icon="mdi-sprout">
                    Активных циклов: {{ activeCycles.length }}
                  </v-chip>
                </div>
              </v-sheet>
            </div>
          </v-card>

          <v-row>
            <v-col cols="12" lg="6">
              <v-card variant="outlined" class="pa-4 h-100">
                <div class="d-flex align-center mb-3 ga-2">
                  <v-icon icon="mdi-map-marker" color="primary"></v-icon>
                  <div class="text-subtitle-1">Зоны</div>
                </div>

                <template v-if="zonesForSelected.length">
                  <v-list density="comfortable">
                    <v-list-item
                      v-for="zone in zonesForSelected"
                      :key="zone.id"
                    >
                      <v-list-item-title>{{ zone.name }}</v-list-item-title>
                      <v-list-item-subtitle class="text-caption text-medium-emphasis">
                        {{ zoneTypeLabel(zone.zone_type) }} ·
                        {{ zone.is_active ? 'активна' : 'отключена' }}
                      </v-list-item-subtitle>
                      <template #append>
                        <v-chip
                          size="small"
                          :color="zone.current_cycle_id ? 'success' : 'default'"
                          variant="outlined"
                        >
                          {{ zone.current_cycle_id ? `Цикл ${zone.current_cycle_id}` : 'без цикла' }}
                        </v-chip>
                      </template>
                    </v-list-item>
                  </v-list>
                </template>
                <v-alert
                  v-else
                  type="info"
                  variant="tonal"
                  border="start"
                  class="mb-0"
                >
                  Для теплицы пока не назначено зон.
                </v-alert>
              </v-card>
            </v-col>

            <v-col cols="12" lg="6">
              <v-card variant="outlined" class="pa-4 h-100">
                <div class="d-flex align-center mb-3 ga-2">
                  <v-icon icon="mdi-access-point-network" color="primary"></v-icon>
                  <div class="text-subtitle-1">Узлы</div>
                </div>

                <template v-if="nodesForSelected.length">
                  <v-list density="comfortable">
                    <v-list-item
                      v-for="node in nodesForSelected"
                      :key="node.node_id"
                    >
                      <v-list-item-title>{{ node.node_id }}</v-list-item-title>
                      <v-list-item-subtitle class="text-caption text-medium-emphasis">
                        {{ node.node_type }} · {{ node.zone || 'без зоны' }}
                      </v-list-item-subtitle>
                      <template #prepend>
                        <v-icon
                          :color="node.online || node.is_online ? 'success' : 'warning'"
                          icon="mdi-circle-medium"
                        ></v-icon>
                      </template>
                    </v-list-item>
                  </v-list>
                </template>

                <v-alert
                  v-else
                  type="info"
                  variant="tonal"
                  border="start"
                  class="mb-0"
                >
                  У теплицы пока нет привязанных узлов.
                </v-alert>
              </v-card>
            </v-col>
          </v-row>

          <v-card variant="outlined" class="pa-4 mt-4">
            <div class="d-flex align-center mb-3 ga-2">
              <v-icon icon="mdi-sprout" color="primary"></v-icon>
              <div class="text-subtitle-1">Активные циклы</div>
            </div>

            <template v-if="activeCycles.length">
              <v-list density="comfortable">
                <v-list-item
                  v-for="cycle in activeCycles"
                  :key="cycle.id"
                >
                  <v-list-item-title>
                    Цикл #{{ cycle.id }} · {{ cycle.preset?.name || 'пресет не указан' }}
                  </v-list-item-title>
                  <v-list-item-subtitle class="text-caption text-medium-emphasis">
                    Зона: {{ findZoneName(cycle.zone_id) }} · Начат: {{ formatDate(cycle.started_at) }}
                  </v-list-item-subtitle>
                </v-list-item>
              </v-list>
            </template>
            <v-alert
              v-else
              type="info"
              variant="tonal"
              border="start"
              class="mb-0"
            >
              В теплице нет активных циклов.
            </v-alert>
          </v-card>
        </template>

        <v-skeleton-loader
          v-else
          type="article"
          class="mt-4"
        ></v-skeleton-loader>
      </v-col>
    </v-row>

    <GreenhouseWizard
      v-model="wizardOpen"
      @created="handleCreated"
    />
  </v-container>
</template>

<script setup lang="ts">
import { computed, ref, onMounted, watch } from 'vue'
import GreenhouseWizard from '@/components/greenhouse/GreenhouseWizard.vue'
import { useGreenhousesStore } from '@/stores/greenhouses'
import { useZonesStore } from '@/stores/zones'
import { useNodesStore } from '@/stores/nodes'
import { useGrowthStore } from '@/stores/growth'
import { useSetupStore } from '@/stores/setup'
import { usePopup } from '@/composables/usePopup'
import { formatDistanceToNow } from '@/utils/time'

const greenhousesStore = useGreenhousesStore()
const zonesStore = useZonesStore()
const nodesStore = useNodesStore()
const growthStore = useGrowthStore()
const setupStore = useSetupStore()
const popup = usePopup()

const wizardOpen = ref(false)
const refreshing = ref(false)

const greenhouseList = computed(() => greenhousesStore.items)
const selectedId = computed(() => greenhousesStore.selectedId ?? null)
const selectedGreenhouse = computed(() => greenhousesStore.selectedGreenhouse)

const zonesForSelected = computed(() =>
  selectedId.value != null ? zonesStore.getZonesByGreenhouse(selectedId.value) : [],
)

const nodesForSelected = computed(() =>
  selectedId.value != null
    ? nodesStore.nodes.filter((node: any) => node.greenhouse_id === selectedId.value)
    : [],
)

const activeCycles = computed(() =>
  selectedId.value != null
    ? growthStore.cycles.filter((cycle) => cycle.greenhouse_id === selectedId.value)
    : [],
)

watch(
  selectedId,
  (value) => {
    setupStore.setTargetGreenhouseId(value ?? null)
    if (value != null) {
      greenhousesStore.fetchGreenhouse(value, { refresh: true }).catch(() => {})
      greenhousesStore.fetchZones(value, { refresh: true }).catch(() => {})
      greenhousesStore.fetchNodes(value, { refresh: true }).catch(() => {})
    }
  },
  { immediate: true },
)

async function refreshAll() {
  refreshing.value = true
  try {
    await Promise.all([
      greenhousesStore.fetchGreenhouses({ refresh: true }),
      zonesStore.fetchZones(),
      nodesStore.fetchNodes(),
      growthStore.fetchCycles(),
    ])
    if (selectedId.value != null) {
      await Promise.all([
        greenhousesStore.fetchGreenhouse(selectedId.value, { refresh: true }),
        greenhousesStore.fetchZones(selectedId.value, { refresh: true }),
        greenhousesStore.fetchNodes(selectedId.value, { refresh: true }),
      ])
    }
    popup.toast.success('Данные обновлены')
  } catch (error: any) {
    console.error('Не удалось обновить данные теплиц', error)
    popup.toast.error(error?.response?.data?.message || 'Ошибка обновления данных')
  } finally {
    refreshing.value = false
  }
}

function openWizard() {
  wizardOpen.value = true
}

function selectGreenhouse(id: number) {
  greenhousesStore.selectGreenhouse(id)
}

function statusLabel(status: string) {
  switch (status) {
    case 'active':
      return 'Активна'
    case 'maintenance':
      return 'Сервис'
    case 'draft':
      return 'Черновик'
    default:
      return status
  }
}

function zoneTypeLabel(value: string) {
  const map: Record<string, string> = {
    nft: 'NFT',
    dwc: 'DWC',
    drip: 'Капельный полив',
    ebb_flow: 'Ebb & Flow',
    aeroponics: 'Аэропоника',
  }
  return map[value] || value
}

function findZoneName(zoneId: number) {
  const zone = zonesStore.getZoneById(zoneId)
  return zone?.name || `Зона ${zoneId}`
}

function formatDate(date: string) {
  if (!date) {
    return '—'
  }
  return formatDistanceToNow(date)
}

function handleCreated() {
  // greenhouseStore обновляется внутри мастера, дополнительной логики не требуется
}

onMounted(async () => {
  try {
    await Promise.all([
      greenhousesStore.fetchGreenhouses(),
      zonesStore.fetchZones(),
      nodesStore.fetchNodes(),
      growthStore.fetchCycles(),
    ])
  } catch (error: any) {
    console.warn('Не удалось загрузить данные теплиц', error)
  }
})
</script>

<style scoped>
.greenhouses-page {
  min-height: 100%;
}

.ga-2 {
  gap: 8px;
}

.ga-4 {
  gap: 16px;
}

.h-100 {
  height: 100%;
}
</style>


