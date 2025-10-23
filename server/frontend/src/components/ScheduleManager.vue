<template>
  <v-card>
    <v-card-title class="bg-gradient">
      <v-icon icon="mdi-calendar-clock" class="mr-2"></v-icon>
      Расписание узла: {{ node.node_id }}
      
      <v-spacer></v-spacer>
      
      <v-btn
        color="white"
        variant="outlined"
        prepend-icon="mdi-plus"
        @click="openAddDialog"
      >
        Добавить
      </v-btn>
    </v-card-title>

    <v-card-text>
      <!-- Текущее активное расписание -->
      <v-alert
        v-if="activeSchedule"
        type="success"
        variant="tonal"
        class="mb-4"
        prominent
      >
        <div class="d-flex align-center">
          <div class="flex-grow-1">
            <div class="text-h6">
              Сейчас активно: {{ activeSchedule.name }}
            </div>
            <div class="text-caption">
              {{ formatTime(activeSchedule.time_start) }} - {{ formatTime(activeSchedule.time_end) }}
            </div>
          </div>
          <v-chip color="success" variant="elevated">
            <v-icon icon="mdi-clock-check" start></v-icon>
            Активно
          </v-chip>
        </div>
      </v-alert>

      <!-- Список расписаний -->
      <v-list v-if="schedules.length > 0">
        <v-list-item
          v-for="schedule in schedules"
          :key="schedule.id"
          :class="schedule.enabled ? '' : 'text-disabled'"
        >
          <template #prepend>
            <v-avatar :color="schedule.enabled ? 'primary' : 'grey'" size="40">
              <v-icon icon="mdi-calendar" color="white"></v-icon>
            </v-avatar>
          </template>

          <v-list-item-title>
            {{ schedule.name }}
            <v-chip v-if="!schedule.enabled" size="x-small" class="ml-2">
              Выключено
            </v-chip>
          </v-list-item-title>

          <v-list-item-subtitle>
            <div>
              <v-icon icon="mdi-clock-outline" size="small"></v-icon>
              {{ formatTime(schedule.time_start) }} - {{ formatTime(schedule.time_end) }}
            </div>
            
            <div v-if="schedule.days_of_week" class="mt-1">
              <v-icon icon="mdi-calendar-range" size="small"></v-icon>
              {{ formatDays(schedule.days_of_week) }}
            </div>

            <div class="mt-1">
              <v-chip
                v-for="(value, key) in getMainConfig(schedule.config)"
                :key="key"
                size="x-small"
                class="mr-1"
                variant="outlined"
              >
                {{ key }}: {{ value }}
              </v-chip>
            </div>
          </v-list-item-subtitle>

          <template #append>
            <div class="d-flex align-center">
              <v-switch
                v-model="schedule.enabled"
                hide-details
                density="compact"
                color="primary"
                @change="toggleSchedule(schedule)"
              ></v-switch>

              <v-btn
                icon="mdi-pencil"
                size="small"
                variant="text"
                @click="editSchedule(schedule)"
              ></v-btn>

              <v-btn
                icon="mdi-delete"
                size="small"
                variant="text"
                color="error"
                @click="deleteSchedule(schedule)"
              ></v-btn>
            </div>
          </template>
        </v-list-item>
      </v-list>

      <div v-else class="text-center pa-8 text-disabled">
        <v-icon icon="mdi-calendar-blank" size="64" class="mb-4"></v-icon>
        <div>Расписаний нет</div>
        <div class="text-caption mt-2">Добавьте первое расписание</div>
      </div>
    </v-card-text>

    <!-- Диалог добавления/редактирования -->
    <v-dialog v-model="scheduleDialog" max-width="600">
      <v-card>
        <v-card-title>
          {{ editingSchedule ? 'Редактировать' : 'Добавить' }} расписание
        </v-card-title>

        <v-card-text>
          <v-text-field
            v-model="scheduleForm.name"
            label="Название"
            placeholder="Например: День, Ночь"
            variant="outlined"
            class="mb-4"
          ></v-text-field>

          <v-row>
            <v-col cols="6">
              <v-text-field
                v-model="scheduleForm.time_start"
                label="Время начала"
                type="time"
                variant="outlined"
              ></v-text-field>
            </v-col>
            <v-col cols="6">
              <v-text-field
                v-model="scheduleForm.time_end"
                label="Время окончания"
                type="time"
                variant="outlined"
              ></v-text-field>
            </v-col>
          </v-row>

          <div v-if="node.node_type === 'ph' || node.node_type === 'ph_ec'">
            <v-text-field
              v-model.number="scheduleForm.config.ph_target"
              label="Целевой pH"
              type="number"
              step="0.1"
              variant="outlined"
            ></v-text-field>
          </div>

          <div v-if="node.node_type === 'ec' || node.node_type === 'ph_ec'">
            <v-text-field
              v-model.number="scheduleForm.config.ec_target"
              label="Целевой EC"
              type="number"
              step="0.1"
              variant="outlined"
            ></v-text-field>
          </div>

          <v-slider
            v-model="scheduleForm.priority"
            label="Приоритет"
            min="0"
            max="100"
            step="1"
            thumb-label
            variant="outlined"
          ></v-slider>

          <v-switch
            v-model="scheduleForm.enabled"
            label="Включено"
            color="primary"
          ></v-switch>
        </v-card-text>

        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="scheduleDialog = false">Отмена</v-btn>
          <v-btn
            color="primary"
            :loading="loading"
            @click="saveSchedule"
          >
            Сохранить
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-card>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useToast } from 'vue-toastification'
import { axios as api } from '@/services/api'

