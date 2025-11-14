<template>
  <v-dialog v-model="dialog" max-width="800" scrollable persistent>
    <template v-slot:activator="{ props }">
      <slot name="activator" :props="props"></slot>
    </template>

    <v-card>
      <v-card-title class="bg-primary">
        <v-icon icon="mdi-plus-circle" class="mr-2"></v-icon>
        Добавить новый узел
      </v-card-title>

      <v-card-text class="pt-4">
        <v-form ref="form" v-model="valid">
          <v-stepper v-model="step" alt-labels>
            <v-stepper-header>
              <v-stepper-item
                title="Тип узла"
                :value="1"
                :complete="step > 1"
              ></v-stepper-item>
              <v-divider></v-divider>
              <v-stepper-item
                title="Основная информация"
                :value="2"
                :complete="step > 2"
              ></v-stepper-item>
              <v-divider></v-divider>
              <v-stepper-item
                title="Конфигурация"
                :value="3"
                :complete="step > 3"
              ></v-stepper-item>
              <v-divider></v-divider>
              <v-stepper-item
                title="Подтверждение"
                :value="4"
              ></v-stepper-item>
            </v-stepper-header>

            <v-stepper-window>
              <!-- Step 1: Node Type -->
              <v-stepper-window-item :value="1">
                <v-card flat>
                  <v-card-text>
                    <h3 class="mb-4">Выберите тип узла</h3>
                    
                    <v-alert
                      v-if="showDeprecatedPhEcAlert"
                      type="warning"
                      variant="tonal"
                      density="compact"
                      class="mb-4"
                    >
                      Комбинированные узлы pH/EC больше не поддерживаются. Используйте отдельные узлы pH и EC.
                    </v-alert>

                    <v-row>
                      <v-col
                        v-for="type in nodeTypes"
                        :key="type.value"
                        cols="12"
                        sm="6"
                        md="4"
                      >
                        <v-card
                          :class="{
                            'border-primary': newNode.node_type === type.value,
                            'opacity-50': type.disabled,
                          }"
                          :variant="newNode.node_type === type.value ? 'tonal' : 'outlined'"
                          :color="newNode.node_type === type.value ? 'primary' : ''"
                          :style="type.disabled ? 'cursor: not-allowed' : 'cursor: pointer'"
                          @click="selectNodeType(type)"
                          :hover="!type.disabled"
                        >
                          <v-card-text class="text-center">
                            <v-icon :icon="type.icon" size="64"></v-icon>
                            <div class="text-h6 mt-2">{{ type.label }}</div>
                            <div class="text-caption">{{ type.description }}</div>
                          </v-card-text>
                        </v-card>
                      </v-col>
                    </v-row>
                  </v-card-text>
                </v-card>
              </v-stepper-window-item>

              <!-- Step 2: Basic Info -->
              <v-stepper-window-item :value="2">
                <v-card flat>
                  <v-card-text>
                    <h3 class="mb-4">Основная информация</h3>

                    <v-text-field
                      v-model="newNode.node_id"
                      label="ID узла *"
                      variant="outlined"
                      :rules="[rules.required, rules.nodeId]"
                      hint="Пример: ph_002, ec_002, climate_001"
                      persistent-hint
                      clearable
                      autofocus
                      placeholder="001"
                    ></v-text-field>
                    
                    <v-alert
                      v-if="newNode.node_type && !newNode.node_id"
                      type="info"
                      variant="tonal"
                      density="compact"
                      class="mt-2"
                    >
                      Рекомендуемый ID: <strong>{{ suggestedPrefix }}001</strong>, {{ suggestedPrefix }}002, и т.д.
                    </v-alert>
                    
                    <!-- Debug info -->
                    <v-alert
                      v-if="newNode.node_id"
                      type="success"
                      variant="tonal"
                      density="compact"
                      class="mt-2"
                    >
                      ID узла: <strong>{{ newNode.node_id }}</strong>
                    </v-alert>

                    <v-select
                      v-model="newNode.zone"
                      :items="zoneItems"
                      item-title="title"
                      item-value="value"
                      label="Зона *"
                      variant="outlined"
                      :rules="[rules.required]"
                      class="mt-4"
                      clearable
                      @update:model-value="handleZoneChange"
                    >
                      <template v-slot:append>
                        <v-btn
                          icon="mdi-plus"
                          size="small"
                          variant="text"
                          @click="addZoneDialog = true"
                        ></v-btn>
                      </template>
                      <template #selection="{ item }">
                        <span class="text-body-2">
                          {{ item?.title ?? item?.raw?.title ?? newNode.zone }}
                        </span>
                      </template>
                      <template #item="{ props, item }">
                        <v-list-item
                          v-bind="props"
                          :title="item?.raw?.title ?? item?.title ?? props.value"
                          :subtitle="item?.raw?.subtitle ?? item?.subtitle ?? ''"
                        >
                          <template v-if="item?.raw?.subtitle ?? item?.subtitle" #subtitle>
                            <span class="text-caption text-medium-emphasis">
                              {{ item?.raw?.subtitle ?? item?.subtitle }}
                            </span>
                          </template>
                        </v-list-item>
                      </template>
                    </v-select>
                    
                    <v-alert
                      v-if="!newNode.zone"
                      type="info"
                      variant="tonal"
                      density="compact"
                      class="mt-2"
                    >
                      {{ hasZoneItems ? 'Выберите зону или создайте новую (+)' : 'Зоны не найдены — добавьте новую (+)' }}
                    </v-alert>

                    <v-text-field
                      v-model="newNode.mac_address"
                      label="MAC адрес (опционально)"
                      variant="outlined"
                      :rules="[rules.macAddress]"
                      hint="Формат: AA:BB:CC:DD:EE:FF"
                      persistent-hint
                      class="mt-4"
                      placeholder="AA:BB:CC:DD:EE:FF"
                    ></v-text-field>

                    <v-textarea
                      v-model="newNode.description"
                      label="Описание (опционально)"
                      variant="outlined"
                      rows="2"
                      class="mt-4"
                    ></v-textarea>
                  </v-card-text>
                </v-card>
              </v-stepper-window-item>

              <!-- Step 3: Configuration -->
              <v-stepper-window-item :value="3">
                <v-card flat>
                  <v-card-text>
                    <h3 class="mb-4">Конфигурация узла</h3>

                    <v-alert
                      v-if="isRootNode"
                      type="info"
                      variant="tonal"
                      class="mb-4"
                    >
                      Для Root узла необходимо указать параметры mesh-сети и подключения к MQTT/Wi-Fi.
                    </v-alert>

                    <v-row v-if="isRootNode" class="mb-2">
                      <v-col cols="12" md="6">
                        <v-combobox
                          v-model="newNode.config.mesh_id"
                          :items="meshIdOptions"
                          item-title="title"
                          item-value="value"
                          label="Mesh Network ID *"
                          variant="outlined"
                          :rules="[rules.meshId]"
                          hint="Пример: zone_greenhouse_1"
                          persistent-hint
                          clearable
                          allow-overflow
                          @update:model-value="handleMeshIdChange"
                        >
                          <template #selection="{ item, index }">
                            <span v-if="index === 0" class="text-body-2">
                              {{ item?.title ?? item?.raw?.title ?? newNode.config.mesh_id }}
                            </span>
                          </template>
                          <template #item="{ props, item }">
                            <v-list-item
                              v-bind="props"
                              :title="item?.raw?.title ?? item?.title ?? props.value"
                            >
                              <template v-if="item?.raw?.subtitle ?? item?.subtitle" #subtitle>
                                <span class="text-caption text-medium-emphasis">
                                  {{ item?.raw?.subtitle ?? item?.subtitle }}
                                </span>
                              </template>
                            </v-list-item>
                          </template>
                        </v-combobox>
                      </v-col>
                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model="newNode.config.root_node_id"
                          label="Root Node ID *"
                          variant="outlined"
                          :rules="[rules.required]"
                          hint="Идентификатор узла в mesh-сети"
                          persistent-hint
                        ></v-text-field>
                      </v-col>
                    </v-row>

                    <v-row v-if="isRootNode" class="mb-2">
                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model="newNode.config.mqtt_broker_host"
                          label="MQTT Host *"
                          variant="outlined"
                          :rules="[rules.host]"
                          hint="Например: 192.168.1.100"
                          persistent-hint
                        ></v-text-field>
                      </v-col>
                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model.number="newNode.config.mqtt_broker_port"
                          label="MQTT Port *"
                          type="number"
                          variant="outlined"
                          :rules="[rules.port]"
                        ></v-text-field>
                      </v-col>
                    </v-row>

                    <v-row v-if="isRootNode" class="mb-6">
                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model="newNode.config.wifi_ssid"
                          label="Wi-Fi SSID *"
                          variant="outlined"
                          :rules="[rules.required]"
                        ></v-text-field>
                      </v-col>
                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model="newNode.config.wifi_password"
                          label="Wi-Fi пароль *"
                          variant="outlined"
                          :type="'password'"
                          :rules="[rules.required]"
                        ></v-text-field>
                      </v-col>
                    </v-row>

                    <!-- Common config -->
                    <v-text-field
                      v-model.number="newNode.config.interval"
                      label="Интервал отправки данных (секунды)"
                      type="number"
                      variant="outlined"
                      :rules="[rules.required, rules.positive]"
                      hint="Как часто узел отправляет телеметрию"
                      persistent-hint
                    ></v-text-field>

                    <!-- Climate specific -->
                    <div v-if="newNode.node_type === 'climate'" class="mt-4">
                      <h4 class="mb-2">Настройки климат-сенсора</h4>
                      
                      <v-row>
                        <v-col cols="6">
                          <v-text-field
                            v-model.number="newNode.config.temp_min"
                            label="Температура мин (°C)"
                            type="number"
                            variant="outlined"
                          ></v-text-field>
                        </v-col>
                        <v-col cols="6">
                          <v-text-field
                            v-model.number="newNode.config.temp_max"
                            label="Температура макс (°C)"
                            type="number"
                            variant="outlined"
                          ></v-text-field>
                        </v-col>
                      </v-row>

                      <v-text-field
                        v-model.number="newNode.config.co2_max"
                        label="CO₂ максимум (ppm)"
                        type="number"
                        variant="outlined"
                      ></v-text-field>
                    </div>

                    <!-- Relay specific -->
                    <div v-if="newNode.node_type === 'relay'" class="mt-4">
                      <h4 class="mb-2">Настройка реле</h4>
                      
                      <v-text-field
                        v-model.number="newNode.config.relay_count"
                        label="Количество реле"
                        type="number"
                        :min="1"
                        :max="8"
                        variant="outlined"
                        hint="От 1 до 8 реле"
                      ></v-text-field>
                    </div>

                    <!-- Advanced JSON config -->
                    <v-expansion-panels class="mt-4">
                      <v-expansion-panel>
                        <v-expansion-panel-title>
                          <v-icon icon="mdi-code-json" class="mr-2"></v-icon>
                          Расширенная конфигурация (JSON)
                        </v-expansion-panel-title>
                        <v-expansion-panel-text>
                          <v-textarea
                            v-model="configJson"
                            rows="8"
                            variant="outlined"
                            :error-messages="jsonError"
                            @update:model-value="validateJson"
                            hint="Можно добавить дополнительные параметры"
                          ></v-textarea>
                        </v-expansion-panel-text>
                      </v-expansion-panel>
                    </v-expansion-panels>
                  </v-card-text>
                </v-card>
              </v-stepper-window-item>

              <!-- Step 4: Confirmation -->
              <v-stepper-window-item :value="4">
                <v-card flat>
                  <v-card-text>
                    <h3 class="mb-4">Подтверждение</h3>

                    <v-alert type="info" variant="tonal" class="mb-4">
                      Проверьте данные перед созданием узла
                    </v-alert>

                    <v-list>
                      <v-list-item>
                        <template v-slot:prepend>
                          <v-icon :icon="getNodeIcon(newNode.node_type)" size="large"></v-icon>
                        </template>
                        <v-list-item-title>{{ newNode.node_id }}</v-list-item-title>
                        <v-list-item-subtitle>{{ getNodeTypeName(newNode.node_type) }}</v-list-item-subtitle>
                      </v-list-item>

                      <v-list-item>
                        <v-list-item-title>Зона</v-list-item-title>
                        <v-list-item-subtitle>{{ newNode.zone }}</v-list-item-subtitle>
                      </v-list-item>

                      <v-list-item v-if="newNode.mac_address">
                        <v-list-item-title>MAC адрес</v-list-item-title>
                        <v-list-item-subtitle>{{ newNode.mac_address }}</v-list-item-subtitle>
                      </v-list-item>

                      <v-list-item v-if="newNode.description">
                        <v-list-item-title>Описание</v-list-item-title>
                        <v-list-item-subtitle>{{ newNode.description }}</v-list-item-subtitle>
                      </v-list-item>

                      <v-divider class="my-2"></v-divider>

                      <v-list-item>
                        <v-list-item-title>Конфигурация</v-list-item-title>
                        <v-list-item-subtitle>
                          <pre class="text-caption">{{ JSON.stringify(finalConfig, null, 2) }}</pre>
                        </v-list-item-subtitle>
                      </v-list-item>
                    </v-list>

                    <v-alert type="success" variant="tonal" class="mt-4">
                      После создания узел будет добавлен в систему и появится на Dashboard
                    </v-alert>
                  </v-card-text>
                </v-card>
              </v-stepper-window-item>
            </v-stepper-window>
          </v-stepper>
        </v-form>
      </v-card-text>

      <v-card-actions>
        <v-btn
          v-if="step > 1"
          @click="step--"
          prepend-icon="mdi-arrow-left"
        >
          Назад
        </v-btn>

        <v-spacer></v-spacer>

        <v-btn @click="cancelAdd">Отмена</v-btn>

        <v-btn
          v-if="step < 4"
          color="primary"
          @click="step++"
          append-icon="mdi-arrow-right"
          :disabled="!canProceed"
        >
          Далее
        </v-btn>

        <v-btn
          v-if="step === 4"
          color="success"
          prepend-icon="mdi-check"
          :disabled="!valid"
          @click="createNode"
        >
          Создать узел
        </v-btn>
      </v-card-actions>
    </v-card>

    <!-- Add Zone Dialog -->
    <v-dialog v-model="addZoneDialog" max-width="400">
      <v-card>
        <v-card-title>Добавить новую зону</v-card-title>
        <v-card-text>
          <v-text-field
            v-model="newZone"
            label="Название зоны"
            variant="outlined"
            placeholder="Zone 3"
          ></v-text-field>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="addZoneDialog = false">Отмена</v-btn>
          <v-btn color="primary" @click="addZone">Добавить</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-dialog>
