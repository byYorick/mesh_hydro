<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-electric-switch" class="mr-2"></v-icon>
      Конфигурация релейного узла: {{ node?.node_id }}
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
        
        <!-- Window Control Settings -->
        <h3 class="text-h6 mb-3">Настройки управления форточками</h3>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.window_open_time"
              label="Время открытия форточки (сек)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model.number="config.window_close_time"
              label="Время закрытия форточки (сек)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-switch
          v-model="config.auto_window_control"
          label="Автоматическое управление форточками"
          color="success"
        ></v-switch>
        
        <v-divider class="my-4"></v-divider>
        
        <!-- Fan Control Settings -->
        <h3 class="text-h6 mb-3">Настройки вентилятора</h3>
        
        <v-switch
          v-model="config.auto_fan_control"
          label="Автоматическое управление вентилятором"
          color="success"
        ></v-switch>
        
        <v-text-field
          v-model.number="config.fan_delay"
          label="Задержка включения вентилятора (сек)"
          type="number"
          variant="outlined"
          density="compact"
        ></v-text-field>
        
        <v-divider class="my-4"></v-divider>
        
        <!-- Light Control Settings -->
        <h3 class="text-h6 mb-3">Настройки освещения</h3>
        
        <v-switch
          v-model="config.auto_light_control"
          label="Автоматическое управление освещением"
          color="success"
        ></v-switch>
        
        <v-row>
          <v-col cols="12" md="6">
            <v-text-field
              v-model="config.light_on_time"
              label="Время включения света"
              type="time"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
          <v-col cols="12" md="6">
            <v-text-field
              v-model="config.light_off_time"
              label="Время выключения света"
              type="time"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </v-col>
        </v-row>
        
        <v-switch
          v-model="config.lux_control_enabled"
          label="Управление по освещенности"
          color="success"
        ></v-switch>
        
        <v-text-field
          v-model.number="config.lux_threshold"
          label="Порог освещенности для включения"
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
  // Initialize with default values for Relay node
  const defaultConfig = {
    zone: props.node.zone || '',
    interval: props.node.config?.interval || 30,
    enabled: props.node.config?.enabled !== false,
    window_open_time: props.node.config?.window_open_time || 30,
    window_close_time: props.node.config?.window_close_time || 30,
    auto_window_control: props.node.config?.auto_window_control !== false,
    auto_fan_control: props.node.config?.auto_fan_control !== false,
    fan_delay: props.node.config?.fan_delay || 60,
    auto_light_control: props.node.config?.auto_light_control !== false,
    light_on_time: props.node.config?.light_on_time || '08:00',
    light_off_time: props.node.config?.light_off_time || '22:00',
    lux_control_enabled: props.node.config?.lux_control_enabled || false,
    lux_threshold: props.node.config?.lux_threshold || 300
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