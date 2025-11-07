<template>
  <div class="zone-selector">
    <v-select
      v-model="selectedZoneId"
      :items="availableZones"
      :label="label"
      :hint="hint"
      :error-messages="errorMessages"
      :disabled="disabled || loading"
      :loading="loading"
      item-title="name"
      item-value="id"
      return-object
      variant="outlined"
      density="comfortable"
      prepend-inner-icon="mdi-map-marker"
      @update:model-value="handleSelection"
    >
      <template v-slot:item="{ props: itemProps, item }">
        <v-list-item v-bind="itemProps">
          <template v-slot:prepend>
            <v-icon
              :icon="getZoneIcon(item.raw.zone_type)"
              :color="getZoneColor(item.raw.zone_type)"
            ></v-icon>
          </template>
          <v-list-item-title>{{ item.raw.name }}</v-list-item-title>
          <v-list-item-subtitle>
            <div class="d-flex align-center mt-1">
              <v-chip
                v-if="item.raw.current_cycle"
                size="x-small"
                color="warning"
                variant="flat"
                class="mr-2"
              >
                Занята
              </v-chip>
              <v-chip
                v-else
                size="x-small"
                color="success"
                variant="flat"
                class="mr-2"
              >
                Доступна
              </v-chip>
              <span class="text-caption">{{ item.raw.zone_type.toUpperCase() }}</span>
              <v-spacer></v-spacer>
              <span class="text-caption text-medium-emphasis">
                {{ item.raw.root_node_id }}
              </span>
            </div>
          </v-list-item-subtitle>
        </v-list-item>
      </template>

      <template v-slot:selection="{ item }">
        <div class="d-flex align-center">
          <v-icon
            :icon="getZoneIcon(item.raw.zone_type)"
            :color="getZoneColor(item.raw.zone_type)"
            class="mr-2"
          ></v-icon>
          <span>{{ item.raw.name }}</span>
          <v-chip
            v-if="item.raw.current_cycle"
            size="x-small"
            color="warning"
            variant="flat"
            class="ml-2"
          >
            Занята
          </v-chip>
        </div>
      </template>
    </v-select>

    <!-- Детали выбранной зоны -->
    <v-card
      v-if="selectedZone"
      variant="outlined"
      class="mt-3"
    >
      <v-card-text>
        <div class="d-flex align-center mb-2">
          <v-icon
            :icon="getZoneIcon(selectedZone.zone_type)"
            :color="getZoneColor(selectedZone.zone_type)"
            class="mr-2"
          ></v-icon>
          <span class="text-subtitle-1 font-weight-medium">{{ selectedZone.name }}</span>
        </div>

        <v-row dense>
          <v-col cols="6">
            <div class="text-caption text-medium-emphasis">Тип системы</div>
            <div class="text-body-2">{{ getZoneTypeText(selectedZone.zone_type) }}</div>
          </v-col>
          <v-col cols="6">
            <div class="text-caption text-medium-emphasis">Root Node</div>
            <div class="text-body-2 font-weight-medium">{{ selectedZone.root_node_id }}</div>
          </v-col>
          <v-col cols="6" v-if="selectedZone.reservoir_volume_liters">
            <div class="text-caption text-medium-emphasis">Объём резервуара</div>
            <div class="text-body-2">{{ selectedZone.reservoir_volume_liters }} л</div>
          </v-col>
          <v-col cols="6" v-if="selectedZone.growing_area_m2">
            <div class="text-caption text-medium-emphasis">Площадь</div>
            <div class="text-body-2">{{ selectedZone.growing_area_m2 }} м²</div>
          </v-col>
        </v-row>

        <v-alert
          v-if="selectedZone.current_cycle"
          type="warning"
          density="compact"
          class="mt-3"
        >
          В этой зоне уже есть активный цикл роста
        </v-alert>
        <v-alert
          v-else
          type="success"
          density="compact"
          class="mt-3"
        >
          Зона доступна для нового цикла
        </v-alert>
      </v-card-text>
    </v-card>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { useZonesStore, type Zone } from '@/stores/zones'

interface Props {
  modelValue?: Zone | null
  label?: string
  hint?: string
  errorMessages?: string[]
  disabled?: boolean
  excludeZoneIds?: number[]
}

const props = withDefaults(defineProps<Props>(), {
  label: 'Выберите зону',
  hint: 'Зона, в которой будет проходить цикл роста',
  disabled: false,
  excludeZoneIds: () => [],
})

const emit = defineEmits<{
  'update:modelValue': [zone: Zone | null]
}>()

const zonesStore = useZonesStore()
const selectedZoneId = ref<Zone | null>(props.modelValue || null)
const loading = computed(() => zonesStore.loading)

const availableZones = computed(() => {
  let zones = zonesStore.availableZones
  
  if (props.excludeZoneIds.length > 0) {
    zones = zones.filter(z => !props.excludeZoneIds.includes(z.id))
  }
  
  return zones
})

const selectedZone = computed(() => selectedZoneId.value)

watch(() => props.modelValue, (newValue) => {
  selectedZoneId.value = newValue || null
})

function handleSelection(zone: Zone | null) {
  selectedZoneId.value = zone
  emit('update:modelValue', zone)
}

function getZoneIcon(zoneType: string): string {
  const map: Record<string, string> = {
    nft: 'mdi-pipe',
    dwc: 'mdi-water',
    drip: 'mdi-water-droplet',
    ebb_flow: 'mdi-water-sync',
    aeroponics: 'mdi-spray',
    other: 'mdi-cube-outline',
  }
  return map[zoneType] || 'mdi-cube-outline'
}

function getZoneColor(zoneType: string): string {
  const map: Record<string, string> = {
    nft: 'primary',
    dwc: 'info',
    drip: 'success',
    ebb_flow: 'warning',
    aeroponics: 'error',
    other: 'grey',
  }
  return map[zoneType] || 'grey'
}

function getZoneTypeText(zoneType: string): string {
  const map: Record<string, string> = {
    nft: 'NFT (Nutrient Film Technique)',
    dwc: 'DWC (Deep Water Culture)',
    drip: 'Капельный полив',
    ebb_flow: 'Ebb & Flow',
    aeroponics: 'Аэропоника',
    other: 'Другое',
  }
  return map[zoneType] || zoneType
}

onMounted(async () => {
  if (zonesStore.zones.length === 0) {
    await zonesStore.fetchZones()
  }
})
</script>

<style scoped>
.zone-selector {
  width: 100%;
}
</style>

