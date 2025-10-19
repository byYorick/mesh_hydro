<template>
  <v-dialog v-model="dialog" max-width="600" scrollable>
    <template v-slot:activator="{ props }">
      <slot name="activator" :props="props"></slot>
    </template>

    <v-card>
      <v-card-title class="bg-secondary">
        <v-icon icon="mdi-pencil" class="mr-2"></v-icon>
        Редактировать узел: {{ node?.node_id }}
      </v-card-title>

      <v-card-text class="pt-4">
        <v-form ref="form" v-model="valid">
          <!-- Zone -->
          <v-select
            v-model="editedNode.zone"
            :items="zones"
            label="Зона"
            variant="outlined"
          >
            <template v-slot:append>
              <v-btn
                icon="mdi-plus"
                size="small"
                variant="text"
                @click="addZoneDialog = true"
              ></v-btn>
            </template>
          </v-select>

          <!-- MAC Address -->
          <v-text-field
            v-model="editedNode.mac_address"
            label="MAC адрес"
            variant="outlined"
            :rules="[rules.macAddress]"
            hint="Формат: AA:BB:CC:DD:EE:FF"
            class="mt-4"
          ></v-text-field>

          <!-- Description -->
          <v-textarea
            v-model="editedNode.description"
            label="Описание"
            variant="outlined"
            rows="2"
            class="mt-4"
          ></v-textarea>

          <v-divider class="my-4"></v-divider>

          <!-- Metadata -->
          <h4 class="mb-2">Метаданные</h4>

          <v-text-field
            v-model="editedNode.metadata.firmware"
            label="Версия прошивки"
            variant="outlined"
            hint="Пример: 1.0.0"
          ></v-text-field>

          <v-text-field
            v-model="editedNode.metadata.hardware"
            label="Аппаратная платформа"
            variant="outlined"
            hint="Пример: ESP32-WROOM-32"
            class="mt-4"
          ></v-text-field>

          <!-- Advanced JSON editor -->
          <v-expansion-panels class="mt-4">
            <v-expansion-panel>
              <v-expansion-panel-title>
                <v-icon icon="mdi-code-json" class="mr-2"></v-icon>
                Метаданные (JSON)
              </v-expansion-panel-title>
              <v-expansion-panel-text>
                <v-textarea
                  v-model="metadataJson"
                  rows="6"
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
        <v-btn
          color="error"
          prepend-icon="mdi-delete"
          @click="confirmDelete = true"
        >
          Удалить
        </v-btn>
        
        <v-spacer></v-spacer>
        
        <v-btn @click="dialog = false">Отмена</v-btn>
        <v-btn
          color="primary"
          :disabled="!valid || !!jsonError"
          :loading="loading"
          @click="saveChanges"
        >
          Сохранить
        </v-btn>
      </v-card-actions>
    </v-card>

    <!-- Add Zone Dialog -->
    <v-dialog v-model="addZoneDialog" max-width="400">
      <v-card>
        <v-card-title>Добавить зону</v-card-title>
        <v-card-text>
          <v-text-field
            v-model="newZone"
            label="Название зоны"
            variant="outlined"
          ></v-text-field>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="addZoneDialog = false">Отмена</v-btn>
          <v-btn color="primary" @click="addZone">Добавить</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Delete Confirmation -->
    <v-dialog v-model="confirmDelete" max-width="400">
      <v-card>
        <v-card-title class="bg-error">
          <v-icon icon="mdi-alert" class="mr-2"></v-icon>
          Удалить узел?
        </v-card-title>
        <v-card-text class="pt-4">
          Вы уверены что хотите удалить узел <strong>{{ node?.node_id }}</strong>?
          <br><br>
          <v-alert type="error" variant="tonal">
            <strong>Внимание!</strong> Это действие необратимо. Все данные узла (телеметрия, события, команды) будут удалены.
          </v-alert>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="confirmDelete = false">Отмена</v-btn>
          <v-btn color="error" @click="deleteNode" :loading="deleting">Удалить</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-dialog>
</template>

<script setup>
import { ref, watch } from 'vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['node-updated', 'node-deleted'])

const dialog = ref(false)
const form = ref(null)
const valid = ref(true)
const loading = ref(false)
const deleting = ref(false)
const addZoneDialog = ref(false)
const newZone = ref('')
const confirmDelete = ref(false)
const jsonError = ref(null)
const metadataJson = ref('')

const zones = ref(['Zone 1', 'Zone 2', 'Main', 'Greenhouse', 'Nursery'])

const editedNode = ref({
  zone: '',
  mac_address: '',
  description: '',
  metadata: {},
})

const rules = {
  macAddress: v => {
    if (!v) return true
    const pattern = /^([0-9A-F]{2}:){5}[0-9A-F]{2}$/i
    return pattern.test(v) || 'Неверный формат MAC адреса'
  },
}

watch(dialog, (val) => {
  if (val) {
    resetForm()
  }
})

function resetForm() {
  editedNode.value = {
    zone: props.node.zone || '',
    mac_address: props.node.mac_address || '',
    description: props.node.metadata?.description || '',
    metadata: { ...props.node.metadata } || {},
  }
  metadataJson.value = JSON.stringify(props.node.metadata || {}, null, 2)
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

function addZone() {
  if (newZone.value && !zones.value.includes(newZone.value)) {
    zones.value.push(newZone.value)
    editedNode.value.zone = newZone.value
  }
  addZoneDialog.value = false
  newZone.value = ''
}

async function saveChanges() {
  if (!form.value.validate()) return

  loading.value = true

  try {
    let metadata = editedNode.value.metadata
    
    if (!jsonError.value && metadataJson.value) {
      try {
        metadata = JSON.parse(metadataJson.value)
      } catch (e) {
        // Use object if JSON invalid
      }
    }

    const updateData = {
      zone: editedNode.value.zone,
      mac_address: editedNode.value.mac_address || null,
      metadata: {
        ...metadata,
        description: editedNode.value.description,
      },
    }

    emit('node-updated', updateData)
    dialog.value = false
  } finally {
    loading.value = false
  }
}

async function deleteNode() {
  deleting.value = true
  
  try {
    emit('node-deleted', props.node.node_id)
    confirmDelete.value = false
    dialog.value = false
  } finally {
    deleting.value = false
  }
}
</script>

