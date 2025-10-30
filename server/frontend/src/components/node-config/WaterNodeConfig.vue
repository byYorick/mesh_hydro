<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-water" class="mr-2"></v-icon>
      Конфигурация водного узла: {{ node?.node_id }}
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
        
        <!-- Water Level Settings -->
        <h3 class="text-h6 mb-3">Настройки уровня воды</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.water_level_target"
              label="Целевой уровень воды (%)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.water_level_tolerance"
              label="Допуск уровня воды (%)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.water_level_min"
              label="Минимальный уровень воды (%)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.water_level_max"
              label="Максимальный уровень воды (%)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-divider class="my-4"></v-divider>
        
        <!-- Water Temperature Settings -->
        <h3 class="text-h6 mb-3">Настройки температуры воды</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.water_temp_target"
              label="Целевая температура воды (°C)"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.water_temp_tolerance"
              label="Допуск температуры воды (°C)"
              type="number"
              step="0.1"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-divider class="my-4"></v-divider>
        
        <!-- Pump Control Settings -->
        <h3 class="text-h6 mb-3">Настройки управления насосами</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.pump_duration"
              label="Длительность работы насоса (сек)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.pump_cooldown"
              label="Время охлаждения насоса (сек)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-switch
          v-model="config.auto_refill_enabled"
          label="Автоматическое пополнение воды"
          color="success"
        ></v-switch>
        
        <v-switch
          v-model="config.drain_protection"
          label="Защита от опустошения"
          color="success"
        ></v-switch>
        
        <!-- Valve Control Settings -->
        <v-divider class="my-4"></v-divider>
        <h3 class="text-h6 mb-3">Настройки клапанов</h3>
        
        <v-switch
          v-model="config.auto_valve_control"
          label="Автоматическое управление клапанами"
          color="success"
        ></v-switch>
        
        <v-text-field
          v-model.number="config.valve_open_time"
          label="Время открытия клапана (сек)"
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
  // Initialize with default values for Water node
  const defaultConfig = {
    zone: props.node.zone || '',
    interval: props.node.config?.interval || 30,
    enabled: props.node.config?.enabled !== false,
    water_level_target: props.node.config?.water_level_target || 70,
    water_level_tolerance: props.node.config?.water_level_tolerance || 5,
    water_level_min: props.node.config?.water_level_min || 20,
    water_level_max: props.node.config?.water_level_max || 90,
    water_temp_target: props.node.config?.water_temp_target || 22,
    water_temp_tolerance: props.node.config?.water_temp_tolerance || 1,
    pump_duration: props.node.config?.pump_duration || 30,
    pump_cooldown: props.node.config?.pump_cooldown || 60,
    auto_refill_enabled: props.node.config?.auto_refill_enabled !== false,
    drain_protection: props.node.config?.drain_protection !== false,
    auto_valve_control: props.node.config?.auto_valve_control !== false,
    valve_open_time: props.node.config?.valve_open_time || 10
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