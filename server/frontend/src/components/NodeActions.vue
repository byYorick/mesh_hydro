<template>
  <div>
    <!-- Quick Actions based on node type -->
    <v-card v-if="node" class="mb-4">
      <v-card-title>
        <v-icon icon="mdi-lightning-bolt" class="mr-2"></v-icon>
        Быстрые действия
      </v-card-title>

      <v-card-text>
        <v-row>
          <!-- pH/EC Node Actions -->
          <template v-if="node.node_type === 'ph_ec'">
            <v-col cols="12" sm="6" md="3">
              <v-btn
                block
                color="primary"
                prepend-icon="mdi-flask"
                @click="openCalibratePh"
                :disabled="!isOnline"
                size="default"
                class="text-none"
              >
                pH калибр.
              </v-btn>
            </v-col>

            <v-col cols="12" sm="6" md="3">
              <v-btn
                block
                color="primary"
                prepend-icon="mdi-waves"
                @click="openCalibrateEc"
                :disabled="!isOnline"
                size="default"
                class="text-none"
              >
                EC калибр.
              </v-btn>
            </v-col>

            <v-col cols="12" sm="6" md="3">
              <v-btn
                block
                color="secondary"
                prepend-icon="mdi-tune-variant"
                @click="openPidPresets"
                :disabled="!isOnline"
                size="default"
                class="text-none"
              >
                PID настройки
              </v-btn>
            </v-col>

            <v-col cols="12" sm="6" md="3">
              <v-menu>
                <template v-slot:activator="{ props }">
                  <v-btn
                    block
                    color="info"
                    prepend-icon="mdi-pump"
                    v-bind="props"
                    :disabled="!isOnline"
                    size="default"
                    class="text-none"
                  >
                    Насосы
                  </v-btn>
                </template>
                <v-list>
                  <v-list-item @click="runPump('ph_up')">
                    <v-list-item-title>pH Up</v-list-item-title>
                  </v-list-item>
                  <v-list-item @click="runPump('ph_down')">
                    <v-list-item-title>pH Down</v-list-item-title>
                  </v-list-item>
                  <v-list-item @click="runPump('ec_up')">
                    <v-list-item-title>EC Up</v-list-item-title>
                  </v-list-item>
                  <v-list-item @click="runPump('water')">
                    <v-list-item-title>Вода</v-list-item-title>
                  </v-list-item>
                </v-list>
              </v-menu>
            </v-col>
          </template>

          <!-- Relay Node Actions -->
          <template v-if="node.node_type === 'relay'">
            <v-col cols="12" sm="6" md="3">
              <v-btn
                block
                color="success"
                prepend-icon="mdi-window-open"
                @click="sendCommand('open_all')"
                :disabled="!isOnline"
              >
                Открыть все
              </v-btn>
            </v-col>

            <v-col cols="12" sm="6" md="3">
              <v-btn
                block
                color="warning"
                prepend-icon="mdi-window-closed"
                @click="sendCommand('close_all')"
                :disabled="!isOnline"
              >
                Закрыть все
              </v-btn>
            </v-col>

            <v-col cols="12" sm="6" md="3">
              <v-btn
                block
                color="info"
                prepend-icon="mdi-fan"
                @click="toggleRelay(3)"
                :disabled="!isOnline"
              >
                Вентилятор
              </v-btn>
            </v-col>

            <v-col cols="12" sm="6" md="3">
              <v-btn
                block
                color="error"
                prepend-icon="mdi-fire"
                @click="toggleRelay(4)"
                :disabled="!isOnline"
              >
                Обогрев
              </v-btn>
            </v-col>
          </template>

          <!-- Climate Node Actions -->
          <template v-if="node.node_type === 'climate'">
            <v-col cols="12" sm="6">
              <v-btn
                block
                color="primary"
                prepend-icon="mdi-refresh"
                @click="sendCommand('update_sensors')"
                :disabled="!isOnline"
              >
                Обновить датчики
              </v-btn>
            </v-col>
          </template>

          <!-- Common Actions -->
          <v-col cols="12" sm="6" md="4">
            <v-btn
              block
              color="grey"
              prepend-icon="mdi-information"
              @click="sendCommand('get_status')"
              :disabled="!isOnline"
            >
              Получить статус
            </v-btn>
          </v-col>

          <v-col cols="12" sm="6" md="4">
            <v-btn
              block
              color="warning"
              prepend-icon="mdi-restart"
              @click="confirmReboot"
              :disabled="!isOnline"
            >
              Перезагрузка
            </v-btn>
          </v-col>

          <v-col cols="12" sm="6" md="4">
            <ConfigEditor :node="node" @config-updated="updateConfig">
              <template v-slot:activator="{ props }">
                <v-btn
                  block
                  color="secondary"
                  prepend-icon="mdi-cog"
                  v-bind="props"
                >
                  Настройки
                </v-btn>
              </template>
            </ConfigEditor>
          </v-col>
        </v-row>
      </v-card-text>
    </v-card>

    <!-- Pump Duration Dialog -->
    <v-dialog v-model="pumpDialog" max-width="400">
      <v-card>
        <v-card-title>Запуск насоса: {{ selectedPump }}</v-card-title>
        <v-card-text>
          <v-text-field
            v-model.number="pumpDuration"
            label="Длительность (секунды)"
            type="number"
            min="1"
            max="300"
            variant="outlined"
          ></v-text-field>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="pumpDialog = false">Отмена</v-btn>
          <v-btn color="primary" @click="executePump">Запустить</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Reboot Confirmation -->
    <v-dialog v-model="rebootDialog" max-width="400">
      <v-card>
        <v-card-title class="text-warning">
          <v-icon icon="mdi-alert" class="mr-2"></v-icon>
          Подтверждение
        </v-card-title>
        <v-card-text>
          Вы уверены что хотите перезагрузить узел <strong>{{ node?.node_id }}</strong>?
          <br><br>
          Узел будет недоступен ~30 секунд.
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="rebootDialog = false">Отмена</v-btn>
          <v-btn color="warning" @click="executeReboot">Перезагрузить</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- PID Presets Dialog -->
    <v-dialog v-model="pidPresetsDialog" max-width="700" scrollable>
      <v-card>
        <v-card-title class="bg-secondary">
          <v-icon icon="mdi-tune-variant" class="mr-2"></v-icon>
          Расширенные настройки PID для {{ node?.node_id }}
        </v-card-title>
        
        <v-card-text class="pt-4">
          <v-alert type="info" variant="tonal" class="mb-4">
            Выберите пресет PID настроек для отправки на узел. Настройки будут сразу применены к pH и EC контроллерам.
          </v-alert>

          <v-row v-if="loadingPresets">
            <v-col cols="12" class="text-center py-8">
              <v-progress-circular indeterminate color="primary"></v-progress-circular>
              <div class="mt-2">Загрузка пресетов...</div>
            </v-col>
          </v-row>

          <v-row v-else>
            <!-- Default Presets -->
            <v-col cols="12" v-if="defaultPresets.length > 0">
              <div class="text-subtitle-2 mb-3">
                <v-icon icon="mdi-star-settings" size="small" class="mr-2"></v-icon>
                Предустановленные пресеты
              </div>
              <v-row dense>
                <v-col cols="12" sm="6" v-for="preset in defaultPresets" :key="preset.id">
                  <v-card
                    :variant="selectedPreset?.id === preset.id ? 'tonal' : 'outlined'"
                    :color="selectedPreset?.id === preset.id ? 'primary' : ''"
                    class="preset-card"
                    @click="selectedPreset = preset"
                    hover
                  >
                    <v-card-text>
                      <div class="d-flex align-center justify-space-between">
                        <div>
                          <div class="text-subtitle-1 font-weight-bold">{{ preset.name }}</div>
                          <div class="text-caption text-grey mt-1">{{ preset.description }}</div>
                        </div>
                        <v-icon 
                          v-if="selectedPreset?.id === preset.id" 
                          icon="mdi-check-circle" 
                          color="primary"
                        ></v-icon>
                      </div>
                      <v-divider class="my-2"></v-divider>
                      <div class="text-caption">
                        <v-chip size="x-small" class="mr-1">pH: {{ preset.ph_config.setpoint }}</v-chip>
                        <v-chip size="x-small" class="mr-1">EC: {{ preset.ec_config.setpoint }}</v-chip>
                        <v-chip size="x-small">Kp: {{ preset.ph_config.kp }}</v-chip>
                      </div>
                    </v-card-text>
                  </v-card>
                </v-col>
              </v-row>
            </v-col>

            <!-- Custom Presets -->
            <v-col cols="12" v-if="customPresets.length > 0">
              <v-divider class="my-3"></v-divider>
              <div class="text-subtitle-2 mb-3">
                <v-icon icon="mdi-star" size="small" class="mr-2"></v-icon>
                Мои пресеты
              </div>
              <v-row dense>
                <v-col cols="12" sm="6" v-for="preset in customPresets" :key="preset.id">
                  <v-card
                    :variant="selectedPreset?.id === preset.id ? 'tonal' : 'outlined'"
                    :color="selectedPreset?.id === preset.id ? 'purple' : ''"
                    class="preset-card"
                    @click="selectedPreset = preset"
                    hover
                  >
                    <v-card-text>
                      <div class="d-flex align-center justify-space-between">
                        <div>
                          <div class="text-subtitle-1 font-weight-bold">{{ preset.name }}</div>
                          <div class="text-caption text-grey mt-1">{{ preset.description }}</div>
                        </div>
                        <v-icon 
                          v-if="selectedPreset?.id === preset.id" 
                          icon="mdi-check-circle" 
                          color="purple"
                        ></v-icon>
                      </div>
                      <v-divider class="my-2"></v-divider>
                      <div class="text-caption">
                        <v-chip size="x-small" class="mr-1">pH: {{ preset.ph_config.setpoint }}</v-chip>
                        <v-chip size="x-small" class="mr-1">EC: {{ preset.ec_config.setpoint }}</v-chip>
                        <v-chip size="x-small">Kp: {{ preset.ph_config.kp }}</v-chip>
                      </div>
                    </v-card-text>
                  </v-card>
                </v-col>
              </v-row>
            </v-col>

            <v-col cols="12" v-if="!loadingPresets && defaultPresets.length === 0 && customPresets.length === 0">
              <v-alert type="warning" variant="tonal">
                Нет доступных пресетов. Создайте пресет в разделе Настройки → PID Контроллеры.
              </v-alert>
            </v-col>
          </v-row>

          <!-- Selected Preset Details -->
          <v-expand-transition>
            <v-card v-if="selectedPreset" variant="tonal" color="primary" class="mt-4">
              <v-card-title class="text-subtitle-2">
                <v-icon icon="mdi-information" size="small" class="mr-2"></v-icon>
                Детали выбранного пресета
              </v-card-title>
              <v-card-text>
                <v-row dense>
                  <v-col cols="6">
                    <div class="text-caption text-grey">pH Контроллер:</div>
                    <div class="text-body-2">
                      Setpoint: {{ selectedPreset.ph_config.setpoint }}<br>
                      Kp: {{ selectedPreset.ph_config.kp }}, Ki: {{ selectedPreset.ph_config.ki }}, Kd: {{ selectedPreset.ph_config.kd }}
                    </div>
                  </v-col>
                  <v-col cols="6">
                    <div class="text-caption text-grey">EC Контроллер:</div>
                    <div class="text-body-2">
                      Setpoint: {{ selectedPreset.ec_config.setpoint }}<br>
                      Kp: {{ selectedPreset.ec_config.kp }}, Ki: {{ selectedPreset.ec_config.ki }}, Kd: {{ selectedPreset.ec_config.kd }}
                    </div>
                  </v-col>
                </v-row>
              </v-card-text>
            </v-card>
          </v-expand-transition>
        </v-card-text>
        
        <v-card-actions>
          <v-btn
            prepend-icon="mdi-cog"
            @click="goToSettings"
            variant="outlined"
          >
            Управление пресетами
          </v-btn>
          <v-spacer></v-spacer>
          <v-btn @click="pidPresetsDialog = false">Отмена</v-btn>
          <v-btn
            color="primary"
            prepend-icon="mdi-send"
            @click="sendPresetToNode"
            :disabled="!selectedPreset"
            :loading="sendingPreset"
          >
            Отправить на узел
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </div>
</template>

