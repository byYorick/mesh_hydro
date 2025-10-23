<template>
  <v-dialog v-model="dialog" max-width="900" scrollable>
    <template #activator="{ props }">
      <v-btn v-bind="props" variant="outlined" prepend-icon="mdi-history">
        История изменений
      </v-btn>
    </template>

    <v-card>
      <v-card-title class="bg-primary">
        <v-icon icon="mdi-history" class="mr-2"></v-icon>
        История конфигурации: {{ node.node_id }}
      </v-card-title>

      <v-card-text class="pa-0">
        <v-list v-if="!loading && history.length > 0">
          <v-list-item
            v-for="(item, index) in history"
            :key="item.id"
            :class="index % 2 === 0 ? 'bg-grey-lighten-5' : ''"
          >
            <template #prepend>
              <v-avatar :color="getColorForType(item.change_type)" size="40">
                <v-icon :icon="getIconForType(item.change_type)" color="white"></v-icon>
              </v-avatar>
            </template>

            <v-list-item-title>
              {{ getTypeLabel(item.change_type) }}
            </v-list-item-title>

            <v-list-item-subtitle>
              <div class="text-caption">
                <v-icon icon="mdi-account" size="x-small"></v-icon>
                {{ item.user_id }}
                •
                <v-icon icon="mdi-clock-outline" size="x-small"></v-icon>
                {{ formatDate(item.changed_at) }}
              </div>
              
              <div v-if="item.comment" class="mt-1 text-body-2">
                💬 {{ item.comment }}
              </div>

              <!-- Изменения -->
              <div v-if="item.changes && Object.keys(item.changes).length > 0" class="mt-2">
                <v-chip
                  v-for="(change, key) in item.changes"
                  :key="key"
                  size="small"
                  class="mr-1 mb-1"
                  variant="outlined"
                >
                  <span class="text-caption">
                    {{ key }}:
                    <span class="text-red">{{ formatValue(change.old) }}</span>
                    →
                    <span class="text-green">{{ formatValue(change.new) }}</span>
                  </span>
                </v-chip>
              </div>
            </v-list-item-subtitle>

            <template #append>
              <v-btn
                icon="mdi-restore"
                size="small"
                variant="text"
                @click="restoreConfig(item)"
              ></v-btn>
            </template>
          </v-list-item>
        </v-list>

        <div v-else-if="loading" class="text-center pa-8">
          <v-progress-circular indeterminate color="primary"></v-progress-circular>
          <div class="mt-2">Загрузка истории...</div>
        </div>

        <div v-else class="text-center pa-8 text-disabled">
          <v-icon icon="mdi-history" size="64" class="mb-4"></v-icon>
          <div>История изменений пуста</div>
        </div>
      </v-card-text>

      <v-card-actions>
        <v-spacer></v-spacer>
        <v-btn @click="dialog = false">Закрыть</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup>
import { ref, watch } from 'vue'
import { useToast } from 'vue-toastification'
import { axios as api } from '@/services/api'

const props = defineProps({
  node: {
    type: Object,
    required: true
  }
})

const emit = defineEmits(['config-restored'])

const toast = useToast()
const dialog = ref(false)
const loading = ref(false)
const history = ref([])

// Загрузка истории при открытии диалога
watch(dialog, async (newVal) => {
  if (newVal) {
    await loadHistory()
  }
})

// Загрузка истории
const loadHistory = async () => {
  loading.value = true
  try {
    const response = await api.get(`/nodes/${props.node.node_id}/config/history`)
    if (response.data.success) {
      history.value = response.data.history
    }
  } catch (error) {
    toast.error('Failed to load config history')
    console.error('Load history error:', error)
  } finally {
    loading.value = false
  }
}

// Восстановление конфигурации
const restoreConfig = (item) => {
  if (!confirm(`Восстановить конфигурацию от ${formatDate(item.changed_at)}?`)) {
    return
  }

  emit('config-restored', item.old_config)
  toast.success('Конфигурация восстановлена')
  dialog.value = false
}

// Получить цвет для типа изменения
const getColorForType = (type) => {
  switch (type) {
    case 'update_config': return 'blue'
    case 'calibrate_pump': return 'green'
    case 'set_target': return 'purple'
    case 'set_schedule': return 'orange'
    default: return 'grey'
  }
}

// Получить иконку для типа
const getIconForType = (type) => {
  switch (type) {
    case 'update_config': return 'mdi-cog'
    case 'calibrate_pump': return 'mdi-water-pump'
    case 'set_target': return 'mdi-target'
    case 'set_schedule': return 'mdi-calendar-clock'
    default: return 'mdi-pencil'
  }
}

// Получить читаемое название типа
const getTypeLabel = (type) => {
  switch (type) {
    case 'update_config': return 'Обновление конфигурации'
    case 'calibrate_pump': return 'Калибровка насоса'
    case 'set_target': return 'Изменение целевого значения'
    case 'set_schedule': return 'Изменение расписания'
    default: return type
  }
}

// Форматирование значения
const formatValue = (value) => {
  if (value === null || value === undefined) return 'N/A'
  if (typeof value === 'number') return value.toFixed(2)
  if (typeof value === 'boolean') return value ? 'Да' : 'Нет'
  return value
}

// Форматирование даты
const formatDate = (dateString) => {
  if (!dateString) return 'N/A'
  const date = new Date(dateString)
  return date.toLocaleString('ru-RU')
}
</script>

<style scoped>
.bg-primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.text-red {
  color: #ef4444;
}

.text-green {
  color: #10b981;
}
</style>

