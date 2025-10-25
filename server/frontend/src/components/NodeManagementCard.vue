<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-cog-outline" class="mr-2"></v-icon>
      Управление узлом
      
      <v-spacer></v-spacer>
      
      <v-chip :color="node.online ? 'success' : 'error'" size="small">
        {{ node.online ? 'Online' : 'Offline' }}
      </v-chip>
    </v-card-title>

    <v-card-text>
      <v-list density="compact">
        <v-list-item>
          <v-list-item-title>Node ID</v-list-item-title>
          <v-list-item-subtitle>{{ node.node_id }}</v-list-item-subtitle>
        </v-list-item>

        <v-list-item>
          <v-list-item-title>Тип</v-list-item-title>
          <v-list-item-subtitle>{{ getNodeTypeName(node.node_type) }}</v-list-item-subtitle>
        </v-list-item>

        <v-list-item>
          <v-list-item-title>Зона</v-list-item-title>
          <v-list-item-subtitle>{{ node.zone || 'Не указана' }}</v-list-item-subtitle>
        </v-list-item>

        <v-list-item>
          <v-list-item-title>MAC адрес</v-list-item-title>
          <v-list-item-subtitle>{{ node.mac_address || 'Не указан' }}</v-list-item-subtitle>
        </v-list-item>

        <v-list-item>
          <v-list-item-title>Последняя связь</v-list-item-title>
          <v-list-item-subtitle>{{ formatDateTime(node.last_seen_at) }}</v-list-item-subtitle>
        </v-list-item>

        <v-list-item v-if="node.metadata?.firmware">
          <v-list-item-title>Прошивка</v-list-item-title>
          <v-list-item-subtitle>{{ node.metadata.firmware }}</v-list-item-subtitle>
        </v-list-item>
      </v-list>
    </v-card-text>

    <v-divider></v-divider>

    <v-card-actions>
      <v-row dense>
        <!-- Edit Node -->
        <v-col cols="12" sm="6">
          <EditNodeDialog :node="node" @node-updated="handleUpdate" @node-deleted="handleDelete">
            <template v-slot:activator="{ props }">
              <v-btn
                block
                variant="elevated"
                color="secondary"
                v-bind="props"
                prepend-icon="mdi-pencil"
                size="large"
                class="text-none font-weight-medium"
              >
                <div class="d-flex flex-column align-center">
                  <span>Редактировать</span>
                  <span class="text-caption">Изменить настройки</span>
                </div>
              </v-btn>
            </template>
          </EditNodeDialog>
        </v-col>

        <!-- Delete Node -->
        <v-col cols="12" sm="6">
          <v-btn
            block
            variant="elevated"
            color="error"
            prepend-icon="mdi-delete"
            @click="confirmDelete = true"
            size="large"
            class="text-none font-weight-medium"
          >
            <div class="d-flex flex-column align-center">
              <span>Удалить узел</span>
              <span class="text-caption">Безвозвратно</span>
            </div>
          </v-btn>
        </v-col>
      </v-row>
    </v-card-actions>

    <!-- Delete Confirmation -->
    <v-dialog v-model="confirmDelete" max-width="500">
      <v-card>
        <v-card-title class="bg-error">
          <v-icon icon="mdi-alert-circle" class="mr-2"></v-icon>
          Подтверждение удаления
        </v-card-title>
        <v-card-text class="pt-4">
          <p>Вы уверены что хотите удалить узел <strong>{{ node.node_id }}</strong>?</p>
          
          <v-alert type="error" variant="tonal" class="mt-4">
            <strong>Будет удалено:</strong>
            <ul class="mt-2">
              <li>Узел из системы</li>
              <li>Вся телеметрия узла</li>
              <li>Все события узла</li>
              <li>История команд</li>
            </ul>
            <br>
            <strong>Это действие необратимо!</strong>
          </v-alert>

          <v-checkbox
            v-model="deleteConfirmed"
            label="Я понимаю что данные будут удалены безвозвратно"
            color="error"
            class="mt-4"
          ></v-checkbox>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="confirmDelete = false">Отмена</v-btn>
          <v-btn
            color="error"
            :disabled="!deleteConfirmed"
            :loading="deleting"
            @click="executeDelete"
          >
            Удалить навсегда
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-card>
</template>

<script setup>
import { ref } from 'vue'
import EditNodeDialog from './EditNodeDialog.vue'
import { formatDateTime } from '@/utils/time'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['node-updated', 'node-deleted'])

const confirmDelete = ref(false)
const deleteConfirmed = ref(false)
const deleting = ref(false)

const nodeTypeNames = {
  'root': 'Root Node',
  'ph_ec': 'pH/EC Сенсор',
  'climate': 'Климат-контроль',
  'relay': 'Реле',
  'water': 'Уровень воды',
  'display': 'Дисплей',
}

function getNodeTypeName(type) {
  return nodeTypeNames[type] || type
}

function handleUpdate(updateData) {
  emit('node-updated', updateData)
}

function handleDelete(nodeId) {
  emit('node-deleted', nodeId)
}

async function executeDelete() {
  deleting.value = true
  
  try {
    emit('node-deleted', props.node.node_id)
    confirmDelete.value = false
    deleteConfirmed.value = false
  } finally {
    deleting.value = false
  }
}
</script>

