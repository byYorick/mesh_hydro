<template>
  <div>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3 mb-4">События</h1>
      </v-col>
    </v-row>

    <!-- Stats Cards -->
    <v-row>
      <v-col cols="12" sm="6" md="3">
        <v-card color="primary" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ eventsStore.events.length }}
            </div>
            <div class="text-subtitle-1">Всего событий</div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" sm="6" md="3">
        <v-card color="warning" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ eventsStore.activeEvents.length }}
            </div>
            <div class="text-subtitle-1">Активных</div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" sm="6" md="3">
        <v-card color="error" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ eventsStore.criticalCount }}
            </div>
            <div class="text-subtitle-1">Критичных</div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" sm="6" md="3">
        <v-card color="success" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ eventsStore.resolvedEvents.length }}
            </div>
            <div class="text-subtitle-1">Решено</div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Events Table -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title class="d-flex align-center">
            <span>Список событий</span>
            
            <v-spacer></v-spacer>
            
            <!-- Filter buttons -->
            <v-btn-toggle
              v-model="filter"
              density="compact"
              variant="outlined"
            >
              <v-btn value="all">Все</v-btn>
              <v-btn value="active">Активные</v-btn>
              <v-btn value="critical">Критичные</v-btn>
              <v-btn value="resolved">Решено</v-btn>
            </v-btn-toggle>
          </v-card-title>

          <v-card-text>
            <v-data-table
              :headers="headers"
              :items="filteredEvents"
              :items-per-page="20"
              :loading="eventsStore.loading"
              @click:row="selectEvent"
            >
              <template v-slot:item.level="{ item }">
                <v-chip :color="getLevelColor(item.level)" size="small">
                  <v-icon :icon="getLevelIcon(item.level)" start size="small"></v-icon>
                  {{ item.level }}
                </v-chip>
              </template>

              <template v-slot:item.created_at="{ item }">
                {{ formatDateTime(item.created_at) }}
              </template>

              <template v-slot:item.resolved_at="{ item }">
                <span v-if="item.resolved_at">
                  {{ formatDateTime(item.resolved_at) }}
                </span>
                <v-chip v-else color="warning" size="small">Активно</v-chip>
              </template>

              <template v-slot:item.actions="{ item }">
                <v-btn
                  v-if="!item.resolved_at"
                  icon="mdi-check"
                  size="small"
                  variant="text"
                  @click.stop="resolveEvent(item.id)"
                ></v-btn>
                <v-btn
                  v-else
                  icon="mdi-delete"
                  size="small"
                  variant="text"
                  @click.stop="deleteEvent(item.id)"
                ></v-btn>
              </template>
            </v-data-table>
          </v-card-text>

          <v-card-actions v-if="hasActiveEvents">
            <v-spacer></v-spacer>
            <v-btn
              color="primary"
              @click="resolveAllEvents"
            >
              Решить все активные
            </v-btn>
          </v-card-actions>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useEventsStore } from '@/stores/events'
import { useAppStore } from '@/stores/app'
import { formatDateTime } from '@/utils/time'

const eventsStore = useEventsStore()
const appStore = useAppStore()

const filter = ref('all')

const headers = [
  { title: 'ID', key: 'id', width: 80 },
  { title: 'Узел', key: 'node_id' },
  { title: 'Уровень', key: 'level' },
  { title: 'Сообщение', key: 'message' },
  { title: 'Создано', key: 'created_at' },
  { title: 'Решено', key: 'resolved_at' },
  { title: 'Действия', key: 'actions', sortable: false, width: 100 },
]

const filteredEvents = computed(() => {
  switch (filter.value) {
    case 'active':
      return eventsStore.activeEvents
    case 'critical':
      return eventsStore.criticalEvents
    case 'resolved':
      return eventsStore.resolvedEvents
    default:
      return eventsStore.events
  }
})

const hasActiveEvents = computed(() => {
  return eventsStore.activeEvents.length > 0
})

onMounted(async () => {
  await eventsStore.fetchEvents()
})

function getLevelColor(level) {
  const colors = {
    info: 'blue',
    warning: 'orange',
    critical: 'red',
    emergency: 'purple',
  }
  return colors[level] || 'grey'
}

function getLevelIcon(level) {
  const icons = {
    info: 'mdi-information',
    warning: 'mdi-alert',
    critical: 'mdi-alert-circle',
    emergency: 'mdi-fire',
  }
  return icons[level] || 'mdi-help-circle'
}

function selectEvent(event, row) {
  // TODO: Open event detail dialog
  console.log('Selected event:', row.item)
}

async function resolveEvent(eventId) {
  try {
    await eventsStore.resolveEvent(eventId)
    appStore.showSnackbar('Событие решено', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка', 'error')
  }
}

async function deleteEvent(eventId) {
  try {
    await eventsStore.deleteEvent(eventId)
    appStore.showSnackbar('Событие удалено', 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка', 'error')
  }
}

async function resolveAllEvents() {
  try {
    const activeIds = eventsStore.activeEvents.map(e => e.id)
    await eventsStore.resolveBulk(activeIds)
    appStore.showSnackbar(`Решено ${activeIds.length} событий`, 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка', 'error')
  }
}
</script>

