<template>
  <div>
    <!-- Quick Actions based on node type -->
    <v-card v-if="node" class="mb-4">
      <v-card-title>
        <v-icon icon="mdi-lightning-bolt" class="mr-2"></v-icon>
        Быстрые действия
      </v-card-title>
      
      <!-- Debug info -->
      <v-alert type="info" variant="tonal" density="compact" class="ma-2">
        <div class="text-caption">
          <strong>Debug NodeActions:</strong><br>
          isOnline: {{ isOnline }}<br>
          isPumpRunning: {{ isPumpRunning }}<br>
          last_seen_at: {{ node.last_seen_at }}<br>
          online: {{ node.online }}<br>
          is_online: {{ node.is_online }}<br>
          node_id: {{ node.node_id }}<br>
          node_type: {{ node.node_type }}
        </div>
      </v-alert>

      <v-card-text>
        <v-row>
          <!-- pH/EC Node Actions -->
          <template v-if="node.node_type === 'ph_ec' || node.node_type === 'ph'">
            <v-col cols="6" sm="6" md="3">
              <v-btn
                block
                color="primary"
                prepend-icon="mdi-flask"
                @click="openCalibratePh"
                :disabled="!isOnline"
                size="small"
                class="text-none"
              >
                pH
              </v-btn>
            </v-col>

            <v-col cols="6" sm="6" md="3">
              <v-btn
                block
                color="primary"
                prepend-icon="mdi-waves"
                @click="openCalibrateEc"
                :disabled="!isOnline"
                size="small"
                class="text-none"
              >
                EC
              </v-btn>
            </v-col>

            <v-col cols="6" sm="6" md="3">
              <v-btn
                block
                color="secondary"
                prepend-icon="mdi-tune-variant"
                @click="openPidPresets"
                :disabled="!isOnline"
                size="small"
                class="text-none"
              >
                PID
              </v-btn>
            </v-col>

            <v-col cols="6" sm="6" md="3">
              <v-menu>
                <template v-slot:activator="{ props }">
                  <v-btn
                    block
                    color="info"
                    prepend-icon="mdi-pump"
                    v-bind="props"
                    :disabled="!isOnline || isPumpRunning"
                    :loading="isPumpRunning"
                    size="small"
                    class="text-none"
                  >
                    Насосы
                  </v-btn>
                </template>
                <v-list>
                  <v-list-item @click="runPump('ph_up')" :disabled="isPumpRunning">
                    <v-list-item-title>pH Up</v-list-item-title>
                  </v-list-item>
                  <v-list-item @click="runPump('ph_down')" :disabled="isPumpRunning">
                    <v-list-item-title>pH Down</v-list-item-title>
                  </v-list-item>
                  <v-list-item @click="runPump('ec_up')" :disabled="isPumpRunning">
                    <v-list-item-title>EC Up</v-list-item-title>
                  </v-list-item>
                  <v-list-item @click="runPump('water')" :disabled="isPumpRunning">
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

          <!-- System Control -->
          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="info"
              prepend-icon="mdi-cog"
              @click="openSystemControl"
              size="small"
              class="text-none"
            >
              Система
            </v-btn>
          </v-col>

          <!-- Mock Control -->
          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              :color="isMockMode ? 'success' : 'grey'"
              prepend-icon="mdi-flask-outline"
              @click="toggleMockMode"
              size="small"
              class="text-none"
            >
              {{ isMockMode ? 'Mock ВКЛ' : 'Mock ВЫКЛ' }}
            </v-btn>
          </v-col>

          <!-- Emergency Control -->
          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              :color="isEmergencyMode ? 'error' : 'warning'"
              prepend-icon="mdi-alert-octagon"
              @click="toggleEmergencyMode"
              size="small"
              class="text-none"
            >
              {{ isEmergencyMode ? 'Авария ВКЛ' : 'Авария ВЫКЛ' }}
            </v-btn>
          </v-col>

          <!-- Quick Control Buttons -->
          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="primary"
              prepend-icon="mdi-target"
              @click="quickSetPhTarget"
              size="small"
              class="text-none"
            >
              pH: 6.5
            </v-btn>
          </v-col>

          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="success"
              prepend-icon="mdi-play"
              @click="startAutonomousMode"
              size="small"
              class="text-none"
            >
              Автоном
            </v-btn>
          </v-col>

          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="info"
              prepend-icon="mdi-stop"
              @click="stopAutonomousMode"
              size="small"
              class="text-none"
            >
              Сервер
            </v-btn>
          </v-col>

          <!-- Pump Quick Actions -->
          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="orange"
              prepend-icon="mdi-arrow-up"
              @click="quickPumpUp"
              :disabled="isPumpRunning"
              size="small"
              class="text-none"
            >
              pH UP 5с
            </v-btn>
          </v-col>

          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="blue"
              prepend-icon="mdi-arrow-down"
              @click="quickPumpDown"
              :disabled="isPumpRunning"
              size="small"
              class="text-none"
            >
              pH DOWN 5с
            </v-btn>
          </v-col>

          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="purple"
              prepend-icon="mdi-tune"
              @click="quickCalibrate"
              size="small"
              class="text-none"
            >
              Калибровка
            </v-btn>
          </v-col>

          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="teal"
              prepend-icon="mdi-chart-line"
              @click="getSensorStatus"
              size="small"
              class="text-none"
            >
              Датчики
            </v-btn>
          </v-col>

          <!-- Common Actions -->
          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="grey"
              prepend-icon="mdi-information"
              @click="sendCommand('get_status')"
              size="small"
              class="text-none"
            >
              Статус
            </v-btn>
          </v-col>

          <v-col cols="12" sm="6" md="3">
            <v-btn
              block
              color="warning"
              prepend-icon="mdi-restart"
              @click="confirmReboot"
              size="small"
              class="text-none"
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
    <v-dialog v-model="pumpDialog" max-width="500">
      <v-card>
        <v-card-title>
          <v-icon icon="mdi-pump" class="mr-2"></v-icon>
          {{ isCalibrationMode ? 'Калибровка насоса' : 'Запуск насоса' }}: {{ selectedPump }}
        </v-card-title>
        <v-card-text>
          <v-text-field
            v-model.number="pumpDuration"
            label="Длительность (секунды)"
            type="number"
            min="1"
            max="300"
            variant="outlined"
            class="mb-3"
          ></v-text-field>
          
          <!-- Калибровка насоса -->
          <v-expand-transition>
            <div v-if="isCalibrationMode">
              <v-divider class="mb-3"></v-divider>
              <h4 class="mb-3">Параметры калибровки</h4>
              
              <v-text-field
                v-model.number="calibrationVolume"
                label="Объем (мл)"
                type="number"
                step="0.1"
                min="0.1"
                max="1000"
                variant="outlined"
                class="mb-3"
                hint="Измерьте точный объем жидкости"
              ></v-text-field>
              
              <v-alert type="info" variant="tonal" density="compact">
                <div class="text-caption">
                  <strong>Инструкция:</strong><br>
                  1. Подготовьте мерный стакан<br>
                  2. Запустите насос на указанное время<br>
                  3. Измерьте точный объем жидкости<br>
                  4. Введите измеренный объем
                </div>
              </v-alert>
            </div>
          </v-expand-transition>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="pumpDialog = false" :disabled="isPumpRunning">Отмена</v-btn>
          <v-btn 
            color="primary" 
            @click="isCalibrationMode ? executeCalibration() : executePump()"
            :disabled="(isCalibrationMode && (!calibrationVolume || calibrationVolume <= 0)) || isPumpRunning"
            :loading="isPumpRunning"
          >
            {{ isCalibrationMode ? 'Калибровать' : (isPumpRunning ? `Запуск... ${remainingSeconds}s` : 'Запустить') }}
          </v-btn>
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

    <!-- System Control Dialog -->
    <v-dialog v-model="systemControlDialog" max-width="600">
      <v-card>
        <v-card-title class="bg-primary text-white">
          <v-icon icon="mdi-cog" class="mr-2"></v-icon>
          Управление системой {{ node?.node_id }}
        </v-card-title>
        
        <v-card-text>
          <!-- System Mode Toggle -->
          <v-card variant="tonal" color="primary" class="mb-4">
            <v-card-title class="text-subtitle-2">
              <v-icon icon="mdi-account-switch" size="small" class="mr-2"></v-icon>
              Режим работы системы
            </v-card-title>
            <v-card-text>
              <v-radio-group v-model="systemMode" inline>
                <v-radio
                  label="Автономный (внутренняя система узла)"
                  value="autonomous"
                  color="success"
                ></v-radio>
                <v-radio
                  label="Серверный (управление через сервер)"
                  value="server"
                  color="info"
                ></v-radio>
              </v-radio-group>
              
              <v-alert type="info" variant="tonal" density="compact" class="mt-2">
                <div class="text-caption">
                  <strong>Автономный:</strong> Узел работает независимо, используя внутренние алгоритмы PID<br>
                  <strong>Серверный:</strong> Узел получает команды от сервера для точного управления
                </div>
              </v-alert>
            </v-card-text>
          </v-card>

          <!-- PID Settings -->
          <v-card variant="tonal" color="secondary" class="mb-4">
            <v-card-title class="text-subtitle-2">
              <v-icon icon="mdi-tune-variant" size="small" class="mr-2"></v-icon>
              Настройки PID
            </v-card-title>
            <v-card-text>
              <v-row>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="pidSettings.ph_target"
                    label="pH Цель"
                    type="number"
                    step="0.1"
                    min="5.0"
                    max="8.0"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="pidSettings.ec_target"
                    label="EC Цель (мСм/см)"
                    type="number"
                    step="0.1"
                    min="0.5"
                    max="3.0"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>

          <!-- Safety Settings -->
          <v-card variant="tonal" color="warning" class="mb-4">
            <v-card-title class="text-subtitle-2">
              <v-icon icon="mdi-shield-check" size="small" class="mr-2"></v-icon>
              Настройки безопасности
            </v-card-title>
            <v-card-text>
              <v-row>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="safetySettings.max_pump_time"
                    label="Макс. время насоса (сек)"
                    type="number"
                    min="1"
                    max="300"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="safetySettings.cooldown"
                    label="Интервал между дозами (сек)"
                    type="number"
                    min="1"
                    max="60"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>
        </v-card-text>
        
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="systemControlDialog = false">Отмена</v-btn>
          <v-btn
            color="primary"
            prepend-icon="mdi-send"
            @click="applySystemSettings"
            :loading="applyingSettings"
          >
            Применить настройки
          </v-btn>
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
import { ref, computed, watch } from 'vue'
import { useRouter } from 'vue-router'
import api from '@/services/api'
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
const calibrationVolume = ref(10) // мл
const isCalibrationMode = ref(false)
const isPumpRunning = ref(false)
const remainingSeconds = ref(0)
let pumpIntervalId = null

