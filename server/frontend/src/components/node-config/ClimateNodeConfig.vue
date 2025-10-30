<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-thermometer" class="mr-2"></v-icon>
      Конфигурация климатического узла: {{ node?.node_id }}
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
        
        <!-- Temperature Settings -->
        <h3 class="text-h6 mb-3">Настройки температуры</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.temp_target"
              label="Целевая температура (°C)"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.temp_tolerance"
              label="Допуск температуры (°C)"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.temp_min"
              label="Минимальная температура (°C)"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.temp_max"
              label="Максимальная температура (°C)"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-divider class="my-4"></v-divider>
        
        <!-- Humidity Settings -->
        <h3 class="text-h6 mb-3">Настройки влажности</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.humidity_target"
              label="Целевая влажность (%)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.humidity_tolerance"
              label="Допуск влажности (%)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.humidity_min"
              label="Минимальная влажность (%)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.humidity_max"
              label="Максимальная влажность (%)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-divider class="my-4"></v-divider>
        
        <!-- CO2 Settings -->
        <h3 class="text-h6 mb-3">Настройки CO2</h3>
        
        <v-text-field
          v-model.number="config.co2_max"
          label="Максимальный CO2 (ppm)"
          type="number"
          variant="outlined"
          density="compact"
        ></v-text-field>
        
        <v-divider class="my-4"></v-divider>
        
        <!-- Lux Settings -->
        <h3 class="text-h6 mb-3">Настройки освещенности</h3>
        
        <v-text-field
          v-model.number="config.lux_min"
          label="Минимальная освещенность (lux)"
          type="number"
          variant="outlined"
          density="compact"
        ></v-text-field>
        
        <!-- Control Settings -->
        <v-divider class="my-4"></v-divider>
        <h3 class="text-h6 mb-3">Настройки управления</h3>
        
        <v-switch
          v-model="config.auto_control_enabled"
          label="Автоматическое управление включено"
          color="success"
        ></v-switch>
        
        <v-text-field
          v-model.number="config.control_delay"
          label="Задержка управления (сек)"
          type="number"
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
  // Initialize with default values for Climate node
  const defaultConfig = {
    zone: props.node.zone || '',
    interval: props.node.config?.interval || 30,
    enabled: props.node.config?.enabled !== false,
    temp_target: props.node.config?.temp_target || 24,
    temp_tolerance: props.node.config?.temp_tolerance || 1,
    temp_min: props.node.config?.temp_min || 18,
    temp_max: props.node.config?.temp_max || 30,
    humidity_target: props.node.config?.humidity_target || 60,
    humidity_tolerance: props.node.config?.humidity_tolerance || 5,
    humidity_min: props.node.config?.humidity_min || 40,
    humidity_max: props.node.config?.humidity_max || 80,
    co2_max: props.node.config?.co2_max || 1000,
    lux_min: props.node.config?.lux_min || 200,
    auto_control_enabled: props.node.config?.auto_control_enabled !== false,
    control_delay: props.node.config?.control_delay || 300
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