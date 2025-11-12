<template>
  <v-dialog
    v-model="internalModel"
    max-width="860"
    persistent
    @keydown.esc.prevent="handleCancel"
  >
    <v-card>
      <v-toolbar color="primary" dark>
        <v-toolbar-title>
          <v-icon class="mr-2" icon="mdi-cog-play"></v-icon>
          {{ node?.is_root ? 'Настройка ROOT узла' : 'Настройка узла' }}
        </v-toolbar-title>
        <v-spacer></v-spacer>
        <v-chip
          size="small"
          variant="outlined"
          color="white"
        >
          PIN {{ node?.pin }}
        </v-chip>
        <v-btn icon="mdi-close" variant="text" @click="handleCancel"></v-btn>
      </v-toolbar>

      <v-card-text class="pt-6">
        <v-stepper v-model="step" alt-labels flat>
          <v-stepper-header>
            <v-stepper-item
              :value="1"
              :complete="step > 1"
              :title="stepLabels[0]"
            ></v-stepper-item>

            <v-divider></v-divider>

            <v-stepper-item
              :value="2"
              :complete="step > 2"
              :title="stepLabels[1]"
            ></v-stepper-item>

            <v-divider></v-divider>

            <v-stepper-item
              v-if="maxStep >= 3"
              :value="3"
              :complete="step > 3"
              :title="stepLabels[2]"
            ></v-stepper-item>

            <template v-if="maxStep === 4">
              <v-divider></v-divider>
              <v-stepper-item
                :value="4"
                :title="stepLabels[3]"
              ></v-stepper-item>
            </template>
          </v-stepper-header>

          <v-stepper-window>
            <v-stepper-window-item :value="1">
              <v-card flat>
                <v-card-text>
                  <v-alert
                    border="start"
                    type="info"
                    variant="tonal"
                    class="mb-4"
                  >
                    Подключите устройство к питанию. Индикатор должен мигать.
                  </v-alert>

                  <v-row dense>
                    <v-col cols="12" md="6">
                      <h3 class="text-subtitle-1 mb-1">1. Wi-Fi сеть устройства</h3>
                      <div class="text-body-2">
                        SSID:
                        <strong>{{ setupSsid }}</strong>
                        <v-btn
                          size="x-small"
                          variant="text"
                          icon="mdi-content-copy"
                          @click="copyToClipboard(setupSsid)"
                        ></v-btn>
                      </div>
                      <div class="text-body-2">
                        Пароль:
                        <strong>{{ setupPassword }}</strong>
                        <v-btn
                          size="x-small"
                          variant="text"
                          icon="mdi-content-copy"
                          @click="copyToClipboard(setupPassword)"
                        ></v-btn>
                      </div>
                    </v-col>
                    <v-col cols="12" md="6">
                      <h3 class="text-subtitle-1 mb-1">2. Отправьте учётные данные</h3>
                      <div class="text-body-2">
                        Откройте браузер и перейдите по адресу:
                        <strong>{{ setupPortalUrl }}</strong>
                        <v-btn
                          size="x-small"
                          variant="text"
                          icon="mdi-content-copy"
                          @click="copyToClipboard(setupPortalUrl)"
                        ></v-btn>
                      </div>
                      <div class="text-caption text-medium-emphasis mt-2">
                        Введите параметры Wi-Fi вашей сети, чтобы устройство подключилось к роутеру.
                      </div>
                    </v-col>
                  </v-row>
                </v-card-text>
              </v-card>
            </v-stepper-window-item>

            <v-stepper-window-item :value="2">
              <v-card flat>
                <v-card-text>
                  <template v-if="node?.is_root">
                    <h3 class="text-subtitle-1 mb-4">Состояние соединения</h3>
                    <v-row dense>
                      <v-col cols="12" md="4">
                        <v-sheet class="pa-4 rounded-xl" variant="outlined">
                          <div class="text-caption text-medium-emphasis mb-1">
                            Статус
                          </div>
                          <div class="text-body-1 font-weight-medium d-flex align-center">
                            <v-icon
                              :color="node?.is_online ? 'success' : 'warning'"
                              class="mr-2"
                              :icon="node?.is_online ? 'mdi-check-circle' : 'mdi-timer-sand'"
                            ></v-icon>
                            {{ node?.is_online ? 'Онлайн' : 'Ожидаем подключение' }}
                          </div>
                          <div class="text-caption text-medium-emphasis mt-2">
                            Последний сигнал: {{ lastHeartbeatAgo }}
                          </div>
                        </v-sheet>
                      </v-col>
                      <v-col cols="12" md="8">
                        <v-alert
                          :type="node?.is_online ? 'success' : 'warning'"
                          variant="tonal"
                          border="start"
                        >
                          {{ connectionHint }}
                        </v-alert>
                      </v-col>
                    </v-row>
                  </template>

                  <template v-else>
                    <h3 class="text-subtitle-1 mb-4">Информация об узле</h3>
                    <v-row dense>
                      <v-col cols="12" md="6">
                        <v-sheet class="pa-4 rounded-xl" variant="outlined">
                          <div class="text-caption text-medium-emphasis mb-1">
                            Временный mesh ID
                          </div>
                          <div class="text-body-1 font-weight-medium">
                            {{ node?.temp_mesh_id || '—' }}
                          </div>
                        </v-sheet>
                      </v-col>
                      <v-col cols="12" md="6">
                        <v-sheet class="pa-4 rounded-xl" variant="outlined">
                          <div class="text-caption text-medium-emphasis mb-1">
                            Тип устройства
                          </div>
                          <div class="text-body-1 font-weight-medium">
                            {{ node?.node_type }}
                          </div>
                        </v-sheet>
                      </v-col>
                    </v-row>
                  </template>
                </v-card-text>
              </v-card>
            </v-stepper-window-item>

            <v-stepper-window-item v-if="maxStep >= 3" :value="3">
              <v-card flat>
                <v-card-text>
                  <h3 class="text-subtitle-1 mb-4">
                    {{ node?.is_root ? 'Параметры ROOT узла' : 'Параметры узла' }}
                  </h3>

                  <v-form ref="formRef">
                    <v-row dense>
                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model="currentForm.nodeId"
                          label="ID узла"
                          variant="outlined"
                          :rules="[rules.required, rules.nodeId]"
                          clearable
                        ></v-text-field>
                      </v-col>

                      <v-col cols="12" md="6">
                        <v-combobox
                          v-model="currentForm.zone"
                          :items="zoneOptions"
                          label="Зона"
                          variant="outlined"
                          clearable
                          :rules="[rules.required]"
                          hint="Можно выбрать существующую зону или ввести новую"
                          persistent-hint
                        ></v-combobox>
                      </v-col>

                      <v-col cols="12" md="6">
                        <v-select
                          v-model="currentForm.greenhouseId"
                          :items="greenhouseOptions"
                          item-title="label"
                          item-value="value"
                          label="Теплица"
                          variant="outlined"
                          clearable
                          :loading="greenhousesLoading"
                          hint="Определяет теплицу, к которой будет закреплён узел"
                          persistent-hint
                        ></v-select>
                      </v-col>

                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model="currentForm.meshId"
                          label="Mesh ID"
                          variant="outlined"
                          :rules="[rules.required]"
                          clearable
                        ></v-text-field>
                      </v-col>

                      <v-col cols="12" md="6" v-if="!node?.is_root">
                        <v-text-field
                          v-model="regularForm.rootNodeId"
                          label="ID ROOT узла"
                          variant="outlined"
                          hint="По умолчанию будет использован ID зоны"
                          persistent-hint
                        ></v-text-field>
                      </v-col>

                      <template v-if="node?.is_root">
                        <v-col cols="12" md="6">
                          <v-text-field
                            v-model="rootForm.mqttHost"
                            label="MQTT хост"
                            variant="outlined"
                            :rules="[rules.required]"
                          ></v-text-field>
                        </v-col>

                        <v-col cols="12" md="6">
                          <v-text-field
                            v-model.number="rootForm.mqttPort"
                            label="MQTT порт"
                            type="number"
                            variant="outlined"
                            :rules="[rules.required, rules.port]"
                          ></v-text-field>
                        </v-col>

                        <v-col cols="12" md="6">
                          <v-text-field
                            v-model="rootForm.wifiSsid"
                            label="SSID вашей сети"
                            variant="outlined"
                            :rules="[rules.required]"
                          ></v-text-field>
                        </v-col>

                        <v-col cols="12" md="6">
                          <v-text-field
                            v-model="rootForm.wifiPassword"
                            label="Пароль Wi-Fi"
                            type="password"
                            variant="outlined"
                            :rules="[rules.required]"
                          ></v-text-field>
                        </v-col>

                        <v-col cols="12">
                          <v-text-field
                            v-model="rootForm.zoneLocation"
                            label="Местоположение зоны (опционально)"
                            variant="outlined"
                          ></v-text-field>
                        </v-col>
                      </template>

                      <v-col cols="12" v-if="!node?.is_root">
                        <v-textarea
                          v-model="regularForm.notes"
                          label="Примечания (опционально)"
                          variant="outlined"
                          rows="2"
                        ></v-textarea>
                      </v-col>
                    </v-row>
                  </v-form>
                </v-card-text>
              </v-card>
            </v-stepper-window-item>

            <v-stepper-window-item :value="maxStep">
              <v-card flat>
                <v-card-text>
                  <h3 class="text-subtitle-1 mb-4">Подтверждение</h3>
                  <v-alert
                    type="success"
                    variant="tonal"
                    border="start"
                    class="mb-4"
                  >
                    Проверьте параметры перед отправкой конфигурации на устройство.
                  </v-alert>

                  <v-row>
                    <v-col cols="12" md="6">
                      <v-sheet class="pa-4 rounded-xl" variant="outlined">
                        <div class="text-caption text-medium-emphasis mb-1">ID узла</div>
                        <div class="text-body-1 font-weight-medium">{{ currentForm.nodeId }}</div>

                        <div class="text-caption text-medium-emphasis mt-4 mb-1">Зона</div>
                        <div class="text-body-1 font-weight-medium">{{ currentForm.zone }}</div>

                        <div class="text-caption text-medium-emphasis mt-4 mb-1">Mesh ID</div>
                        <div class="text-body-1 font-weight-medium">{{ currentForm.meshId }}</div>

                        <div class="text-caption text-medium-emphasis mt-4 mb-1">Теплица</div>
                        <div class="text-body-1 font-weight-medium">
                          {{ greenhouseLabel(currentForm.greenhouseId ?? null) }}
                        </div>
                      </v-sheet>
                    </v-col>

                    <v-col cols="12" md="6" v-if="node?.is_root">
                      <v-sheet class="pa-4 rounded-xl" variant="outlined">
                        <div class="text-caption text-medium-emphasis mb-1">MQTT</div>
                        <div class="text-body-1 font-weight-medium">
                          {{ rootForm.mqttHost }}:{{ rootForm.mqttPort }}
                        </div>

                        <div class="text-caption text-medium-emphasis mt-4 mb-1">Wi-Fi</div>
                        <div class="text-body-1 font-weight-medium">
                          {{ rootForm.wifiSsid }}
                        </div>
                      </v-sheet>
                    </v-col>

                    <v-col cols="12" v-else>
                      <v-sheet class="pa-4 rounded-xl" variant="outlined">
                        <div class="text-caption text-medium-emphasis mb-1">ROOT узел</div>
                        <div class="text-body-1 font-weight-medium">
                          {{ regularForm.rootNodeId || 'Будет назначен автоматически' }}
                        </div>

                        <div
                          v-if="regularForm.notes"
                          class="text-caption text-medium-emphasis mt-4 mb-1"
                        >
                          Примечания
                        </div>
                        <div class="text-body-2">
                          {{ regularForm.notes }}
                        </div>
                      </v-sheet>
                    </v-col>
                  </v-row>
                </v-card-text>
              </v-card>
            </v-stepper-window-item>
          </v-stepper-window>
        </v-stepper>
      </v-card-text>

      <v-divider></v-divider>

      <v-card-actions class="pa-4">
        <v-btn
          variant="text"
          prepend-icon="mdi-arrow-left"
          :disabled="step === 1 || loading"
          @click="prevStep"
        >
          Назад
        </v-btn>

        <v-spacer></v-spacer>

        <v-btn
          color="primary"
          prepend-icon="mdi-arrow-right"
          v-if="step < maxStep"
          :disabled="loading"
          @click="nextStep"
        >
          Далее
        </v-btn>

        <v-btn
          color="success"
          prepend-icon="mdi-check-circle"
          v-else
          :loading="loading"
          @click="submit"
        >
          Отправить конфигурацию
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { computed, watch, ref, reactive } from 'vue'
import type { NewNode } from '@/types/newNode'
import type { Zone } from '@/stores/zones'
import { formatDistanceToNow } from '@/utils/time'
import { useGreenhousesStore } from '@/stores/greenhouses'
import { useSetupStore } from '@/stores/setup'