</template>

<script setup>
import { ref, computed, watch, onMounted } from 'vue'
import { storeToRefs } from 'pinia'
import { useZonesStore } from '@/stores/zones'

const emit = defineEmits(['node-created'])

const zonesStore = useZonesStore()
const { zones: storeZones, selectedZone } = storeToRefs(zonesStore)

const dialog = ref(false)
const step = ref(1)
const form = ref(null)
const valid = ref(false)
const addZoneDialog = ref(false)
const newZone = ref('')
const jsonError = ref(null)
const configJson = ref('')
const zoneTouched = ref(false)
const manualZones = ref([])

const fallbackZones = ['Main', 'Zone 1', 'Zone 2', 'Greenhouse', 'Nursery']

const selectedZoneSummary = computed(() => zonesStore.selectedZoneSummary)

const zoneItems = computed(() => {
  const items = []
  const seen = new Set()

  const pushItem = (rawValue, rawTitle, subtitle = '') => {
    if (!rawValue || typeof rawValue !== 'string') {
      return
    }
    const value = rawValue.trim()
    if (!value || seen.has(value)) {
      return
    }
    const title = typeof rawTitle === 'string' && rawTitle.trim().length
      ? rawTitle.trim()
      : value
    items.push({
      value,
      title,
      subtitle: typeof subtitle === 'string' && subtitle.trim().length ? subtitle.trim() : '',
    })
    seen.add(value)
  }

  storeZones.value.forEach((zone) => {
    if (!zone || typeof zone !== 'object') {
      return
    }

    const value = [
      zone.mesh_network_id,
      zone.zone,
      zone.name,
    ].find((candidate) => typeof candidate === 'string' && candidate.trim().length)

    const title = [
      zone.zone_name,
      zone.name,
      zone.mesh_network_id,
      zone.zone,
    ].find((candidate) => typeof candidate === 'string' && candidate.trim().length)

    const subtitleParts = []
    if (typeof zone.nodes_total === 'number') {
      subtitleParts.push(`Узлов: ${zone.nodes_total}`)
    }
    if (typeof zone.nodes_online === 'number') {
      subtitleParts.push(`Онлайн: ${zone.nodes_online}`)
    }

    pushItem(value, title, subtitleParts.join(' · '))
  })

  manualZones.value
    .filter((zone) => typeof zone === 'string')
    .forEach((zone) => pushItem(zone, zone, 'Добавлена вручную'))

  fallbackZones.forEach((zone) => pushItem(zone, zone, 'Зона по умолчанию'))

  return items
})

