<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon :icon="nodeIcon" class="mr-2"></v-icon>
      Панель управления узлом: {{ node?.node_id }}
      <v-spacer></v-spacer>
      <v-chip :color="statusColor" size="small">
        {{ statusText }}
      </v-chip>
    </v-card-title>

    <v-card-text>
      <!-- Node Status Overview -->
      <v-row>
        <v-col cols="12" md="3">
          <v-card variant="outlined" class="h-100">
            <v-card-title class="text-subtitle-1">Статус</v-card-title>
            <v-card-text>
              <div class="d-flex align-center">
                <v-icon :color="statusColor" size="large" class="mr-2">
                  {{ statusIcon }}
                </v-icon>
                <div>
                  <div class="font-weight-bold">{{ statusText }}</div>
                  <div class="text-caption">{{ lastSeenText }}</div>
                </div>
              </div>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="3">
          <v-card variant="outlined" class="h-100">
            <v-card-title class="text-subtitle-1">Память</v-card-title>
            <v-card-text>
              <div v-if="hasMemoryInfo">
                <div class="d-flex justify-space-between mb-1">
                  <span class="text-caption">RAM</span>
                  <span class="text-caption">{{ heapPercent.toFixed(0) }}%</span>
                </div>
                <v-progress-linear
                  :model-value="heapPercent"
                  :color="getMemoryColor(heapPercent)"
                  height="6"
                  rounded
                ></v-progress-linear>
                <div class="text-caption mt-1">
                  {{ formatBytes(heapUsed) }} / {{ formatBytes(heapTotal) }}
                </div>
              </div>
              <div v-else class="text-caption text-disabled">
                Нет данных
              </div>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="3">
          <v-card variant="outlined" class="h-100">
            <v-card-title class="text-subtitle-1">WiFi</v-card-title>
            <v-card-text>
              <div v-if="hasRssi">
                <div class="d-flex justify-space-between mb-1">
                  <span class="text-caption">Сигнал</span>
                  <span class="text-caption">{{ rssiValue }} dBm</span>
                </div>
                <v-progress-linear
                  :model-value="rssiPercent"
                  :color="getRssiColor(rssiPercent)"
                  height="6"
                  rounded
                ></v-progress-linear>
                <div class="text-caption mt-1">
                  {{ rssiDescription }}
                </div>
              </div>
              <div v-else class="text-caption text-disabled">
                Нет данных
              </div>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" md="3">
          <v-card variant="outlined" class="h-100">
            <v-card-title class="text-subtitle-1">Ошибки</v-card-title>
            <v-card-text>
              <div class="d-flex align-center">
                <v-icon 
                  :color="errorCount > 0 ? 'error' : 'success'" 
                  size="large" 
                  class="mr-2"
                >
                  {{ errorCount > 0 ? 'mdi-alert-circle' : 'mdi-check-circle' }}
                </v-icon>
                <div>
                  <div class="font-weight-bold">{{ errorCount }}</div>
                  <div class="text-caption">активных ошибок</div>
                </div>
              </div>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>

      <!-- Node Specific Controls -->
      <v-divider class="my-4"></v-divider>

      <!-- pH Node Controls -->
      <div v-if="node?.node_type === 'ph'">
        <h3 class="text-h6 mb-3">Управление pH</h3>
        
        <v-row>
          <!-- Current Values -->
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Текущие значения</v-card-title>
              <v-card-text>
                <div class="d-flex justify-space-between mb-2">
                  <span>pH:</span>
                  <span class="font-weight-bold">{{ lastData?.ph?.toFixed(2) || '-' }}</span>
                </div>
                <div class="d-flex justify-space-between mb-2">
                  <span>Температура:</span>
                  <span class="font-weight-bold">{{ lastData?.temp?.toFixed(1) || '-' }}°C</span>
                </div>
                <div class="d-flex justify-space-between">
                  <span>Статус:</span>
                  <span class="font-weight-bold">{{ pHStatus }}</span>
                </div>
              </v-card-text>
            </v-card>
          </v-col>

          <!-- Target Values -->
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Целевые значения</v-card-title>
              <v-card-text>
                <div class="d-flex justify-space-between mb-2">
                  <span>pH:</span>
                  <span class="font-weight-bold">{{ node?.config?.ph_target || '-' }}</span>
                </div>
                <div class="d-flex justify-space-between mb-2">
                  <span>Диапазон:</span>
                  <span class="font-weight-bold">
                    {{ node?.config?.ph_min || '-' }} - {{ node?.config?.ph_max || '-' }}
                  </span>
                </div>
                <div class="d-flex justify-space-between">
                  <span>Допуск:</span>
                  <span class="font-weight-bold">{{ node?.config?.ph_tolerance || '-' }}</span>
                </div>
              </v-card-text>
            </v-card>
          </v-col>

          <!-- Quick Actions -->
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Быстрые действия</v-card-title>
              <v-card-text>
                <v-row dense>
                  <v-col cols="6">
                    <v-btn
                      block
                      color="primary"
                      size="small"
                      @click="runPump('ph_up', 5)"
                      :disabled="!canRunPumps"
                    >
                      pH+ (5с)
                    </v-btn>
                  </v-col>
                  <v-col cols="6">
                    <v-btn
                      block
                      color="primary"
                      size="small"
                      @click="runPump('ph_down', 5)"
                      :disabled="!canRunPumps"
                    >
                      pH- (5с)
                    </v-btn>
                  </v-col>
                </v-row>
                
                <v-btn
                  block
                  color="error"
                  class="mt-2"
                  @click="sendCommand('emergency_stop')"
                  :disabled="!canPerformActions"
                >
                  Экстренная остановка
                </v-btn>
                
                <v-btn
                  block
                  color="warning"
                  class="mt-2"
                  @click="openCalibration"
                >
                  Калибровка
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </div>

      <!-- EC Node Controls -->
      <div v-else-if="node?.node_type === 'ec'">
        <h3 class="text-h6 mb-3">Управление EC</h3>
        
        <v-row>
          <!-- Current Values -->
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Текущие значения</v-card-title>
              <v-card-text>
                <div class="d-flex justify-space-between mb-2">
                  <span>EC:</span>
                  <span class="font-weight-bold">{{ lastData?.ec?.toFixed(2) || '-' }} mS/cm</span>
                </div>
                <div class="d-flex justify-space-between mb-2">
                  <span>Температура:</span>
                  <span class="font-weight-bold">{{ lastData?.temp?.toFixed(1) || '-' }}°C</span>
                </div>
                <div class="d-flex justify-space-between">
                  <span>Статус:</span>
                  <span class="font-weight-bold">{{ ecStatus }}</span>
                </div>
              </v-card-text>
            </v-card>
          </v-col>

          <!-- Target Values -->
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Целевые значения</v-card-title>
              <v-card-text>
                <div class="d-flex justify-space-between mb-2">
                  <span>EC:</span>
                  <span class="font-weight-bold">{{ node?.config?.ec_target || '-' }} mS/cm</span>
                </div>
                <div class="d-flex justify-space-between mb-2">
                  <span>Диапазон:</span>
                  <span class="font-weight-bold">
                    {{ node?.config?.ec_min || '-' }} - {{ node?.config?.ec_max || '-' }} mS/cm
                  </span>
                </div>
                <div class="d-flex justify-space-between">
                  <span>Допуск:</span>
                  <span class="font-weight-bold">{{ node?.config?.ec_tolerance || '-' }}</span>
                </div>
              </v-card-text>
            </v-card>
          </v-col>

          <!-- Quick Actions -->
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Быстрые действия</v-card-title>
              <v-card-text>
                <v-row dense>
                  <v-col cols="6">
                    <v-btn
                      block
                      color="success"
                      size="small"
                      @click="runPump('ec_up', 5)"
                      :disabled="!canRunPumps"
                    >
                      EC+ (5с)
                    </v-btn>
                  </v-col>
                  <v-col cols="6">
                    <v-btn
                      block
                      color="warning"
                      size="small"
                      @click="runPump('ec_down', 5)"
                      :disabled="!canRunPumps"
                    >
                      EC- (5с)
                    </v-btn>
                  </v-col>
                </v-row>
                
                <v-btn
                  block
                  color="error"
                  class="mt-2"
                  @click="sendCommand('emergency_stop')"
                  :disabled="!canPerformActions"
                >
                  Экстренная остановка
                </v-btn>
                
                <v-btn
                  block
                  color="warning"
                  class="mt-2"
                  @click="openCalibration"
                >
                  Калибровка
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </div>

      <!-- Climate Node Controls -->
      <div v-else-if="node?.node_type === 'climate'">
        <h3 class="text-h6 mb-3">Управление климатом</h3>
        
        <v-row>
          <v-col cols="12" md="3">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Температура</v-card-title>
              <v-card-text class="text-center">
                <div class="text-h4 font-weight-bold">
                  {{ lastData?.temperature?.toFixed(1) || '-' }}°C
                </div>
                <div class="text-caption">Цель: {{ node?.config?.temp_target || '-' }}°C</div>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="3">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Влажность</v-card-title>
              <v-card-text class="text-center">
                <div class="text-h4 font-weight-bold">
                  {{ lastData?.humidity?.toFixed(0) || '-' }}%
                </div>
                <div class="text-caption">Цель: {{ node?.config?.humidity_target || '-' }}%</div>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="3">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">CO₂</v-card-title>
              <v-card-text class="text-center">
                <div class="text-h4 font-weight-bold">
                  {{ lastData?.co2 || '-' }}
                </div>
                <div class="text-caption">Макс: {{ node?.config?.co2_max || '-' }} ppm</div>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="3">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Освещенность</v-card-title>
              <v-card-text class="text-center">
                <div class="text-h4 font-weight-bold">
                  {{ lastData?.lux || '-' }}
                </div>
                <div class="text-caption">lux</div>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
        
        <!-- Climate Control Actions -->
        <v-row class="mt-4">
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Управление вентиляцией</v-card-title>
              <v-card-text>
                <v-btn
                  block
                  color="success"
                  class="mb-2"
                  @click="sendCommand('start_fan')"
                  :disabled="!canPerformActions"
                >
                  Включить вентиляцию
                </v-btn>
                <v-btn
                  block
                  color="warning"
                  @click="sendCommand('stop_fan')"
                  :disabled="!canPerformActions"
                >
                  Выключить вентиляцию
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Управление форточками</v-card-title>
              <v-card-text>
                <v-btn
                  block
                  color="primary"
                  class="mb-2"
                  @click="sendCommand('open_windows')"
                  :disabled="!canPerformActions"
                >
                  Открыть форточки
                </v-btn>
                <v-btn
                  block
                  color="secondary"
                  @click="sendCommand('close_windows')"
                  :disabled="!canPerformActions"
                >
                  Закрыть форточки
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Управление освещением</v-card-title>
              <v-card-text>
                <v-btn
                  block
                  color="info"
                  class="mb-2"
                  @click="sendCommand('turn_on_lights')"
                  :disabled="!canPerformActions"
                >
                  Включить освещение
                </v-btn>
                <v-btn
                  block
                  color="info"
                  variant="outlined"
                  @click="sendCommand('turn_off_lights')"
                  :disabled="!canPerformActions"
                >
                  Выключить освещение
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </div>

      <!-- Relay Node Controls -->
      <div v-else-if="node?.node_type === 'relay'">
        <h3 class="text-h6 mb-3">Управление реле</h3>
        
        <v-row>
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Форточки</v-card-title>
              <v-card-text>
                <v-btn
                  block
                  color="primary"
                  class="mb-2"
                  @click="sendCommand('open_windows')"
                  :disabled="!canPerformActions"
                >
                  Открыть все форточки
                </v-btn>
                <v-btn
                  block
                  color="secondary"
                  @click="sendCommand('close_windows')"
                  :disabled="!canPerformActions"
                >
                  Закрыть все форточки
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Вентиляция</v-card-title>
              <v-card-text>
                <v-btn
                  block
                  color="success"
                  class="mb-2"
                  @click="sendCommand('start_fan')"
                  :disabled="!canPerformActions"
                >
                  Включить вентиляцию
                </v-btn>
                <v-btn
                  block
                  color="warning"
                  @click="sendCommand('stop_fan')"
                  :disabled="!canPerformActions"
                >
                  Выключить вентиляцию
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Освещение</v-card-title>
              <v-card-text>
                <v-slider
                  v-model="lightBrightness"
                  label="Яркость"
                  :min="0"
                  :max="100"
                  :step="5"
                  thumb-label
                  @update:model-value="setLightBrightness"
                ></v-slider>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </div>

      <!-- Water Node Controls -->
      <div v-else-if="node?.node_type === 'water'">
        <h3 class="text-h6 mb-3">Управление водой</h3>
        
        <v-row>
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Уровень воды</v-card-title>
              <v-card-text class="text-center">
                <div class="text-h4 font-weight-bold">
                  {{ lastData?.level?.toFixed(0) || '-' }}%
                </div>
                <div class="text-caption">Цель: {{ node?.config?.water_level_target || '-' }}%</div>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Температура</v-card-title>
              <v-card-text class="text-center">
                <div class="text-h4 font-weight-bold">
                  {{ lastData?.temp?.toFixed(1) || '-' }}°C
                </div>
                <div class="text-caption">Цель: {{ node?.config?.water_temp_target || '-' }}°C</div>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="4">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Насосы</v-card-title>
              <v-card-text>
                <v-btn
                  block
                  color="primary"
                  class="mb-2"
                  @click="sendCommand('start_supply_pump')"
                  :disabled="!canPerformActions"
                >
                  Подача воды
                </v-btn>
                <v-btn
                  block
                  color="info"
                  @click="sendCommand('start_drain_pump')"
                  :disabled="!canPerformActions"
                >
                  Слив воды
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
        
        <!-- Water Control Actions -->
        <v-row class="mt-4">
          <v-col cols="12" md="6">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Клапаны</v-card-title>
              <v-card-text>
                <v-btn
                  block
                  color="success"
                  class="mb-2"
                  @click="sendCommand('open_valves')"
                  :disabled="!canPerformActions"
                >
                  Открыть клапаны
                </v-btn>
                <v-btn
                  block
                  color="warning"
                  @click="sendCommand('close_valves')"
                  :disabled="!canPerformActions"
                >
                  Закрыть клапаны
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
          
          <v-col cols="12" md="6">
            <v-card variant="outlined">
              <v-card-title class="text-subtitle-1">Обслуживание</v-card-title>
              <v-card-text>
                <v-btn
                  block
                  color="info"
                  class="mb-2"
                  @click="sendCommand('check_water_quality')"
                  :disabled="!canPerformActions"
                >
                  Проверить качество воды
                </v-btn>
                <v-btn
                  block
                  color="warning"
                  @click="sendCommand('flush_system')"
                  :disabled="!canPerformActions"
                >
                  Промыть систему
                </v-btn>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </div>

      <!-- System Actions -->
      <v-divider class="my-4"></v-divider>
      <h3 class="text-h6 mb-3">Системные действия</h3>
      
      <v-row>
        <v-col cols="12" sm="6" md="3">
          <v-btn
            block
            color="info"
            @click="sendCommand('get_status')"
            :disabled="!canPerformActions"
          >
            Получить статус
          </v-btn>
        </v-col>
        <v-col cols="12" sm="6" md="3">
          <v-btn
            block
            color="success"
            @click="getConfig"
            :disabled="!canPerformActions"
          >
            Получить конфиг
          </v-btn>
        </v-col>
        <v-col cols="12" sm="6" md="3">
          <v-btn
            block
            color="warning"
            @click="sendCommand('reboot')"
            :disabled="!canPerformActions"
          >
            Перезагрузить
          </v-btn>
        </v-col>
        <v-col cols="12" sm="6" md="3">
          <v-btn
            block
            color="error"
            @click="sendCommand('emergency_stop')"
            :disabled="!canPerformActions"
          >
            Экстренная остановка
          </v-btn>
        </v-col>
      </v-row>
    </v-card-text>
    
    <!-- Calibration Dialog -->
    <v-dialog v-model="showCalibration" max-width="500">
      <v-card>
        <v-card-title>
          <v-icon icon="mdi-tune-variant" class="mr-2"></v-icon>
          Калибровка {{ node?.node_type === 'ph' ? 'pH' : 'EC' }} датчика
        </v-card-title>
        <v-card-text>
          <v-select
            v-model="calibrationType"
            :items="calibrationOptions"
            label="Тип калибровки"
            variant="outlined"
          ></v-select>
          
          <v-text-field
            v-model.number="calibrationValue"
            :label="`Значение для калибровки (${calibrationType})`"
            type="number"
            step="0.1"
            variant="outlined"
          ></v-text-field>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="showCalibration = false">Отмена</v-btn>
          <v-btn 
            color="primary" 
            @click="performCalibration"
            :disabled="!calibrationValue"
          >
            Калибровать
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-card>
</template>

