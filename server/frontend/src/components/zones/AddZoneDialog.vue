<template>
  <v-dialog
    v-model="dialog"
    max-width="800"
    persistent
    scrollable
  >
    <v-card>
      <v-card-title class="d-flex align-center">
        <v-icon icon="mdi-map-marker-plus" color="primary" class="mr-2"></v-icon>
        <span>Добавить новую зону</span>
        <v-spacer></v-spacer>
        <v-btn
          icon="mdi-close"
          variant="text"
          @click="closeDialog"
        ></v-btn>
      </v-card-title>

      <v-divider></v-divider>

      <v-card-text>
        <v-form ref="form" v-model="valid">
          <!-- Основная информация -->
          <h3 class="mb-4">Основная информация</h3>
          
          <v-text-field
            v-model="formData.name"
            label="Название зоны *"
            variant="outlined"
            density="comfortable"
            prepend-inner-icon="mdi-format-title"
            :rules="[v => !!v || 'Название обязательно']"
            :error-messages="errors.name"
            class="mb-3"
          ></v-text-field>

          <v-textarea
            v-model="formData.description"
            label="Описание"
            variant="outlined"
            density="comfortable"
            prepend-inner-icon="mdi-text"
            rows="2"
            class="mb-3"
          ></v-textarea>

          <v-text-field
            v-model="formData.location"
            label="Расположение"
            variant="outlined"
            density="comfortable"
            prepend-inner-icon="mdi-map-marker"
            class="mb-3"
          ></v-text-field>

          <!-- Тип системы -->
          <h3 class="mb-4 mt-4">Тип системы</h3>

          <v-select
            v-model="formData.zone_type"
            :items="zoneTypes"
            label="Тип системы *"
            variant="outlined"
            density="comfortable"
            prepend-inner-icon="mdi-water"
            :rules="[v => !!v || 'Тип системы обязателен']"
            :error-messages="errors.zone_type"
            class="mb-3"
          ></v-select>

          <!-- Root Node -->
          <h3 class="mb-4 mt-4">Root Node</h3>

          <v-select
            v-model="formData.root_node_id"
            :items="availableRootNodes"
            label="Root Node *"
            item-title="node_id"
            item-value="node_id"
            variant="outlined"
            density="comfortable"
            prepend-inner-icon="mdi-server-network"
            :rules="[v => !!v || 'Root Node обязателен']"
            :error-messages="errors.root_node_id"
            :loading="loadingNodes"
            class="mb-3"
          >
            <template v-slot:item="{ props, item }">
              <v-list-item v-bind="props">
                <template v-slot:prepend>
                  <v-icon icon="mdi-server-network" color="primary"></v-icon>
                </template>
                <v-list-item-title>{{ item.raw.node_id }}</v-list-item-title>
                <v-list-item-subtitle>
                  {{ item.raw.online ? 'Онлайн' : 'Офлайн' }}
                </v-list-item-subtitle>
              </v-list-item>
            </template>
          </v-select>

          <!-- Mesh Network ID -->
          <v-text-field
            v-model="formData.mesh_network_id"
            label="Mesh Network ID *"
            variant="outlined"
            density="comfortable"
            prepend-inner-icon="mdi-wifi"
            hint="Уникальный SSID для mesh сети (например: HYDRO1_ZONE1)"
            persistent-hint
            :rules="[v => !!v || 'Mesh Network ID обязателен']"
            :error-messages="errors.mesh_network_id"
            class="mb-3"
          ></v-text-field>

          <!-- MQTT Topic Prefix -->
          <v-text-field
            v-model="formData.mqtt_topic_prefix"
            label="MQTT Topic Prefix *"
            variant="outlined"
            density="comfortable"
            prepend-inner-icon="mdi-message-text"
            hint="Префикс для MQTT топиков (например: hydro/zone1/)"
            persistent-hint
            :rules="[v => !!v || 'MQTT Topic Prefix обязателен']"
            :error-messages="errors.mqtt_topic_prefix"
            class="mb-3"
          ></v-text-field>

          <!-- Характеристики -->
          <h3 class="mb-4 mt-4">Характеристики</h3>

          <v-row dense>
            <v-col cols="12" md="6">
              <v-text-field
                v-model.number="formData.reservoir_volume_liters"
                label="Объем резервуара (л)"
                type="number"
                variant="outlined"
                density="comfortable"
                prepend-inner-icon="mdi-water"
                class="mb-3"
              ></v-text-field>
            </v-col>
            <v-col cols="12" md="6">
              <v-text-field
                v-model.number="formData.growing_area_m2"
                label="Площадь выращивания (м²)"
                type="number"
                variant="outlined"
                density="comfortable"
                prepend-inner-icon="mdi-ruler"
                class="mb-3"
              ></v-text-field>
            </v-col>
            <v-col cols="12" md="6">
              <v-text-field
                v-model.number="formData.plant_capacity"
                label="Вместимость растений"
                type="number"
                variant="outlined"
                density="comfortable"
                prepend-inner-icon="mdi-sprout"
                class="mb-3"
              ></v-text-field>
            </v-col>
          </v-row>

          <!-- Статус -->
          <v-switch
            v-model="formData.is_active"
            label="Зона активна"
            color="success"
            class="mb-3"
          ></v-switch>

          <v-switch
            v-model="formData.is_available"
            label="Доступна для новых циклов"
            color="info"
            class="mb-3"
          ></v-switch>
        </v-form>
      </v-card-text>

      <v-divider></v-divider>

      <v-card-actions>
        <v-spacer></v-spacer>
        <v-btn
          variant="text"
          @click="closeDialog"
        >
          Отмена
        </v-btn>
        <v-btn
          color="primary"
          prepend-icon="mdi-check"
          @click="createZone"
          :loading="creating"
          :disabled="!valid"
        >
          Создать зону
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { useZonesStore, type Zone } from '@/stores/zones'
import { useNodesStore } from '@/stores/nodes'