const zoneValues = computed(() => zoneItems.value.map((item) => item.value))

const hasZoneItems = computed(() => zoneItems.value.length > 0)

const meshIdOptions = computed(() =>
  zoneItems.value.map((item) => ({
    value: item.value,
    title: item.title,
    subtitle: item.subtitle,
  })),
)

const newNode = ref({
  node_id: '',
  node_type: null,
  zone: '',
  mac_address: '',
  description: '',
  config: {
    interval: 30,
    mesh_id: '',
    root_node_id: '',
    mqtt_broker_host: '192.168.1.100',
    mqtt_broker_port: 1883,
    wifi_ssid: '',
    wifi_password: '',
  },
  metadata: {},
})

function sanitizeMeshId(value) {
  if (typeof value !== 'string') {
    return ''
  }
  return value.trim().replace(/\s+/g, '_').replace(/[^a-zA-Z0-9_-]/g, '_')
}

const nodeTypes = [
  {
    value: 'root',
    label: 'Root Node',
    description: 'Главный узел mesh-сети',
    icon: 'mdi-server-network',
  },
  {
    value: 'deprecated_ph_ec',
    label: 'pH/EC (устарело)',
    description: 'Комбинированный узел. Используйте отдельные pH и EC устройства.',
    icon: 'mdi-flask-off',
    disabled: true,
  },
  {
    value: 'climate',
    label: 'Климат-контроль',
    description: 'Температура, влажность, CO₂',
    icon: 'mdi-thermometer',
  },
  {
    value: 'relay',
    label: 'Реле',
    description: 'Управление окнами, вентиляцией',
    icon: 'mdi-electric-switch',
  },
  {
    value: 'water',
    label: 'Уровень воды',
    description: 'Мониторинг резервуара',
    icon: 'mdi-water',
  },
  {
    value: 'display',
    label: 'Дисплей',
    description: 'LCD экран для отображения данных',
    icon: 'mdi-monitor',
  },
]

