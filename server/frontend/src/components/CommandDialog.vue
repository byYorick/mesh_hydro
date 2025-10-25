<template>
  <v-dialog v-model="dialog" max-width="600">
    <template v-slot:activator="{ props }">
      <slot name="activator" :props="props"></slot>
    </template>

    <v-card>
      <v-card-title class="bg-primary">
        <v-icon icon="mdi-send" class="mr-2"></v-icon>
        Отправить команду: {{ node?.node_id }}
      </v-card-title>

      <v-card-text class="pt-4">
        <!-- Command selection -->
        <v-select
          v-model="selectedCommand"
          :items="availableCommands"
          item-title="label"
          item-value="value"
          label="Выберите команду"
          variant="outlined"
        ></v-select>

        <!-- Command parameters (dynamic based on command) -->
        <div v-if="selectedCommand" class="mt-4">
          <h4 class="mb-2">Параметры команды</h4>

          <!-- Calibrate pH -->
          <v-text-field
            v-if="selectedCommand === 'calibrate_ph'"
            v-model.number="params.ph_value"
            label="pH значение"
            type="number"
            step="0.01"
            variant="outlined"
            hint="Введите значение буферного раствора (4.0, 7.0, 10.0)"
          ></v-text-field>

          <!-- Calibrate EC -->
          <v-text-field
            v-if="selectedCommand === 'calibrate_ec'"
            v-model.number="params.ec_value"
            label="EC значение"
            type="number"
            step="0.01"
            variant="outlined"
            hint="Введите значение калибровочного раствора (1.413)"
          ></v-text-field>

          <!-- Pump control -->
          <v-select
            v-if="selectedCommand === 'run_pump'"
            v-model="params.pump"
            :items="pumpOptions"
            label="Выберите насос"
            variant="outlined"
          ></v-select>

          <v-text-field
            v-if="selectedCommand === 'run_pump'"
            v-model.number="params.duration"
            label="Длительность (секунды)"
            type="number"
            variant="outlined"
          ></v-text-field>

          <!-- Relay control -->
          <v-select
            v-if="selectedCommand === 'toggle_relay'"
            v-model.number="params.relay_id"
            :items="[1, 2, 3, 4]"
            label="Номер реле"
            variant="outlined"
          ></v-select>

          <v-radio-group
            v-if="selectedCommand === 'toggle_relay'"
            v-model="params.state"
            inline
          >
            <v-radio label="Включить" value="on"></v-radio>
            <v-radio label="Выключить" value="off"></v-radio>
          </v-radio-group>

          <!-- No params needed -->
          <v-alert
            v-if="selectedCommand && isNoParamsCommand(selectedCommand)"
            type="info"
            variant="tonal"
          >
            Эта команда не требует параметров
          </v-alert>
        </div>

        <!-- Command confirmation -->
        <v-alert
          v-if="selectedCommand"
          type="warning"
          variant="tonal"
          class="mt-4"
        >
          <strong>Внимание!</strong> Команда будет отправлена узлу немедленно.
        </v-alert>
      </v-card-text>

      <v-card-actions>
        <v-spacer></v-spacer>
        <v-btn @click="dialog = false">Отмена</v-btn>
        <v-btn
          color="primary"
          :disabled="!selectedCommand"
          :loading="loading"
          @click="sendCommand"
        >
          Отправить
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup>
import { ref, computed } from 'vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['command-sent'])

const dialog = ref(false)
const selectedCommand = ref(null)
const params = ref({})
const loading = ref(false)

const pumpOptions = ['ph_up', 'ph_down', 'ec_up', 'water']

// Available commands based on node type
const availableCommands = computed(() => {
  const commands = []

  if (props.node.node_type === 'ph_ec') {
    commands.push(
      { label: 'Калибровка pH', value: 'calibrate_ph' },
      { label: 'Калибровка EC', value: 'calibrate_ec' },
      { label: 'Запустить насос', value: 'run_pump' },
      { label: 'Сброс ошибок', value: 'reset_errors' },
    )
  }

  if (props.node.node_type === 'relay') {
    commands.push(
      { label: 'Переключить реле', value: 'toggle_relay' },
      { label: 'Открыть все окна', value: 'open_all' },
      { label: 'Закрыть все окна', value: 'close_all' },
    )
  }

  if (props.node.node_type === 'climate') {
    commands.push(
      { label: 'Обновить датчики', value: 'update_sensors' },
    )
  }

  // Common commands for all nodes
  commands.push(
    { label: 'Получить статус', value: 'get_status' },
    { label: 'Перезагрузка', value: 'reboot' },
  )

  return commands
})

// Check if command requires no parameters
function isNoParamsCommand(command) {
  if (!command || typeof command !== 'string') return false
  const noParamsCommands = ['reboot', 'get_status', 'reset_errors']
  try {
    // Дополнительная проверка на undefined/null перед вызовом includes
    if (noParamsCommands && Array.isArray(noParamsCommands) && command) {
      return noParamsCommands.includes(command)
    }
    return false
  } catch (error) {
    console.error('CommandDialog.vue: isNoParamsCommand - Error in includes:', error)
    console.error('CommandDialog.vue: isNoParamsCommand - command:', command, typeof command)
    console.error('CommandDialog.vue: isNoParamsCommand - noParamsCommands:', noParamsCommands, typeof noParamsCommands)
    return false
  }
}

async function sendCommand() {
  loading.value = true
  
  try {
    emit('command-sent', {
      command: selectedCommand.value,
      params: params.value,
    })
    
    dialog.value = false
    selectedCommand.value = null
    params.value = {}
  } finally {
    loading.value = false
  }
}
</script>