<script setup>
import { ref, computed } from 'vue'
import { useNodeStatus } from '@/composables/useNodeStatus'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
  errorCount: {
    type: Number,
    default: 0,
  },
})

const emit = defineEmits(['command'])

// Централизованная система статусов
const {
  isOnline,
  isPumpRunning,
  statusColor,
  statusIcon,
  statusText,
  lastSeenText,
  canPerformActions,
  canRunPumps
} = useNodeStatus({ value: () => props.node })

// Light brightness control
const lightBrightness = ref(50)

// Calibration dialog
const showCalibration = ref(false)
const calibrationType = ref('7.0')
const calibrationValue = ref(7.0)

const calibrationOptions = computed(() => {
  if (props.node?.node_type === 'ph') {
    return [
      { title: '4.0 - Кислая', value: '4.0' },
      { title: '7.0 - Нейтральная', value: '7.0' },
      { title: '10.0 - Щелочная', value: '10.0' }
    ]
  } else if (props.node?.node_type === 'ec') {
    return [
      { title: '1.413 mS/cm - Низкая', value: '1.413' },
      { title: '2.76 mS/cm - Средняя', value: '2.76' },
      { title: '12.88 mS/cm - Высокая', value: '12.88' }
    ]
  }
  return []
})

// Node icon
const nodeIcon = computed(() => {
  const icons = {
    'ph': 'mdi-flask-outline',
    'ec': 'mdi-flash',
    'climate': 'mdi-thermometer',
    'relay': 'mdi-electric-switch',
    'water': 'mdi-water',
    'display': 'mdi-monitor',
    'root': 'mdi-server-network',
  }
  return icons[props.node?.node_type] || 'mdi-chip'
})

