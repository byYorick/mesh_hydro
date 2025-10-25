<template>
  <v-dialog v-model="dialog" max-width="800px" scrollable>
    <v-card>
      <v-card-title class="d-flex align-center">
        <v-icon icon="mdi-cog" class="mr-2"></v-icon>
        Конфигурация узла: {{ nodeId }}
      </v-card-title>

      <v-card-text>
        <div v-if="loading" class="text-center py-4">
          <v-progress-circular indeterminate color="primary"></v-progress-circular>
          <p class="mt-2">Загрузка конфигурации...</p>
        </div>

        <div v-else-if="error" class="text-center py-4">
          <v-icon icon="mdi-alert-circle" color="error" size="48"></v-icon>
          <p class="text-error mt-2">{{ error }}</p>
        </div>

        <div v-else-if="config" class="config-content">
          <!-- Basic Info -->
          <v-card variant="outlined" class="mb-4">
            <v-card-title class="text-subtitle-1">Основная информация</v-card-title>
            <v-card-text>
              <v-row>
                <v-col cols="6">
                  <strong>Node ID:</strong> {{ config.node_id }}
                </v-col>
                <v-col cols="6">
                  <strong>Тип:</strong> {{ config.node_type }}
                </v-col>
                <v-col cols="6">
                  <strong>Зона:</strong> {{ config.zone }}
                </v-col>
                <v-col cols="6">
                  <strong>Версия:</strong> {{ config.config_version }}
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>

          <!-- pH Parameters -->
          <v-card v-if="config.ph_target !== undefined" variant="outlined" class="mb-4">
            <v-card-title class="text-subtitle-1">pH параметры</v-card-title>
            <v-card-text>
              <v-row>
                <v-col cols="4">
                  <strong>Целевое значение:</strong> {{ config.ph_target }}
                </v-col>
                <v-col cols="4">
                  <strong>Минимум:</strong> {{ config.ph_min }}
                </v-col>
                <v-col cols="4">
                  <strong>Максимум:</strong> {{ config.ph_max }}
                </v-col>
                <v-col cols="6">
                  <strong>Калибровка:</strong> {{ config.ph_cal_offset }}
                </v-col>
                <v-col cols="6">
                  <strong>Emergency низкий:</strong> {{ config.ph_emergency_low }}
                </v-col>
                <v-col cols="6">
                  <strong>Emergency высокий:</strong> {{ config.ph_emergency_high }}
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>

          <!-- EC Parameters -->
          <v-card v-if="config.ec_target !== undefined" variant="outlined" class="mb-4">
            <v-card-title class="text-subtitle-1">EC параметры</v-card-title>
            <v-card-text>
              <v-row>
                <v-col cols="4">
                  <strong>Целевое значение:</strong> {{ config.ec_target }}
                </v-col>
                <v-col cols="4">
                  <strong>Минимум:</strong> {{ config.ec_min }}
                </v-col>
                <v-col cols="4">
                  <strong>Максимум:</strong> {{ config.ec_max }}
                </v-col>
                <v-col cols="6">
                  <strong>Калибровка:</strong> {{ config.ec_cal_offset }}
                </v-col>
                <v-col cols="6">
                  <strong>Emergency высокий:</strong> {{ config.ec_emergency_high }}
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>

          <!-- Safety Parameters -->
          <v-card variant="outlined" class="mb-4">
            <v-card-title class="text-subtitle-1">Параметры безопасности</v-card-title>
            <v-card-text>
              <v-row>
                <v-col cols="4">
                  <strong>Макс. время насоса:</strong> {{ config.max_pump_time_ms }} мс
                </v-col>
                <v-col cols="4">
                  <strong>Cooldown:</strong> {{ config.cooldown_ms }} мс
                </v-col>
                <v-col cols="4">
                  <strong>Дневной лимит:</strong> {{ config.max_daily_volume_ml }} мл
                </v-col>
                <v-col cols="6">
                  <strong>Автономия:</strong> {{ config.autonomous_enabled ? 'Включена' : 'Выключена' }}
                </v-col>
                <v-col cols="6">
                  <strong>Таймаут mesh:</strong> {{ config.mesh_timeout_ms }} мс
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>

          <!-- Pump Calibration -->
          <v-card v-if="config.pumps_calibration" variant="outlined" class="mb-4">
            <v-card-title class="text-subtitle-1">Калибровка насосов</v-card-title>
            <v-card-text>
              <v-table density="compact">
                <thead>
                  <tr>
                    <th>Насос</th>
                    <th>мл/сек</th>
                    <th>Объем (мл)</th>
                    <th>Время (мс)</th>
                    <th>Калиброван</th>
                    <th>Последняя калибровка</th>
                  </tr>
                </thead>
                <tbody>
                  <tr v-for="(pump, index) in config.pumps_calibration" :key="index">
                    <td>{{ index }}</td>
                    <td>{{ pump.ml_per_second }}</td>
                    <td>{{ pump.calibration_volume_ml }}</td>
                    <td>{{ pump.calibration_time_ms }}</td>
                    <td>
                      <v-icon :color="pump.is_calibrated ? 'success' : 'error'">
                        {{ pump.is_calibrated ? 'mdi-check' : 'mdi-close' }}
                      </v-icon>
                    </td>
                    <td>{{ formatDate(pump.last_calibrated) }}</td>
                  </tr>
                </tbody>
              </v-table>
            </v-card-text>
          </v-card>

          <!-- PID Parameters -->
          <v-card v-if="config.pumps_pid" variant="outlined" class="mb-4">
            <v-card-title class="text-subtitle-1">PID параметры</v-card-title>
            <v-card-text>
              <v-table density="compact">
                <thead>
                  <tr>
                    <th>Насос</th>
                    <th>Kp</th>
                    <th>Ki</th>
                    <th>Kd</th>
                    <th>Включен</th>
                  </tr>
                </thead>
                <tbody>
                  <tr v-for="(pid, index) in config.pumps_pid" :key="index">
                    <td>{{ index }}</td>
                    <td>{{ pid.kp }}</td>
                    <td>{{ pid.ki }}</td>
                    <td>{{ pid.kd }}</td>
                    <td>
                      <v-icon :color="pid.enabled ? 'success' : 'error'">
                        {{ pid.enabled ? 'mdi-check' : 'mdi-close' }}
                      </v-icon>
                    </td>
                  </tr>
                </tbody>
              </v-table>
            </v-card-text>
          </v-card>

          <!-- Raw JSON -->
          <v-card variant="outlined">
            <v-card-title class="text-subtitle-1">JSON конфигурация</v-card-title>
            <v-card-text>
              <v-textarea
                :value="JSON.stringify(config, null, 2)"
                readonly
                rows="10"
                variant="outlined"
                class="font-family-monospace"
              ></v-textarea>
            </v-card-text>
          </v-card>
        </div>
      </v-card-text>

      <v-card-actions>
        <v-spacer></v-spacer>
        <v-btn @click="dialog = false" color="primary">
          Закрыть
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup>
import { ref, watch } from 'vue'