const props = defineProps<{
  modelValue: boolean
  node: NewNode | null
  zones: Zone[]
  loading?: boolean
}>()

const emit = defineEmits<{
  (event: 'update:modelValue', value: boolean): void
  (event: 'submit', payload: Record<string, any>): void
  (event: 'cancel'): void
}>()

const internalModel = ref(false)
const step = ref(1)
const formRef = ref()
const greenhousesStore = useGreenhousesStore()
const setupStore = useSetupStore()
const rules = {
  required: (value: any) => !!value || 'Обязательное поле',
  nodeId: (value: string) => /^[a-zA-Z0-9_\-]+$/.test(value || '') || 'Допустимы латинские буквы, цифры, -, _',
  port: (value: number) => value > 0 && value < 65536 || 'Недопустимый порт',
}

const rootForm = reactive({
  nodeId: '',
  zone: '',
  meshId: '',
  mqttHost: '192.168.1.100',
  mqttPort: 1883,
  wifiSsid: '',
  wifiPassword: '',
  rootNodeId: '',
  zoneLocation: '',
  greenhouseId: greenhousesStore.selectedId ?? null,
})

const regularForm = reactive({
  nodeId: '',
  zone: '',
  meshId: '',
  rootNodeId: '',
  notes: '',
  greenhouseId: greenhousesStore.selectedId ?? null,
})

