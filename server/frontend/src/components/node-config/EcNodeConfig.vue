<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-flash" class="mr-2"></v-icon>
      Конфигурация EC узла: {{ node?.node_id }}
    </v-card-title>
    
    <v-card-text>
      <v-form ref="form" v-model="valid">
        <!-- General Settings -->
        <h3 class="text-h6 mb-3">Общие настройки</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model="config.zone"
              label="Зона"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.interval"
              label="Интервал отправки (секунды)"
              type="number"
              variant="outlined"
              density="compact"
              :rules="[v => v > 0 || 'Должно быть больше 0']"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-switch
          v-model="config.enabled"
          label="Узел включен"
          color="success"
        ></v-switch>
        
        <v-divider class="my-4"></v-divider>
        
        <!-- EC Settings -->
        <h3 class="text-h6 mb-3">Настройки EC</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.ec_target"
              label="Целевой EC (mS/cm)"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.ec_tolerance"
              label="Допуск EC"
              type="number"
              step="0.01"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.ec_min"
              label="Минимальный EC"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.ec_max"
              label="Максимальный EC"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-divider class="my-4"></v-divider>
        
        <!-- Pump Settings -->
        <h3 class="text-h6 mb-3">Настройки насосов</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.max_pump_time"
              label="Макс. время работы насоса (сек)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.cooldown_time"
              label="Время охлаждения (сек)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <!-- PID Controllers -->
        <v-divider class="my-4"></v-divider>
        <h3 class="text-h6 mb-3">PID контроллеры</h3>
        
        <v-expansion-panels>
          <v-expansion-panel>
            <v-expansion-panel-title>
              EC Up контроллер
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-row>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ec_up.kp"
                    label="Kp"
                    type="number"
                    step="0.01"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ec_up.ki"
                    label="Ki"
                    type="number"
                    step="0.001"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ec_up.kd"
                    label="Kd"
                    type="number"
                    step="0.001"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
              </v-row>
              <v-row>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="config.pid_ec_up.output_min"
                    label="Мин. выход"
                    type="number"
                    step="0.1"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="config.pid_ec_up.output_max"
                    label="Макс. выход"
                    type="number"
                    step="0.1"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
              </v-row>
              <v-switch
                v-model="config.pid_ec_up.enabled"
                label="Контроллер включен"
                color="success"
              ></v-switch>
            </v-expansion-panel-text>
          </v-expansion-panel>
          
          <v-expansion-panel>
            <v-expansion-panel-title>
              EC Down контроллер
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-row>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ec_down.kp"
                    label="Kp"
                    type="number"
                    step="0.01"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ec_down.ki"
                    label="Ki"
                    type="number"
                    step="0.001"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ec_down.kd"
                    label="Kd"
                    type="number"
                    step="0.001"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
              </v-row>
              <v-row>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="config.pid_ec_down.output_min"
                    label="Мин. выход"
                    type="number"
                    step="0.1"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="config.pid_ec_down.output_max"
                    label="Макс. выход"
                    type="number"
                    step="0.1"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
              </v-row>
              <v-switch
                v-model="config.pid_ec_down.enabled"
                label="Контроллер включен"
                color="success"
              ></v-switch>
            </v-expansion-panel-text>
          </v-expansion-panel>
        </v-expansion-panels>
        
        <!-- Safety Settings -->
        <v-divider class="my-4"></v-divider>
        <h3 class="text-h6 mb-3">Настройки безопасности</h3>
        
        <v-switch
          v-model="config.emergency_mode"
          label="Аварийный режим включен"
          color="error"
        ></v-switch>
        
        <v-text-field
          v-model.number="config.emergency_ec_min"
          label="Минимальный аварийный EC"
          type="number"
          step="0.1"
          variant="outlined"
          density="compact"
        ></v-text-field>
        
        <v-text-field
          v-model.number="config.emergency_ec_max"
          label="Максимальный аварийный EC"
          type="number"
          step="0.1"
          variant="outlined"
          density="compact"
        ></v-text-field>
      </v-form>
    </v-card-text>
    
    <v-card-actions>
      <v-btn @click="resetConfig">Сбросить</v-btn>
      <v-spacer></v-spacer>
      <v-btn @click="$emit('close')">Отмена</v-btn>
      <v-btn
        color="primary"
        :disabled="!valid"
        :loading="loading"
        @click="saveConfig"
      >
        Сохранить
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup>
import { ref, watch } from 'vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['close', 'config-updated'])

const form = ref(null)
const valid = ref(true)
const loading = ref(false)
const config = ref({})

// Initialize config when node changes
watch(() => props.node, (newNode) => {
  if (newNode) {
    resetConfig()
  }
}, { immediate: true })

function resetConfig() {
  // Initialize with default values for EC node
  const defaultConfig = {
    zone: props.node.zone || '',
    interval: props.node.config?.interval || 30,
    enabled: props.node.config?.enabled !== false,
    ec_target: props.node.config?.ec_target || 1.2,
    ec_tolerance: props.node.config?.ec_tolerance || 0.1,
    ec_min: props.node.config?.ec_min || 0.8,
    ec_max: props.node.config?.ec_max || 2.0,
    max_pump_time: props.node.config?.max_pump_time || 30,
    cooldown_time: props.node.config?.cooldown_time || 60,
    emergency_mode: props.node.config?.emergency_mode || false,
    emergency_ec_min: props.node.config?.emergency_ec_min || 0.5,
    emergency_ec_max: props.node.config?.emergency_ec_max || 3.0,
    pid_ec_up: {
      kp: props.node.config?.pid_ec_up?.kp || 0.8,
      ki: props.node.config?.pid_ec_up?.ki || 0.03,
      kd: props.node.config?.pid_ec_up?.kd || 0.1,
      output_min: props.node.config?.pid_ec_up?.output_min || 0,
      output_max: props.node.config?.pid_ec_up?.output_max || 5,
      enabled: props.node.config?.pid_ec_up?.enabled !== false
    },
    pid_ec_down: {
      kp: props.node.config?.pid_ec_down?.kp || 0.8,
      ki: props.node.config?.pid_ec_down?.ki || 0.03,
      kd: props.node.config?.pid_ec_down?.kd || 0.1,
      output_min: props.node.config?.pid_ec_down?.output_min || 0,
      output_max: props.node.config?.pid_ec_down?.output_max || 5,
      enabled: props.node.config?.pid_ec_down?.enabled !== false
    }
  }
  
  config.value = { ...defaultConfig }
}

async function saveConfig() {
  const { valid } = await form.value.validate()
  
  if (!valid) return
  
  loading.value = true
  
  try {
    emit('config-updated', config.value)
  } finally {
    loading.value = false
  }
}
</script>