const props = defineProps({
  modelValue: {
    type: Boolean,
    default: false
  },
  nodeId: {
    type: String,
    required: true
  }
})

const emit = defineEmits(['update:modelValue'])

const dialog = ref(false)
const loading = ref(false)
const error = ref(null)
const config = ref(null)

// Watch for dialog changes
watch(() => props.modelValue, (newVal) => {
  dialog.value = newVal
  if (newVal) {
    loadConfig()
  }
})

watch(dialog, (newVal) => {
  emit('update:modelValue', newVal)
})

// Load config from node
async function loadConfig() {
  loading.value = true
  error.value = null
  config.value = null
  
  try {
    // Здесь нужно будет добавить логику для получения конфига
    // Пока что показываем заглушку
    await new Promise(resolve => setTimeout(resolve, 1000))
    
    // TODO: Реализовать получение конфига от ноды
    config.value = {
      node_id: props.nodeId,
      node_type: 'ph',
      zone: 'Zone A',
      config_version: 1,
      ph_target: 6.5,
      ph_min: 5.5,
      ph_max: 7.5,
      ph_cal_offset: 0.0,
      ph_emergency_low: 5.5,
      ph_emergency_high: 7.5,
      max_pump_time_ms: 10000,
      cooldown_ms: 60000,
      max_daily_volume_ml: 1000,
      autonomous_enabled: true,
      mesh_timeout_ms: 30000,
      pumps_calibration: [
        {
          ml_per_second: 1.0,
          calibration_volume_ml: 10.0,
          calibration_time_ms: 10000,
          is_calibrated: true,
          last_calibrated: Date.now()
        },
        {
          ml_per_second: 1.0,
          calibration_volume_ml: 10.0,
          calibration_time_ms: 10000,
          is_calibrated: false,
          last_calibrated: 0
        }
      ],
      pumps_pid: [
        { kp: 1.0, ki: 0.05, kd: 0.3, enabled: true },
        { kp: 1.0, ki: 0.05, kd: 0.3, enabled: true }
      ]
    }
  } catch (err) {
    error.value = err.message || 'Ошибка загрузки конфигурации'
  } finally {
    loading.value = false
  }
}

function formatDate(timestamp) {
  if (!timestamp) return 'Никогда'
  return new Date(timestamp).toLocaleString()
}
</script>

<style scoped>
.font-family-monospace {
  font-family: 'Courier New', monospace;
  font-size: 12px;
}
</style>