const currentForm = computed(() => (props.node?.is_root ? rootForm : regularForm))

const stepLabels = computed(() => {
  if (props.node?.is_root) {
    return ['Подключение', 'Состояние', 'Параметры', 'Подтверждение']
  }
  return ['Подготовка', 'Параметры', 'Подтверждение']
})

const maxStep = computed(() => stepLabels.value.length)

const zoneOptions = computed(() => props.zones?.map(zone => zone.name) ?? [])

const greenhouseOptions = computed(() => greenhousesStore.greenhouseOptions)
const greenhousesLoading = computed(() => greenhousesStore.loading)

const lastHeartbeatAgo = computed(() => {
  if (!props.node?.last_heartbeat_at) return 'нет данных'
  return formatDistanceToNow(props.node.last_heartbeat_at)
})

const connectionHint = computed(() => {
  if (props.node?.is_online) {
    return 'Устройство подключено к вашей сети. Можно переходить к настройке.'
  }
  return 'Устройство ещё не подключилось к сети. Проверьте питание, данные Wi-Fi и повторите попытку.'
})

const setupSsid = computed(() => {
  if (props.node?.metadata?.setup_ssid) return props.node.metadata.setup_ssid
  if (props.node?.temp_mesh_id) return props.node.temp_mesh_id.replace('HYDRO_', 'HYDRO_SETUP_')
  if (props.node?.pin) return `HYDRO_SETUP_${props.node.pin}`
  return 'HYDRO_SETUP_XXXXXX'
})