// Last telemetry data
const lastData = computed(() => {
  if (!props.node) return null
  return props.node.last_telemetry?.data || props.node.last_data || null
})

// Status indicators
const pHStatus = computed(() => {
  if (!lastData.value?.ph) return 'Нет данных'
  const ph = lastData.value.ph
  const target = props.node?.config?.ph_target || 6.5
  const tolerance = props.node?.config?.ph_tolerance || 0.1
  
  if (ph < target - tolerance) return 'Низкий'
  if (ph > target + tolerance) return 'Высокий'
  return 'Нормальный'
})

const ecStatus = computed(() => {
  if (!lastData.value?.ec) return 'Нет данных'
  const ec = lastData.value.ec
  const target = props.node?.config?.ec_target || 1.2
  const tolerance = props.node?.config?.ec_tolerance || 0.1
  
  if (ec < target - tolerance) return 'Низкий'
  if (ec > target + tolerance) return 'Высокий'
  return 'Нормальный'
})

// Memory info
const metadata = computed(() => {
  if (!props.node) return {}
  return props.node.metadata || {}
})

const hasMemoryInfo = computed(() => {
  return metadata.value.heap_total || metadata.value.total_heap || metadata.value.heap_used || metadata.value.heap_free
})

const heapFree = computed(() => metadata.value.heap_free || metadata.value.free_heap || 0)
const heapTotal = computed(() => {
  if (metadata.value.heap_total || metadata.value.total_heap) {
    return metadata.value.heap_total || metadata.value.total_heap
  }
  // Оцениваем total по free (предполагаем free ~60%)
  if (heapFree.value > 0) {
    return Math.round(heapFree.value / 0.6)
  }
  return 320000
})
const heapUsed = computed(() => {
  if (metadata.value.heap_used) return metadata.value.heap_used
  return heapTotal.value - heapFree.value
})
const heapPercent = computed(() => (heapUsed.value / heapTotal.value) * 100)

