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
                    
                    <v-row>
                      <v-col
                        v-for="type in nodeTypes"
                        :key="type.value"
                        cols="12"
                        sm="6"
                        md="4"
                      >
                        <v-card
                          :class="{ 'border-primary': newNode.node_type === type.value }"
                          :variant="newNode.node_type === type.value ? 'tonal' : 'outlined'"
                          :color="newNode.node_type === type.value ? 'primary' : ''"
                          @click="selectNodeType(type.value)"
                          hover
                          style="cursor: pointer"
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
                      hint="Пример: ph_ec_002, climate_001"
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
                      :items="zones"
                      label="Зона *"
                      variant="outlined"
                      :rules="[rules.required]"
                      class="mt-4"
                      clearable
                    >
                      <template v-slot:append>
                        <v-btn
                          icon="mdi-plus"
                          size="small"
                          variant="text"
                          @click="addZoneDialog = true"
                        ></v-btn>
                      </template>
                    </v-select>
                    
                    <v-alert
                      v-if="!newNode.zone"
                      type="info"
                      variant="tonal"
                      density="compact"
                      class="mt-2"
                    >
                      Выберите зону или создайте новую (+)
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

                    <!-- pH/EC specific -->
                    <div v-if="newNode.node_type === 'ph_ec'" class="mt-4">
                      <h4 class="mb-2">Настройки pH/EC сенсора</h4>
                      
                      <v-row>
                        <v-col cols="6">
                          <v-text-field
                            v-model.number="newNode.config.ph_min"
                            label="pH минимум"
                            type="number"
                            step="0.1"
                            variant="outlined"
                            hint="Нижний порог pH"
                          ></v-text-field>
                        </v-col>
                        <v-col cols="6">
                          <v-text-field
                            v-model.number="newNode.config.ph_max"
                            label="pH максимум"
                            type="number"
                            step="0.1"
                            variant="outlined"
                            hint="Верхний порог pH"
                          ></v-text-field>
                        </v-col>
                      </v-row>

                      <v-row>
                        <v-col cols="6">
                          <v-text-field
                            v-model.number="newNode.config.ec_min"
                            label="EC минимум"
                            type="number"
                            step="0.1"
                            variant="outlined"
                            hint="Нижний порог EC"
                          ></v-text-field>
                        </v-col>
                        <v-col cols="6">
                          <v-text-field
                            v-model.number="newNode.config.ec_max"
                            label="EC максимум"
                            type="number"
                            step="0.1"
                            variant="outlined"
                            hint="Верхний порог EC"
                          ></v-text-field>
                        </v-col>
                      </v-row>
                    </div>

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
          :loading="loading"
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
import { ref, computed, watch } from 'vue'
import api from '@/services/api'
import { useAppStore } from '@/stores/app'

const emit = defineEmits(['node-created'])
const appStore = useAppStore()

const dialog = ref(false)
const step = ref(1)
const form = ref(null)
const valid = ref(false)
const loading = ref(false)
const addZoneDialog = ref(false)
const newZone = ref('')
const jsonError = ref(null)
const configJson = ref('')

const zones = ref(['Main', 'Zone 1', 'Zone 2', 'Greenhouse', 'Nursery'])

const newNode = ref({
  node_id: '',
  node_type: null,
  zone: '',
  mac_address: '',
  description: '',
  config: {
    interval: 30,
  },
  metadata: {},
})