const setupPassword = computed(() => props.node?.metadata?.setup_password || 'hydro2025')

const setupPortalUrl = computed(() => props.node?.metadata?.portal_url || 'http://192.168.4.1')

watch(() => props.modelValue, (value) => {
  internalModel.value = value
  if (value) {
    initialiseForms()
    loadGreenhouses()
  }
})

watch(internalModel, (value) => {
  emit('update:modelValue', value)
  if (!value) {
    emit('cancel')
  }
})

watch(() => props.node, () => {
  if (internalModel.value) {
    initialiseForms()
  }
})

watch(() => rootForm.zone, (value) => {
  if (!props.node?.is_root || !value) return
  const zone = props.zones?.find(item => item.name === value)
  if (zone) {
    rootForm.meshId = zone.mesh_network_id || rootForm.meshId
    rootForm.rootNodeId = zone.root_node_id || rootForm.rootNodeId
    if (zone.greenhouse_id != null) {
      rootForm.greenhouseId = zone.greenhouse_id
    }
  }
})

watch(() => regularForm.zone, (value) => {
  if (props.node?.is_root || !value) return
  const zone = props.zones?.find(item => item.name === value)
  if (zone) {
    regularForm.meshId = zone.mesh_network_id || regularForm.meshId
    regularForm.rootNodeId = zone.root_node_id || regularForm.rootNodeId
    if (zone.greenhouse_id != null) {
      regularForm.greenhouseId = zone.greenhouse_id
    }
  }
})