const rules = {
  required: v => !!v || 'Обязательное поле',
  positive: v => v > 0 || 'Должно быть больше 0',
  nodeId: v => {
    const pattern = /^[a-z0-9_]+$/
    return pattern.test(v) || 'Только строчные буквы, цифры и _'
  },
  macAddress: v => {
    if (!v) return true
    const pattern = /^([0-9A-F]{2}:){5}[0-9A-F]{2}$/i
    return pattern.test(v) || 'Неверный формат MAC адреса'
  },
  meshId: v => {
    if (!v) return 'Укажите mesh ID'
    const pattern = /^[a-zA-Z0-9_-]+$/
    return pattern.test(v) || 'Используйте буквы, цифры, - или _'
  },
  host: v => !!v || 'Укажите адрес брокера',
  port: v => {
    const num = Number(v)
    return (num > 0 && num < 65536) || 'Порт 1-65535'
  },
}

const suggestedPrefix = computed(() => {
  if (!newNode.value.node_type) return ''
  return `${newNode.value.node_type}_`
})

const isRootNode = computed(() => newNode.value.node_type === 'root')

const canProceed = computed(() => {
  if (step.value === 1) {
    // Должен быть выбран тип узла
    return !!newNode.value.node_type
  }
  if (step.value === 2) {
    // Должны быть заполнены node_id и zone (минимальная валидация)
    const nodeId = typeof newNode.value.node_id === 'string' ? newNode.value.node_id.trim() : ''
    const zoneValue = typeof newNode.value.zone === 'string' ? newNode.value.zone.trim() : ''
    const hasNodeId = nodeId.length > 0
    const hasZone = zoneValue.length > 0
    return hasNodeId && hasZone
  }
  if (step.value === 3) {
    if (!isRootNode.value) {
      return true
    }
    const cfg = newNode.value.config
    return !!cfg.mesh_id &&
      !!cfg.root_node_id &&
      !!cfg.mqtt_broker_host &&
      !!cfg.mqtt_broker_port &&
      !!cfg.wifi_ssid &&
      !!cfg.wifi_password
  }
  // Шаг 4 - подтверждение
  return true
})

