<template>
  <div>
    <!-- Quick Actions based on node type -->
    <v-card v-if="node" class="mb-4">
      <v-card-title>
        <v-icon icon="mdi-lightning-bolt" class="mr-2"></v-icon>
        Быстрые действия
      </v-card-title>

      <v-card-text>
        <v-row dense>
          <!-- pH/EC Node Actions -->
          <template v-if="node.node_type === 'ph_ec'">
            <v-col cols="12" sm="6" md="4">
              <v-btn
                block
                color="primary"
                prepend-icon="mdi-flask"
                @click="openCalibratePh"
                :disabled="!isOnline"
              >
                Калибровка pH
              </v-btn>
            </v-col>

            <v-col cols="12" sm="6" md="4">
              <v-btn
                block
                color="primary"
                prepend-icon="mdi-waves"
                @click="openCalibrateEc"
                :disabled="!isOnline"
              >
                Калибровка EC
              </v-btn>
            </v-col>

            <v-col cols="12" sm="6" md="4">
              <v-menu>
                <template v-slot:activator="{ props }">
                  <v-btn
                    block
                    color="info"
                    prepend-icon="mdi-pump"
                    v-bind="props"
                    :disabled="!isOnline"
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
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'
import ConfigEditor from './ConfigEditor.vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['command', 'config-update'])

const config = ref({})
const pumpDialog = ref(false)
const rebootDialog = ref(false)
const selectedPump = ref(null)
const pumpDuration = ref(5)

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
</script>