<style scoped>
.preset-card {
  cursor: pointer;
  transition: all 0.2s;
}

.preset-card:hover {
  transform: translateY(-2px);
}
</style>

<script setup>
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { axios as api } from '@/services/api'
import ConfigEditor from './ConfigEditor.vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['command', 'config-update'])

const router = useRouter()

const config = ref({})
const pumpDialog = ref(false)
const rebootDialog = ref(false)
const selectedPump = ref(null)
const pumpDuration = ref(5)

// PID Presets
const pidPresetsDialog = ref(false)
const loadingPresets = ref(false)
const sendingPreset = ref(false)
const defaultPresets = ref([])
const customPresets = ref([])
const selectedPreset = ref(null)

const isOnline = computed(() => {
  return props.node?.online || props.node?.is_online
})

function sendCommand(command, params = {}) {
  emit('command', { command, params })
}

function openCalibratePh() {
  // Open command dialog with calibrate_ph preset
  sendCommand('calibrate_ph', { ph_value: 7.0 })
}

function openCalibrateEc() {
  sendCommand('calibrate_ec', { ec_value: 1.413 })
}

function runPump(pump) {
  selectedPump.value = pump
  pumpDuration.value = 5
  pumpDialog.value = true
}

function executePump() {
  sendCommand('run_pump', {
    pump: selectedPump.value,
    duration: pumpDuration.value,
  })
  pumpDialog.value = false
}