function getMemoryColor(percent) {
  if (percent < 50) return 'success'
  if (percent < 75) return 'warning'
  return 'error'
}

function formatBytes(bytes) {
  if (bytes === 0) return '0 Bytes'
  const k = 1024
  const sizes = ['Bytes', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
}

// WiFi Signal (RSSI)
const hasRssi = computed(() => {
  return metadata.value.rssi_to_parent != null || metadata.value.wifi_rssi != null
})

const rssiValue = computed(() => {
  return metadata.value.rssi_to_parent || metadata.value.wifi_rssi || 0
})

// Преобразование RSSI (-100 до -30 dBm) в проценты (0-100%)
const rssiPercent = computed(() => {
  const rssi = rssiValue.value
  if (rssi === 0) return 0
  // -30 dBm = отлично (100%), -90 dBm = плохо (0%)
  const percent = Math.min(100, Math.max(0, (rssi + 90) * (100 / 60)))
  return percent
})

const rssiDescription = computed(() => {
  const percent = rssiPercent.value
  if (percent > 75) return 'Отличный'
  if (percent > 50) return 'Хороший'
  if (percent > 25) return 'Слабый'
  return 'Плохой'
})

function getRssiColor(percent) {
  if (percent > 60) return 'success'
  if (percent > 30) return 'warning'
  return 'error'
}

// Command functions
function sendCommand(command, params = {}) {
  emit('command', { command, params })
}

function runPump(pumpType, duration) {
  sendCommand('run_pump_manual', { pump: pumpType, duration })
}

function setLightBrightness(brightness) {
  sendCommand('set_light_brightness', { brightness })
}

function getConfig() {
  sendCommand('get_config')
}

function openCalibration() {
  showCalibration.value = true
}

function performCalibration() {
  if (props.node?.node_type === 'ph') {
    sendCommand('calibrate_ph', { 
      value: calibrationValue.value,
      type: calibrationType.value
    })
  } else if (props.node?.node_type === 'ec') {
    sendCommand('calibrate_ec', { 
      value: calibrationValue.value,
      type: calibrationType.value
    })
  }
  showCalibration.value = false
}
</script>