const finalConfig = computed(() => {
  if (jsonError.value || !configJson.value) {
    return newNode.value.config
  }
  
  try {
    return JSON.parse(configJson.value)
  } catch (e) {
    return newNode.value.config
  }
})

async function ensureZonesAvailable() {
  if (storeZones.value.length) {
    return
  }
  try {
    await zonesStore.fetchZones()
  } catch (error) {
    console.error('AddNodeDialog.vue: failed to fetch zones', error)
  }
}

onMounted(() => {
  ensureZonesAvailable()
})

function setDefaultZone(force = false) {
  const available = zoneValues.value

  if (!available.length) {
    newNode.value.zone = ''
    return
  }

  if (!force && zoneTouched.value) {
    return
  }

  const currentZone = typeof newNode.value.zone === 'string' ? newNode.value.zone : ''
  if (currentZone && available.includes(currentZone)) {
    return
  }

  const summary = selectedZoneSummary.value || null
  const candidates = [
    currentZone,
    summary?.mesh_network_id,
    summary?.zone,
    summary?.name,
    selectedZone.value,
  ]

  for (const candidate of candidates) {
    if (typeof candidate === 'string' && available.includes(candidate)) {
      newNode.value.zone = candidate
      if (isRootNode.value) {
        const sanitized = sanitizeMeshId(candidate)
        if (!newNode.value.config.mesh_id || force) {
          newNode.value.config.mesh_id = sanitized
        }
      }
      return
    }
  }

  newNode.value.zone = available[0]
  if (isRootNode.value) {
    newNode.value.config.mesh_id = sanitizeMeshId(available[0])
  }
}