const nodeTypes = [
  {
    value: 'root',
    label: 'Root Node',
    description: 'Главный узел mesh-сети',
    icon: 'mdi-server-network',
  },
  {
    value: 'ph_ec',
    label: 'pH/EC Сенсор',
    description: 'Измерение pH и EC раствора',
    icon: 'mdi-flask',
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
}

const suggestedPrefix = computed(() => {
  if (!newNode.value.node_type) return ''
  return `${newNode.value.node_type}_`
})

const canProceed = computed(() => {
  if (step.value === 1) {
    // Должен быть выбран тип узла
    return !!newNode.value.node_type
  }
  if (step.value === 2) {
    // Должны быть заполнены node_id и zone (минимальная валидация)
    const hasNodeId = newNode.value.node_id && newNode.value.node_id.length > 0
    const hasZone = newNode.value.zone && newNode.value.zone.length > 0
    return hasNodeId && hasZone
  }
  if (step.value === 3) {
    // Шаг конфигурации - всегда можно продолжить
    return true
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

watch(dialog, (val) => {
  console.log('Dialog opened:', val)
  if (val) {
    resetForm()
  }
})

watch(step, (newVal, oldVal) => {
  console.log('Step changed:', oldVal, '→', newVal)
})

watch(() => newNode.value.config, (config) => {
  configJson.value = JSON.stringify(config, null, 2)
}, { deep: true })

function selectNodeType(type) {
  console.log('Selecting node type:', type)
  newNode.value.node_type = type
  
  // Set default zone if not set
  if (!newNode.value.zone) {
    newNode.value.zone = 'Main'
  }
  
  // Set default config based on type
  if (type === 'ph_ec') {
    newNode.value.config = {
      interval: 30,
      ph_min: 5.5,
      ph_max: 6.5,
      ec_min: 1.2,
      ec_max: 2.0,
    }
  } else if (type === 'climate') {
    newNode.value.config = {
      interval: 30,
      temp_min: 18,
      temp_max: 28,
      co2_max: 1200,
    }
  } else if (type === 'relay') {
    newNode.value.config = {
      interval: 60,
      relay_count: 4,
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
  console.log('🔍 AddNodeDialog: addZone function called')
  console.log('🔍 AddNodeDialog: newZone.value:', newZone.value, typeof newZone.value)
  console.log('🔍 AddNodeDialog: zones.value:', zones.value, typeof zones.value)
  
  try {
    // Дополнительная проверка на undefined/null перед вызовом includes
    if (newZone.value && zones.value && Array.isArray(zones.value) && !zones.value.includes(newZone.value)) {
      console.log('🔍 AddNodeDialog: Adding new zone:', newZone.value)
      zones.value.push(newZone.value)
      newNode.value.zone = newZone.value
    }
  } catch (error) {
    console.error('AddNodeDialog.vue: addZone - Error in includes:', error)
    console.error('AddNodeDialog.vue: addZone - newZone.value:', newZone.value, typeof newZone.value)
    console.error('AddNodeDialog.vue: addZone - zones.value:', zones.value, typeof zones.value)
  }
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
  const { valid: isValid } = await form.value.validate()
  if (!isValid) {
    console.error('Form validation failed')
    return
  }

  loading.value = true

  try {
    const nodeData = {
      node_id: newNode.value.node_id.trim(),
      node_type: newNode.value.node_type,
      zone: newNode.value.zone,
      mac_address: newNode.value.mac_address?.trim() || null,
      config: finalConfig.value,
      metadata: {
        description: newNode.value.description || '',
        created_via: 'web_ui',
        created_at: new Date().toISOString(),
      },
    }

    console.log('Creating node with data:', nodeData)
    console.log('Node data details:', {
      node_id: nodeData.node_id,
      node_id_length: nodeData.node_id?.length,
      node_type: nodeData.node_type,
      zone: nodeData.zone
    })

    // Call API to create node
    const response = await api.createNode(nodeData)
    console.log('API response received:', response)
    
    // Backend returns { success: true, node: {...} }
    const createdNode = response.node || response
    
    console.log('Node created successfully:', createdNode)
    
    // Success notification
    appStore.showSnackbar(
      `Узел ${createdNode.node_id} успешно создан`, 
      'success'
    )
    
    emit('node-created', createdNode)
    dialog.value = false
    
  } catch (error) {
    console.error('Error creating node:', error)
    console.error('Error response:', error.response?.data)
    
    // Error notification
    let errorMessage = 'Не удалось создать узел'
    if (error.response?.data?.message) {
      errorMessage = error.response.data.message
    } else if (error.response?.data?.errors) {
      // Laravel validation errors
      const errors = Object.values(error.response.data.errors).flat()
      errorMessage = errors.join(', ')
      console.error('Validation errors:', errors)
    } else if (error.message) {
      errorMessage = error.message
    }
    
    appStore.showSnackbar(errorMessage, 'error')
  } finally {
    loading.value = false
  }
}

function cancelAdd() {
  dialog.value = false
  resetForm()
}

function resetForm() {
  step.value = 1
  newNode.value = {
    node_id: '',
    node_type: null,
    zone: '',
    mac_address: '',
    description: '',
    config: {
      interval: 30,
    },
    metadata: {},
  }
  configJson.value = ''
  jsonError.value = null
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