interface Props {
  modelValue: boolean
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  'created': [zone: Zone]
}>()

const zonesStore = useZonesStore()
const nodesStore = useNodesStore()

const dialog = computed({
  get: () => props.modelValue,
  set: (value) => emit('update:modelValue', value)
})

const valid = ref(false)
const creating = ref(false)
const loadingNodes = ref(false)

const formData = ref<{
  name: string
  description?: string
  location?: string
  root_node_id?: string
  mesh_network_id: string
  mqtt_topic_prefix: string
  zone_type: string
  reservoir_volume_liters?: number
  growing_area_m2?: number
  plant_capacity?: number
  is_active: boolean
  is_available: boolean
}>({
  name: '',
  description: '',
  location: '',
  root_node_id: undefined,
  mesh_network_id: '',
  mqtt_topic_prefix: '',
  zone_type: 'nft',
  reservoir_volume_liters: undefined,
  growing_area_m2: undefined,
  plant_capacity: undefined,
  is_active: true,
  is_available: true,
})

const errors = ref<Record<string, string[]>>({})

const zoneTypes = [
  { title: 'NFT', value: 'nft' },
  { title: 'DWC', value: 'dwc' },
  { title: 'Капельный полив', value: 'drip' },
  { title: 'Ebb & Flow', value: 'ebb_flow' },
  { title: 'Аэропоника', value: 'aeroponics' },
  { title: 'Другое', value: 'other' },
]

const availableRootNodes = computed(() => {
  // Получаем только Root Nodes, которые еще не назначены зонам
  const rootNodes = nodesStore.nodes.filter((n: any) => n.node_type === 'root')
  const usedRootNodeIds = zonesStore.zones
    .map(z => z.root_node_id)
    .filter(Boolean)
  
  return rootNodes.filter((n: any) => !usedRootNodeIds.includes(n.node_id))
})

watch(dialog, (isOpen) => {
  if (isOpen) {
    resetForm()
    loadRootNodes()
  }
})

function resetForm() {
  formData.value = {
    name: '',
    description: '',
    location: '',
    root_node_id: undefined,
    mesh_network_id: '',
    mqtt_topic_prefix: '',
    zone_type: 'nft',
    reservoir_volume_liters: undefined,
    growing_area_m2: undefined,
    plant_capacity: undefined,
    is_active: true,
    is_available: true,
  }
  errors.value = {}
}

async function loadRootNodes() {
  loadingNodes.value = true
  try {
    await nodesStore.fetchNodes()
  } finally {
    loadingNodes.value = false
  }
}

async function createZone() {
  if (!valid.value) return

  creating.value = true
  errors.value = {}

  try {
    const zone = await zonesStore.createZone(formData.value)
    emit('created', zone)
    closeDialog()
  } catch (error: any) {
    if (error.response?.data?.errors) {
      errors.value = error.response.data.errors
    } else {
      errors.value = { general: [error.response?.data?.message || 'Ошибка при создании зоны'] }
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
    loadRootNodes()
  }
})
</script>