function handleZoneChange(value) {
  zoneTouched.value = true
  if (typeof value === 'string') {
    newNode.value.zone = value
    if (isRootNode.value && value) {
      newNode.value.config.mesh_id = sanitizeMeshId(value)
    }
  } else {
    newNode.value.zone = ''
  }
}

watch(dialog, async (val) => {
  console.log('Dialog opened:', val)
  if (val) {
    await ensureZonesAvailable()
    resetForm()
  } else {
    zoneTouched.value = false
  }
})

watch(zoneItems, () => {
  if (!dialog.value) return
  setDefaultZone()
})

watch(selectedZone, () => {
  if (!dialog.value) return
  if (zoneTouched.value) return
  setDefaultZone()
})

watch(selectedZoneSummary, () => {
  if (!dialog.value) return
  if (zoneTouched.value) return
  setDefaultZone()
})

function handleMeshIdChange(value) {
  if (!isRootNode.value) {
    return
  }
  const raw = typeof value === 'string' ? value : ''
  const sanitized = sanitizeMeshId(raw)
  newNode.value.config.mesh_id = sanitized

  if (zoneTouched.value) {
    return
  }

  if (raw && zoneValues.value.includes(raw)) {
    newNode.value.zone = raw
  }
}

watch(step, (newVal, oldVal) => {
  console.log('Step changed:', oldVal, '→', newVal)
})

watch(() => newNode.value.node_id, (val, oldVal) => {
  if (!isRootNode.value) return
  if (!newNode.value.config.root_node_id || newNode.value.config.root_node_id === oldVal) {
    newNode.value.config.root_node_id = val || ''
  }
  if (!newNode.value.config.mesh_id && val) {
    newNode.value.config.mesh_id = val.replace(/[^a-zA-Z0-9_-]/g, '_')
  }
})

watch(() => newNode.value.zone, (val, oldVal) => {
  if (!isRootNode.value) return
  const sanitized = sanitizeMeshId(typeof val === 'string' ? val : '')
  const prevSanitized = sanitizeMeshId(typeof oldVal === 'string' ? oldVal : '')
  if (!sanitized) {
    return
  }
  if (!newNode.value.config.mesh_id || newNode.value.config.mesh_id === prevSanitized) {
    newNode.value.config.mesh_id = sanitized
  }
})