// Отслеживаем изменения isPumpRunning
watch(isPumpRunning, (newVal, oldVal) => {
  console.log('🔄 isPumpRunning изменился:', oldVal, '->', newVal)
})

// PID Presets
const pidPresetsDialog = ref(false)
const loadingPresets = ref(false)
const sendingPreset = ref(false)
const defaultPresets = ref([])
const customPresets = ref([])
const selectedPreset = ref(null)

// System Control
const systemControlDialog = ref(false)
const systemMode = ref('autonomous')
const isMockMode = ref(false)
const isEmergencyMode = ref(false)
const applyingSettings = ref(false)

// PID Settings
const pidSettings = ref({
  ph_target: 6.5,
  ec_target: 1.2
})

// Safety Settings
const safetySettings = ref({
  max_pump_time: 30,
  cooldown: 5
})

const isOnline = computed(() => {
  console.log('🔍 NodeActions isOnline check:', {
    node: props.node,
    is_online: props.node?.is_online,
    online: props.node?.online,
    last_seen_at: props.node?.last_seen_at
  })
  
  // Приоритет: backend поля is_online и online
  if (props.node?.is_online !== undefined) {
    console.log('✅ Using is_online:', props.node.is_online)
    return props.node.is_online
  }
  if (props.node?.online !== undefined) {
    console.log('✅ Using online:', props.node.online)
    return props.node.online
  }
  
  // Fallback: проверяем по last_seen_at
  if (props.node?.last_seen_at) {
    const lastSeen = new Date(props.node.last_seen_at)
    const seconds = (Date.now() - lastSeen.getTime()) / 1000
    const online = seconds < 60 // Увеличиваем таймаут до 60 секунд
    console.log('🔍 Проверка isOnline по last_seen_at:', {
      last_seen_at: props.node.last_seen_at,
      seconds: seconds,
      online: online
    })
    return online
  }
  
  // По умолчанию считаем офлайн
  console.log('❌ Node offline - no data')
  return false
})

