<template>
  <v-dialog v-model="dialog" max-width="800" scrollable>
    <template v-slot:activator="{ props }">
      <slot name="activator" :props="props"></slot>
    </template>

    <v-card>
      <v-card-title class="bg-secondary">
        <v-icon icon="mdi-cog" class="mr-2"></v-icon>
        Конфигурация: {{ node?.node_id }}
      </v-card-title>

      <v-card-text class="pt-4">
        <v-form ref="form" v-model="valid">
          <!-- General Settings -->
          <h3 class="mb-3">Общие настройки</h3>

          <v-text-field
            v-model="config.zone"
            label="Зона"
            variant="outlined"
            density="compact"
          ></v-text-field>

          <v-text-field
            v-model.number="config.interval"
            label="Интервал отправки (секунды)"
            type="number"
            variant="outlined"
            density="compact"
            :rules="[v => v > 0 || 'Должно быть больше 0']"
          ></v-text-field>

          <v-divider class="my-4"></v-divider>

          <!-- pH/EC specific -->
          <div v-if="node?.node_type === 'ph_ec'">
            <h3 class="mb-3">Настройки pH/EC</h3>

            <v-row>
              <v-col cols="6">
                <v-text-field
                  v-model.number="config.ph_threshold_min"
                  label="pH мин"
                  type="number"
                  step="0.1"
                  variant="outlined"
                  density="compact"
                ></v-text-field>
              </v-col>
              <v-col cols="6">
                <v-text-field
                  v-model.number="config.ph_threshold_max"
                  label="pH макс"
                  type="number"
                  step="0.1"
                  variant="outlined"
                  density="compact"
                ></v-text-field>
              </v-col>
            </v-row>

            <v-row>
              <v-col cols="6">
                <v-text-field
                  v-model.number="config.ec_threshold_min"
                  label="EC мин"
                  type="number"
                  step="0.1"
                  variant="outlined"
                  density="compact"
                ></v-text-field>
              </v-col>
              <v-col cols="6">
                <v-text-field
                  v-model.number="config.ec_threshold_max"
                  label="EC макс"
                  type="number"
                  step="0.1"
                  variant="outlined"
                  density="compact"
                ></v-text-field>
              </v-col>
            </v-row>
          </div>

          <!-- Climate specific -->
          <div v-if="node?.node_type === 'climate'">
            <h3 class="mb-3">Настройки климата</h3>

            <v-row>
              <v-col cols="6">
                <v-text-field
                  v-model.number="config.temp_threshold_min"
                  label="Температура мин (°C)"
                  type="number"
                  variant="outlined"
                  density="compact"
                ></v-text-field>
              </v-col>
              <v-col cols="6">
                <v-text-field
                  v-model.number="config.temp_threshold_max"
                  label="Температура макс (°C)"
                  type="number"
                  variant="outlined"
                  density="compact"
                ></v-text-field>
              </v-col>
            </v-row>

            <v-text-field
              v-model.number="config.co2_max"
              label="CO₂ макс (ppm)"
              type="number"
              variant="outlined"
              density="compact"
            ></v-text-field>
          </div>

          <!-- JSON Editor (advanced) -->
          <v-expansion-panels class="mt-4">
            <v-expansion-panel>
              <v-expansion-panel-title>
                <v-icon icon="mdi-code-json" class="mr-2"></v-icon>
                Расширенная конфигурация (JSON)
              </v-expansion-panel-title>
              <v-expansion-panel-text>
                <v-textarea
                  v-model="configJson"
                  rows="10"
                  variant="outlined"
                  :error-messages="jsonError"
                  @update:model-value="validateJson"
                ></v-textarea>
              </v-expansion-panel-text>
            </v-expansion-panel>
          </v-expansion-panels>
        </v-form>
      </v-card-text>

      <v-card-actions>
        <v-btn @click="resetConfig">Сбросить</v-btn>
        <v-spacer></v-spacer>
        <v-btn @click="dialog = false">Отмена</v-btn>
        <v-btn
          color="primary"
          :disabled="!valid || !!jsonError"
          :loading="loading"
          @click="saveConfig"
        >
          Сохранить
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup>
import { ref, watch, computed } from 'vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['config-updated'])

const dialog = ref(false)
const form = ref(null)
const valid = ref(true)
const loading = ref(false)
const config = ref({})
const configJson = ref('')
const jsonError = ref(null)

// Initialize config when dialog opens
watch(dialog, (newVal) => {
  if (newVal) {
    resetConfig()
  }
})

function resetConfig() {
  config.value = { ...props.node.config } || {}
  config.value.zone = props.node.zone || ''
  configJson.value = JSON.stringify(props.node.config || {}, null, 2)
  jsonError.value = null
}

function validateJson(value) {
  try {
    JSON.parse(value)
    jsonError.value = null
  } catch (e) {
    jsonError.value = 'Невалидный JSON: ' + e.message
  }
}

async function saveConfig() {
  loading.value = true
  
  try {
    // Parse JSON if user edited it
    let finalConfig = { ...config.value }
    
    if (!jsonError.value && configJson.value) {
      try {
        finalConfig = JSON.parse(configJson.value)
      } catch (e) {
        // Use config object if JSON invalid
      }
    }

    emit('config-updated', finalConfig)
    dialog.value = false
  } finally {
    loading.value = false
  }
}
</script>