const props = defineProps({
  node: {
    type: Object,
    required: true
  }
})

const toast = useToast()
const loading = ref(false)
const schedules = ref([])
const activeSchedule = ref(null)
const scheduleDialog = ref(false)
const editingSchedule = ref(null)
const scheduleForm = ref({
  name: '',
  time_start: '08:00',
  time_end: '20:00',
  config: {},
  enabled: true,
  priority: 50
})

// Загрузка расписаний
const loadSchedules = async () => {
  try {
    const response = await api.get(`/schedules/node/${props.node.node_id}`)
    if (response.data.success) {
      schedules.value = response.data.schedules
    }

    // Загрузка активного расписания
    const activeResponse = await api.get(`/schedules/node/${props.node.node_id}/active`)
    if (activeResponse.data.success) {
      activeSchedule.value = activeResponse.data.active_schedule
    }
  } catch (error) {
    console.error('Failed to load schedules:', error)
  }
}

// Открыть диалог добавления
const openAddDialog = () => {
  editingSchedule.value = null
  scheduleForm.value = {
    name: '',
    time_start: '08:00',
    time_end: '20:00',
    config: {
      ph_target: props.node.config?.ph_target || 6.5,
      ec_target: props.node.config?.ec_target || 2.0
    },
    enabled: true,
    priority: 50
  }
  scheduleDialog.value = true
}

// Редактировать расписание
const editSchedule = (schedule) => {
  editingSchedule.value = schedule
  scheduleForm.value = { ...schedule }
  scheduleDialog.value = true
}

// Сохранить расписание
const saveSchedule = async () => {
  loading.value = true
  try {
    if (editingSchedule.value) {
      await api.put(
        `/schedules/node/${props.node.node_id}/${editingSchedule.value.id}`,
        scheduleForm.value
      )
      toast.success('Расписание обновлено')
    } else {
      await api.post(
        `/schedules/node/${props.node.node_id}`,
        scheduleForm.value
      )
      toast.success('Расписание создано')
    }

    scheduleDialog.value = false
    await loadSchedules()
  } catch (error) {
    toast.error('Failed to save schedule')
    console.error('Save schedule error:', error)
  } finally {
    loading.value = false
  }
}

// Переключить расписание
const toggleSchedule = async (schedule) => {
  try {
    await api.put(
      `/schedules/node/${props.node.node_id}/${schedule.id}`,
      { enabled: schedule.enabled }
    )
    toast.success(schedule.enabled ? 'Расписание включено' : 'Расписание выключено')
  } catch (error) {
    toast.error('Failed to toggle schedule')
    schedule.enabled = !schedule.enabled // Откат
  }
}

// Удалить расписание
const deleteSchedule = async (schedule) => {
  if (!confirm(`Удалить расписание "${schedule.name}"?`)) return

  try {
    await api.delete(`/schedules/node/${props.node.node_id}/${schedule.id}`)
    toast.success('Расписание удалено')
    await loadSchedules()
  } catch (error) {
    toast.error('Failed to delete schedule')
  }
}

// Форматирование времени
const formatTime = (time) => {
  if (!time) return 'N/A'
  const date = new Date(`2000-01-01T${time}`)
  return date.toLocaleTimeString('ru-RU', { hour: '2-digit', minute: '2-digit' })
}

// Форматирование дней недели
const formatDays = (days) => {
  if (!days || days.length === 7) return 'Каждый день'
  const dayNames = ['Пн', 'Вт', 'Ср', 'Чт', 'Пт', 'Сб', 'Вс']
  return days.map(d => dayNames[d - 1]).join(', ')
}

// Получить основные параметры конфига
const getMainConfig = (config) => {
  const result = {}
  if (config.ph_target) result['pH'] = config.ph_target.toFixed(1)
  if (config.ec_target) result['EC'] = config.ec_target.toFixed(1)
  return result
}

// При монтировании
onMounted(() => {
  loadSchedules()
  
  // Обновлять каждую минуту
  setInterval(loadSchedules, 60000)
})
</script>

<style scoped>
.bg-gradient {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}
</style>