function sendCommand(command, params = {}) {
  emit('command', { command, params })
}

function openCalibratePh() {
  // Открываем диалог калибровки насоса pH
  selectedPump.value = 'ph_up'
  pumpDuration.value = 5
  calibrationVolume.value = 10
  isCalibrationMode.value = true
  pumpDialog.value = true
}

function openCalibrateEc() {
  // Открываем диалог калибровки насоса EC
  selectedPump.value = 'ec_up'
  pumpDuration.value = 5
  calibrationVolume.value = 10
  isCalibrationMode.value = true
  pumpDialog.value = true
}

function runPump(pump) {
  selectedPump.value = pump
  pumpDuration.value = 5
  isCalibrationMode.value = false
  pumpDialog.value = true
}

async function executePump() {
  try {
    console.log('🚀 executePump вызван, isPumpRunning:', isPumpRunning.value)
    if (isPumpRunning.value) {
      console.log('⚠️ Насос уже запущен, игнорируем запрос')
      return
    }
    
    // Определяем pump_id на основе типа насоса
    const pumpIdMap = {
      'ph_up': 0,
      'ph_down': 1,
      'ec_up': 2,
      'ec_down': 3,
      'water': 4
    }
    
    const pumpId = pumpIdMap[selectedPump.value] || 0
    console.log('🔧 Запуск насоса:', selectedPump.value, 'pump_id:', pumpId, 'duration:', pumpDuration.value)
    
    // Отправляем запрос на запуск насоса
    const response = await api.post(`/nodes/${props.node.node_id}/pump/run`, {
      pump_id: pumpId,
      duration_sec: pumpDuration.value
    })
    
    console.log('📡 Ответ сервера:', response.data)
    
    if (response.data.success) {
      console.log('✅ Насос успешно запущен')
      // Блокируем кнопки и показываем загрузку на время выполнения
      isPumpRunning.value = true
      remainingSeconds.value = Math.max(1, Math.round(Number(pumpDuration.value) || 0))
      console.log('⏰ Устанавливаем таймер на', remainingSeconds.value, 'секунд')
      
      if (pumpIntervalId) clearInterval(pumpIntervalId)
      pumpIntervalId = setInterval(() => {
        if (remainingSeconds.value > 0) {
          remainingSeconds.value -= 1
          console.log('⏱️ Осталось секунд:', remainingSeconds.value)
        }
        if (remainingSeconds.value <= 0) {
          console.log('🏁 Таймер завершен, разблокируем кнопки')
          clearInterval(pumpIntervalId)
          pumpIntervalId = null
          isPumpRunning.value = false
          pumpDialog.value = false
        }
      }, 1000)
    } else {
      console.log('❌ Сервер вернул success: false')
    }
  } catch (error) {
    console.error('❌ Ошибка запуска насоса:', error)
    console.error('❌ Детали ошибки:', {
      message: error.message,
      response: error.response?.data,
      status: error.response?.status,
      config: error.config
    })
    // Показываем уведомление об ошибке
    isPumpRunning.value = false
  }
}

