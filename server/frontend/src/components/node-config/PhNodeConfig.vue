<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-flask" class="mr-2"></v-icon>
      Конфигурация pH узла: {{ node?.node_id }}
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
        
        <!-- pH Settings -->
        <h3 class="text-h6 mb-3">Настройки pH</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.ph_target"
              label="Целевой pH"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.ph_tolerance"
              label="Допуск pH"
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
              v-model.number="config.ph_min"
              label="Минимальный pH"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.ph_max"
              label="Максимальный pH"
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
              pH Up контроллер
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-row>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ph_up.kp"
                    label="Kp"
                    type="number"
                    step="0.01"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ph_up.ki"
                    label="Ki"
                    type="number"
                    step="0.001"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ph_up.kd"
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
                    v-model.number="config.pid_ph_up.output_min"
                    label="Мин. выход"
                    type="number"
                    step="0.1"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="config.pid_ph_up.output_max"
                    label="Макс. выход"
                    type="number"
                    step="0.1"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
              </v-row>
              <v-switch
                v-model="config.pid_ph_up.enabled"
                label="Контроллер включен"
                color="success"
              ></v-switch>
            </v-expansion-panel-text>
          </v-expansion-panel>
          
          <v-expansion-panel>
            <v-expansion-panel-title>
              pH Down контроллер
            </v-expansion-panel-title>
            <v-expansion-panel-text>
              <v-row>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ph_down.kp"
                    label="Kp"
                    type="number"
                    step="0.01"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ph_down.ki"
                    label="Ki"
                    type="number"
                    step="0.001"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="4">
                  <v-text-field
                    v-model.number="config.pid_ph_down.kd"
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
                    v-model.number="config.pid_ph_down.output_min"
                    label="Мин. выход"
                    type="number"
                    step="0.1"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
                <v-col cols="6">
                  <v-text-field
                    v-model.number="config.pid_ph_down.output_max"
                    label="Макс. выход"
                    type="number"
                    step="0.1"
                    variant="outlined"
                    density="compact"
                  ></v-text-field>
                </v-col>
              </v-row>
              <v-switch
                v-model="config.pid_ph_down.enabled"
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
          v-model.number="config.emergency_ph_min"
          label="Минимальный аварийный pH"
          type="number"
          step="0.1"
          variant="outlined"
          density="compact"
        ></v-text-field>
        
        <v-text-field
          v-model.number="config.emergency_ph_max"
          label="Максимальный аварийный pH"
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
  // Initialize with default values for pH node
  const defaultConfig = {
    zone: props.node.zone || '',
    interval: props.node.config?.interval || 30,
    enabled: props.node.config?.enabled !== false,
    ph_target: props.node.config?.ph_target || 6.5,
    ph_tolerance: props.node.config?.ph_tolerance || 0.1,
    ph_min: props.node.config?.ph_min || 5.5,
    ph_max: props.node.config?.ph_max || 7.5,
    max_pump_time: props.node.config?.max_pump_time || 30,
    cooldown_time: props.node.config?.cooldown_time || 60,
    emergency_mode: props.node.config?.emergency_mode || false,
    emergency_ph_min: props.node.config?.emergency_ph_min || 5.0,
    emergency_ph_max: props.node.config?.emergency_ph_max || 8.0,
    pid_ph_up: {
      kp: props.node.config?.pid_ph_up?.kp || 1.0,
      ki: props.node.config?.pid_ph_up?.ki || 0.05,
      kd: props.node.config?.pid_ph_up?.kd || 0.3,
      output_min: props.node.config?.pid_ph_up?.output_min || 0,
      output_max: props.node.config?.pid_ph_up?.output_max || 5,
      enabled: props.node.config?.pid_ph_up?.enabled !== false
    },
    pid_ph_down: {
      kp: props.node.config?.pid_ph_down?.kp || 1.0,
      ki: props.node.config?.pid_ph_down?.ki || 0.05,
      kd: props.node.config?.pid_ph_down?.kd || 0.3,
      output_min: props.node.config?.pid_ph_down?.output_min || 0,
      output_max: props.node.config?.pid_ph_down?.output_max || 5,
      enabled: props.node.config?.pid_ph_down?.enabled !== false
    }
  }
  
  config.value = { ...defaultConfig }
}

async function saveConfig() {
  loading.value = true
  
  try {
    emit('config-updated', config.value)
  } finally {
    loading.value = false
  }
}
</script>