function toggleRelay(relayId) {
  sendCommand('toggle_relay', { relay_id: relayId })
}

function confirmReboot() {
  rebootDialog.value = true
}

function executeReboot() {
  sendCommand('reboot')
  rebootDialog.value = false
}

function updateConfig(newConfig) {
  emit('config-update', newConfig)
}

// PID Presets Functions
async function openPidPresets() {
  pidPresetsDialog.value = true
  await loadPresets()
}

async function loadPresets() {
  loadingPresets.value = true
  try {
    const data = await api.get('/pid-presets')
    
    if (data && Array.isArray(data)) {
      defaultPresets.value = data.filter(p => p.is_default)
      customPresets.value = data.filter(p => !p.is_default)
      console.log(`✅ Загружено ${defaultPresets.value.length} дефолтных и ${customPresets.value.length} пользовательских пресетов`)
    }
  } catch (error) {
    console.error('Error loading PID presets:', error)
  } finally {
    loadingPresets.value = false
  }
}

function sendPresetToNode() {
  if (!selectedPreset.value) return
  
  sendingPreset.value = true
  
  console.log('📤 Отправка пресета на узел:', props.node.node_id, selectedPreset.value.name)
  
  // Отправляем команду set_pid_config с настройками pH и EC
  sendCommand('set_pid_config', {
    ph: selectedPreset.value.ph_config,
    ec: selectedPreset.value.ec_config,
    preset_name: selectedPreset.value.name
  })
  
  console.log('✅ Команда отправлена на узел')
  
  // Задержка для UX, затем закрываем диалог
  setTimeout(() => {
    pidPresetsDialog.value = false
    selectedPreset.value = null
    sendingPreset.value = false
  }, 500)
}

function goToSettings() {
  pidPresetsDialog.value = false
  router.push({ name: 'Settings', hash: '#pid' })
}
</script>