async function executeCalibration() {
  try {
    // Определяем pump_id на основе типа насоса
    const pumpIdMap = {
      'ph_up': 0,
      'ph_down': 1,
      'ec_up': 2,
      'ec_down': 3,
      'water': 4
    }
    
    const pumpId = pumpIdMap[selectedPump.value] || 0
    
    // Отправляем запрос на калибровку
    const response = await api.post(`/nodes/${props.node.node_id}/pump/calibrate`, {
      pump_id: pumpId,
      duration_sec: pumpDuration.value,
      volume_ml: calibrationVolume.value
    })
    
    if (response.data.success) {
      console.log('✅ Калибровка насоса успешно сохранена')
      // Показываем уведомление об успехе
      // Можно добавить snackbar здесь
    }
    
    pumpDialog.value = false
  } catch (error) {
    console.error('❌ Ошибка калибровки насоса:', error)
    // Показываем уведомление об ошибке
    // Можно добавить snackbar здесь
  }
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

// System Control Functions
function openSystemControl() {
  // Загружаем текущие настройки узла
  loadNodeSettings()
  systemControlDialog.value = true
}

async function loadNodeSettings() {
  try {
    // Получаем текущую конфигурацию узла
    const response = await api.get(`/nodes/${props.node.node_id}`)
    const nodeData = response.data
    
    if (nodeData.config) {
      // Загружаем настройки из конфигурации
      systemMode.value = nodeData.config.autonomous_enabled ? 'autonomous' : 'server'
      pidSettings.value.ph_target = nodeData.config.ph_target || 6.5
      pidSettings.value.ec_target = nodeData.config.ec_target || 1.2
      safetySettings.value.max_pump_time = nodeData.config.max_pump_time_ms ? nodeData.config.max_pump_time_ms / 1000 : 30
      safetySettings.value.cooldown = nodeData.config.cooldown_ms ? nodeData.config.cooldown_ms / 1000 : 5
    }
    
    // Проверяем статус mock режима
    const statusResponse = await api.post(`/nodes/${props.node.node_id}/command`, {
      command: 'get_sensor_status',
      params: {}
    })
    
    if (statusResponse.data.success) {
      isMockMode.value = statusResponse.data.data?.mock_mode || false
      isEmergencyMode.value = statusResponse.data.data?.emergency || false
    }
  } catch (error) {
    console.error('Ошибка загрузки настроек узла:', error)
  }
}

async function applySystemSettings() {
  try {
    applyingSettings.value = true
    
    // Отправляем команду изменения режима
    if (systemMode.value === 'autonomous') {
      await sendCommand('set_autonomous_mode', { enable: true })
    } else {
      await sendCommand('set_autonomous_mode', { enable: false })
    }
    
    // Отправляем настройки PID
    await sendCommand('set_ph_target', { ph_target: pidSettings.value.ph_target })
    await sendCommand('set_ec_target', { ec_target: pidSettings.value.ec_target })
    
    // Отправляем настройки безопасности
    await sendCommand('set_safety_settings', {
      max_pump_time_ms: safetySettings.value.max_pump_time * 1000,
      cooldown_ms: safetySettings.value.cooldown * 1000
    })
    
    console.log('✅ Настройки системы применены')
    systemControlDialog.value = false
    
  } catch (error) {
    console.error('❌ Ошибка применения настроек:', error)
  } finally {
    applyingSettings.value = false
  }
}

// Mock Mode Control
async function toggleMockMode() {
  try {
    const newMockMode = !isMockMode.value
    await sendCommand('force_mock_mode', { enable: newMockMode })
    isMockMode.value = newMockMode
    console.log(`✅ Mock режим ${newMockMode ? 'включен' : 'выключен'}`)
  } catch (error) {
    console.error('❌ Ошибка переключения mock режима:', error)
  }
}

// Emergency Mode Control
async function toggleEmergencyMode() {
  try {
    const newEmergencyMode = !isEmergencyMode.value
    if (newEmergencyMode) {
      await sendCommand('emergency_stop', {})
    } else {
      await sendCommand('reset_emergency', {})
    }
    isEmergencyMode.value = newEmergencyMode
    console.log(`✅ Аварийный режим ${newEmergencyMode ? 'включен' : 'выключен'}`)
  } catch (error) {
    console.error('❌ Ошибка переключения аварийного режима:', error)
  }
}

// Quick Control Functions
async function quickSetPhTarget() {
  try {
    // Устанавливаем стандартную pH цель 6.5
    await sendCommand('set_ph_target', { ph_target: 6.5 })
    console.log('✅ pH цель установлена на 6.5')
  } catch (error) {
    console.error('❌ Ошибка установки pH цели:', error)
  }
}

async function startAutonomousMode() {
  try {
    await sendCommand('set_autonomous_mode', { enable: true })
    systemMode.value = 'autonomous'
    console.log('✅ Автономный режим включен')
  } catch (error) {
    console.error('❌ Ошибка включения автономного режима:', error)
  }
}

async function stopAutonomousMode() {
  try {
    await sendCommand('set_autonomous_mode', { enable: false })
    systemMode.value = 'server'
    console.log('✅ Серверный режим включен')
  } catch (error) {
    console.error('❌ Ошибка включения серверного режима:', error)
  }
}

// Pump Quick Actions
async function quickPumpUp() {
  try {
    selectedPump.value = 'ph_up'
    pumpDuration.value = 5
    isCalibrationMode.value = false
    await executePump()
    console.log('✅ pH UP насос запущен на 5 секунд')
  } catch (error) {
    console.error('❌ Ошибка запуска pH UP насоса:', error)
  }
}

async function quickPumpDown() {
  try {
    selectedPump.value = 'ph_down'
    pumpDuration.value = 5
    isCalibrationMode.value = false
    await executePump()
    console.log('✅ pH DOWN насос запущен на 5 секунд')
  } catch (error) {
    console.error('❌ Ошибка запуска pH DOWN насоса:', error)
  }
}

async function quickCalibrate() {
  try {
    // Открываем диалог калибровки pH UP
    selectedPump.value = 'ph_up'
    pumpDuration.value = 5
    calibrationVolume.value = 10
    isCalibrationMode.value = true
    pumpDialog.value = true
    console.log('✅ Диалог калибровки открыт')
  } catch (error) {
    console.error('❌ Ошибка открытия калибровки:', error)
  }
}

async function getSensorStatus() {
  try {
    await sendCommand('get_sensor_status', {})
    console.log('✅ Запрос статуса датчиков отправлен')
  } catch (error) {
    console.error('❌ Ошибка получения статуса датчиков:', error)
  }
}
</script>