watch(
  () => setupStore.targetGreenhouseId,
  (value) => {
    if (value == null) {
      return
    }
    if (rootForm.greenhouseId == null) {
      rootForm.greenhouseId = value
    }
    if (regularForm.greenhouseId == null) {
      regularForm.greenhouseId = value
    }
  },
  { immediate: true },
)

watch(
  () => greenhousesStore.selectedId,
  (value) => {
    if (value == null) {
      return
    }
    if (rootForm.greenhouseId == null) {
      rootForm.greenhouseId = value
    }
    if (regularForm.greenhouseId == null) {
      regularForm.greenhouseId = value
    }
  },
)

async function loadGreenhouses() {
  try {
    if (!greenhousesStore.items.length) {
      await greenhousesStore.fetchGreenhouses()
    }
  } catch (error) {
    console.warn('Не удалось загрузить список теплиц', error)
  }
}

function initialiseForms() {
  step.value = 1
  if (!props.node) return

  const defaultNodeId = createSuggestedNodeId(props.node)
  const defaultZone = props.node.metadata?.requested_zone || ''
  const defaultMesh = props.node.metadata?.requested_mesh_id || props.node.temp_mesh_id || ''
  const defaultGreenhouseId =
    props.node.metadata?.greenhouse_id ||
    props.node.metadata?.requested_greenhouse_id ||
    setupStore.targetGreenhouseId ||
    greenhousesStore.selectedId ||
    null

  rootForm.nodeId = defaultNodeId
  rootForm.zone = defaultZone
  rootForm.meshId = defaultMesh
  rootForm.mqttHost = props.node.metadata?.mqtt_host || rootForm.mqttHost
  rootForm.mqttPort = props.node.metadata?.mqtt_port || rootForm.mqttPort
  rootForm.wifiSsid = props.node.metadata?.wifi_ssid || ''
  rootForm.wifiPassword = props.node.metadata?.wifi_password || ''
  rootForm.rootNodeId = props.node.metadata?.root_node_id || defaultNodeId
  rootForm.zoneLocation = props.node.metadata?.zone_location || ''
  rootForm.greenhouseId = defaultGreenhouseId

  regularForm.nodeId = defaultNodeId
  regularForm.zone = defaultZone
  regularForm.meshId = defaultMesh
  regularForm.rootNodeId = props.node.metadata?.root_node_id || ''
  regularForm.notes = ''
  regularForm.greenhouseId = defaultGreenhouseId

  // Apply zone defaults
  if (defaultZone) {
    const zone = props.zones?.find(item => item.name === defaultZone)
    if (zone) {
      if (props.node.is_root) {
        rootForm.meshId = zone.mesh_network_id || rootForm.meshId
        rootForm.rootNodeId = zone.root_node_id || rootForm.rootNodeId
        if (zone.greenhouse_id != null) {
          rootForm.greenhouseId = zone.greenhouse_id
        }
      } else {
        regularForm.meshId = zone.mesh_network_id || regularForm.meshId
        regularForm.rootNodeId = zone.root_node_id || regularForm.rootNodeId
        if (zone.greenhouse_id != null) {
          regularForm.greenhouseId = zone.greenhouse_id
        }
      }
    }
  }
}

