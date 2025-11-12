<template>
  <div class="zone-selector d-flex align-center">
    <v-select
      v-model="model"
      :items="options"
      :loading="zonesStore.loading"
      :disabled="zonesStore.loading"
      :clearable="allowClear"
      item-title="label"
      item-value="value"
      label="Зона"
      variant="outlined"
      density="compact"
      hide-details="auto"
      class="zone-selector__field"
      prepend-inner-icon="mdi-map-marker-radius"
      clear-icon="mdi-close-circle"
    >
      <template #selection="{ item }">
        <div class="d-flex align-center">
          <span class="mr-2">{{ item.title }}</span>
          <v-chip
            v-if="item.raw?.nodes_online !== undefined"
            size="x-small"
            color="success"
            class="text-caption text-uppercase"
            variant="flat"
          >
            {{ item.raw.nodes_online }}/{{ item.raw.nodes_total }}
          </v-chip>
        </div>
      </template>
      <template #item="{ props, item }">
        <v-list-item v-bind="props">
          <template #append>
            <v-chip
              v-if="item.raw?.nodes_online !== undefined"
              size="x-small"
              color="success"
              variant="tonal"
              class="text-caption text-uppercase"
            >
              {{ item.raw.nodes_online }}/{{ item.raw.nodes_total }}
            </v-chip>
          </template>
        </v-list-item>
      </template>
    </v-select>

    <v-btn
      v-if="showRefresh"
      icon="mdi-refresh"
      size="small"
      variant="text"
      class="ml-1"
      :loading="zonesStore.loading"
      @click="refreshZones"
    ></v-btn>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, watch } from 'vue'
import { useZonesStore } from '@/stores/zones'

const props = withDefaults(
  defineProps<{
    allowClear?: boolean
    showRefresh?: boolean
  }>(),
  {
    allowClear: true,
    showRefresh: false,
  },
)

const emit = defineEmits<{
  (event: 'change', zone: string | null): void
  (event: 'loaded'): void
}>()

const zonesStore = useZonesStore()

const options = computed(() => {
  const items = zonesStore.zoneOptions.map((zone) => ({
    value: zone.value,
    label: zone.label,
    nodes_total: zone.nodes_total,
    nodes_online: zone.nodes_online,
  }))

  if (props.allowClear) {
    return [
      {
        value: null,
        label: 'Все зоны',
        nodes_total: items.reduce((sum, z) => sum + (z.nodes_total ?? 0), 0),
        nodes_online: items.reduce((sum, z) => sum + (z.nodes_online ?? 0), 0),
      },
      ...items,
    ]
  }

  return items
})

const model = computed({
  get: () => zonesStore.selectedZone,
  set: (value: string | null) => {
    zonesStore.selectZone(value)
  },
})

async function refreshZones() {
  try {
    await zonesStore.fetchZones(true)
    emit('loaded')
  } catch (error) {
    console.error('[ZoneSelector] refreshZones failed', error)
  }
}

onMounted(async () => {
  if (!zonesStore.hasZones) {
    try {
      await zonesStore.fetchZones()
      emit('loaded')
    } catch (error) {
      console.error('[ZoneSelector] initial fetch failed', error)
    }
  }
})

watch(
  () => zonesStore.selectedZone,
  (zone) => {
    emit('change', zone ?? null)
  },
)
</script>

<style scoped>
.zone-selector__field {
  min-width: 180px;
  max-width: 260px;
}
</style>