watch(() => newNode.value.config, (config) => {
  configJson.value = JSON.stringify(config, null, 2)
}, { deep: true })

const showDeprecatedPhEcAlert = ref(false)

function selectNodeType(type) {
  if (!type || typeof type !== 'object') {
    return
  }
  if (type.disabled || type.value === 'deprecated_ph_ec') {
    showDeprecatedPhEcAlert.value = true
    newNode.value.node_type = null
    return
  }

  const value = type.value
  console.log('Selecting node type:', value)
  newNode.value.node_type = value
  
  // Set default zone if not set
  if (!newNode.value.zone || !zoneValues.value.includes(newNode.value.zone)) {
    setDefaultZone(true)
  }
  
  // Set default config based on type
  if (value === 'climate') {
    newNode.value.config = {
      interval: 30,
      temp_min: 18,
      temp_max: 28,
      co2_max: 1200,
    }
  } else if (value === 'relay') {
    newNode.value.config = {
      interval: 60,
      relay_count: 4,
    }
  } else if (value === 'root') {
    newNode.value.config = {
      interval: 30,
      mesh_id: '',
      root_node_id: '',
      mqtt_broker_host: '192.168.1.100',
      mqtt_broker_port: 1883,
      wifi_ssid: '',
      wifi_password: '',
    }
  } else {
    newNode.value.config = {
      interval: 30,
    }
  }
  
  // Auto-advance to next step
  console.log('Moving to step 2')
  step.value = 2
  console.log('Current step:', step.value)
}

function validateJson(value) {
  try {
    JSON.parse(value)
    jsonError.value = null
  } catch (e) {
    jsonError.value = 'Невалидный JSON: ' + e.message
  }
}

function addZone() {
  const name = typeof newZone.value === 'string' ? newZone.value.trim() : ''

  if (!name) {
    addZoneDialog.value = false
    newZone.value = ''
    return
  }

  if (!manualZones.value.includes(name)) {
    manualZones.value.push(name)
  }

  newNode.value.zone = name
  zoneTouched.value = true
  addZoneDialog.value = false
  newZone.value = ''
}

function getNodeTypeName(type) {
  return nodeTypes.find(t => t.value === type)?.label || type
}

function getNodeIcon(type) {
  return nodeTypes.find(t => t.value === type)?.icon || 'mdi-chip'
}

async function createNode() {
  // Validate form
  const formRef = form.value
  if (!formRef) {
    return
  }

  const { valid: isValid } = await formRef.validate()
  if (!isValid) {
    console.error('Form validation failed')
    return
  }

  const zoneValue = typeof newNode.value.zone === 'string' ? newNode.value.zone.trim() : ''
  const mac = typeof newNode.value.mac_address === 'string' ? newNode.value.mac_address.trim() : ''

  const nodeData = {
    node_id: newNode.value.node_id.trim(),
    node_type: newNode.value.node_type,
    zone: zoneValue || null,
    mac_address: mac || null,
    config: finalConfig.value,
    metadata: {
      description: newNode.value.description || '',
      created_via: 'web_ui',
      created_at: new Date().toISOString(),
    },
  }

  console.log('Prepared node payload:', nodeData)

  emit('node-created', nodeData)
  dialog.value = false
  resetForm()
}

function cancelAdd() {
  dialog.value = false
  resetForm()
}

function resetForm() {
  step.value = 1
  zoneTouched.value = false
  newNode.value = {
    node_id: '',
    node_type: null,
    zone: '',
    mac_address: '',
    description: '',
    config: {
      interval: 30,
      mesh_id: '',
      root_node_id: '',
      mqtt_broker_host: '192.168.1.100',
      mqtt_broker_port: 1883,
      wifi_ssid: '',
      wifi_password: '',
    },
    metadata: {},
  }
  configJson.value = ''
  jsonError.value = null
  setDefaultZone(true)
}
</script>

<style scoped>
.border-primary {
  border: 2px solid rgb(var(--v-theme-primary)) !important;
}

pre {
  white-space: pre-wrap;
  word-break: break-all;
}
</style>

