<template>
  <v-card
    :class="['zone-card', { 'zone-card--active': zone.is_active, 'zone-card--busy': zone.current_cycle_id }]"
    variant="outlined"
    :color="zone.is_active ? 'success' : 'grey'"
    @click="$emit('select', zone)"
  >
    <v-card-title class="d-flex align-center">
      <v-icon :icon="getZoneIcon" :color="getZoneColor" class="mr-2"></v-icon>
      <span>{{ zone.name }}</span>
      <v-spacer></v-spacer>
      <v-chip
        :color="zone.is_active ? 'success' : 'grey'"
        size="small"
        variant="flat"
      >
        {{ zone.is_active ? 'Активна' : 'Неактивна' }}
      </v-chip>
    </v-card-title>

    <v-card-subtitle v-if="zone.description">
      {{ zone.description }}
    </v-card-subtitle>

    <v-card-text>
      <v-row dense>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">Тип системы</div>
          <div class="text-body-2">{{ getZoneTypeText(zone.zone_type) }}</div>
        </v-col>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">Расположение</div>
          <div class="text-body-2">{{ zone.location || 'Не указано' }}</div>
        </v-col>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">Root Node</div>
          <div class="text-body-2">
            <v-chip size="x-small" variant="outlined">
              {{ zone.root_node_id || 'Не назначен' }}
            </v-chip>
          </div>
        </v-col>
        <v-col cols="6">
          <div class="text-caption text-medium-emphasis">Статус</div>
          <div class="text-body-2">
            <v-chip
              :color="zone.current_cycle_id ? 'warning' : 'success'"
              size="small"
              variant="flat"
            >
              {{ zone.current_cycle_id ? 'Занята' : 'Доступна' }}
            </v-chip>
          </div>
        </v-col>
      </v-row>

      <!-- Информация о текущем цикле -->
      <v-alert
        v-if="zone.current_cycle && cycle"
        type="info"
        variant="tonal"
        density="compact"
        class="mt-3"
      >
        <div class="text-caption">
          <strong>Активный цикл:</strong> {{ cycle.name || 'Без названия' }}
        </div>
        <div class="text-caption" v-if="cycle.culture">
          Культура: {{ cycle.culture.name }}
        </div>
      </v-alert>

      <!-- Информация о узлах -->
      <v-expansion-panels v-if="zoneNodes.length > 0" class="mt-3" variant="accordion">
        <v-expansion-panel>
          <v-expansion-panel-title>
            <span>Узлы зоны ({{ zoneNodes.length }})</span>
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-chip
              v-for="node in zoneNodes"
              :key="node.node_id"
              size="small"
              variant="outlined"
              class="mr-2 mb-2"
            >
              <v-icon :icon="node.icon" size="small" class="mr-1"></v-icon>
              {{ node.node_id }}
            </v-chip>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </v-card-text>

    <v-card-actions v-if="showActions">
      <v-btn
        variant="text"
        prepend-icon="mdi-eye"
        @click.stop="$emit('view', zone)"
      >
        Просмотр
      </v-btn>
      <v-spacer></v-spacer>
      <v-btn
        v-if="!zone.current_cycle_id"
        color="success"
        variant="text"
        prepend-icon="mdi-plus"
        @click.stop="$emit('create-cycle', zone)"
      >
        Создать цикл
      </v-btn>
      <v-btn
        variant="text"
        prepend-icon="mdi-pencil"
        @click.stop="$emit('edit', zone)"
      >
        Редактировать
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useZonesStore, type Zone } from '@/stores/zones'
import { useGrowthStore } from '@/stores/growth'

interface Props {
  zone: Zone
  showActions?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  showActions: true,
})

defineEmits<{
  select: [zone: Zone]
  view: [zone: Zone]
  edit: [zone: Zone]
  'create-cycle': [zone: Zone]
}>()

const zonesStore = useZonesStore()
const growthStore = useGrowthStore()

const zoneNodes = computed(() => {
  // Получаем узлы зоны через store
  const nodes = zonesStore.getZoneNodes(props.zone.id) || []
  // Добавляем иконки для узлов
  return nodes.map((node: any) => ({
    ...node,
    icon: getNodeIcon(node.node_type),
  }))
})

function getNodeIcon(nodeType: string): string {
  const icons: Record<string, string> = {
    ph_ec: 'mdi-flask',
    ph: 'mdi-flask-outline',
    ec: 'mdi-flash-outline',
    climate: 'mdi-thermometer',
    relay: 'mdi-electric-switch',
    water: 'mdi-water',
    display: 'mdi-monitor',
    root: 'mdi-server-network',
  }
  return icons[nodeType] || 'mdi-chip'
}

const cycle = computed(() => {
  if (!props.zone.current_cycle_id) return null
  return growthStore.getCycleById(props.zone.current_cycle_id)
})

const getZoneIcon = computed(() => {
  const icons: Record<string, string> = {
    nft: 'mdi-pipe',
    dwc: 'mdi-water',
    drip: 'mdi-water-drop',
    ebb_flow: 'mdi-water-sync',
    aeroponics: 'mdi-spray',
    other: 'mdi-flower',
  }
  return icons[props.zone.zone_type] || 'mdi-flower'
})

const getZoneColor = computed(() => {
  if (!props.zone.is_active) return 'grey'
  if (props.zone.current_cycle_id) return 'warning'
  return 'success'
})

function getZoneTypeText(type: string): string {
  const types: Record<string, string> = {
    nft: 'NFT',
    dwc: 'DWC',
    drip: 'Капельный полив',
    ebb_flow: 'Ebb & Flow',
    aeroponics: 'Аэропоника',
    other: 'Другое',
  }
  return types[type] || type
}
</script>

<style scoped>
.zone-card {
  transition: all 0.2s;
  cursor: pointer;
}

.zone-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
}

.zone-card--active {
  border-color: rgb(var(--v-theme-success));
}

.zone-card--busy {
  border-color: rgb(var(--v-theme-warning));
}
</style>