function createSuggestedNodeId(node: NewNode): string {
  if (node.is_root) {
    return node.metadata?.suggested_node_id || `root_${node.pin?.toLowerCase()}`
  }
  const prefix = node.node_type?.replace(/[^a-zA-Z0-9]/g, '_') || 'node'
  return `${prefix}_${node.pin?.toLowerCase()}`
}

function nextStep() {
  if (step.value === maxStep.value) return

  if (step.value === maxStep.value - 1) {
    // Validate before moving to summary
    validateForm().then((valid) => {
      if (valid) {
        step.value += 1
      }
    })
  } else {
    step.value += 1
  }
}

function prevStep() {
  if (step.value > 1) {
    step.value -= 1
  }
}

async function validateForm(): Promise<boolean> {
  if (!formRef.value) {
    return true
  }

  const { valid } = await formRef.value.validate?.() ?? { valid: true }
  return valid
}

async function submit() {
  const isValid = await validateForm()
  if (!isValid) return

  const payload: Record<string, any> = {
    node_id: currentForm.value.nodeId?.trim(),
    zone: currentForm.value.zone?.trim(),
    mesh_id: currentForm.value.meshId?.trim(),
  }

  if (props.node?.is_root) {
    payload.mqtt_broker_host = rootForm.mqttHost.trim()
    payload.mqtt_broker_port = Number(rootForm.mqttPort)
    payload.wifi_ssid = rootForm.wifiSsid.trim()
    payload.wifi_password = rootForm.wifiPassword
    if (rootForm.rootNodeId) {
      payload.root_node_id = rootForm.rootNodeId.trim()
    }
    if (rootForm.zoneLocation) {
      payload.zone_location = rootForm.zoneLocation.trim()
    }
  } else if (regularForm.rootNodeId) {
    payload.root_node_id = regularForm.rootNodeId.trim()
  }

  if (!props.node?.is_root && regularForm.notes) {
    payload.metadata = {
      notes: regularForm.notes,
    }
  }

  if (currentForm.value.greenhouseId != null) {
    payload.greenhouse_id = currentForm.value.greenhouseId
    setupStore.setTargetGreenhouseId(currentForm.value.greenhouseId)
    greenhousesStore.selectGreenhouse(currentForm.value.greenhouseId)
  }

  emit('submit', payload)
}

function handleCancel() {
  if (props.loading) return
  internalModel.value = false
}

function copyToClipboard(value: string) {
  if (!navigator?.clipboard || !value) return
  navigator.clipboard.writeText(value).catch(() => {
    // Ignore errors, clipboard may be blocked
  })
}

function greenhouseLabel(id: number | null): string {
  if (id == null) {
    return 'Не выбрано'
  }
  const option = greenhouseOptions.value.find((item) => item.value === id)
  return option?.label || `ID ${id}`
}
</script>

