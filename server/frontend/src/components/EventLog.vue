<template>
  <v-card>
    <v-card-title class="d-flex align-center">
      <v-icon icon="mdi-alert-circle" class="mr-2"></v-icon>
      События
      
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
      </v-btn-toggle>
    </v-card-title>

    <v-card-text class="pa-0">
      <v-list v-if="filteredEvents.length > 0" lines="two">
        <v-list-item
          v-for="event in filteredEvents"
          :key="event.id"
          :class="{'event-resolved': event.resolved_at}"
        >
          <template v-slot:prepend>
            <v-avatar :color="getLevelColor(event.level)">
              <v-icon :icon="getLevelIcon(event.level)" color="white"></v-icon>
            </v-avatar>
          </template>

          <v-list-item-title>
            {{ event.message }}
          </v-list-item-title>

          <v-list-item-subtitle>
            {{ event.node_id }} • {{ formatDateTime(event.created_at) }}
            <span v-if="event.resolved_at" class="text-success">
              • Решено {{ formatDistanceToNow(event.resolved_at) }}
            </span>
          </v-list-item-subtitle>

          <template v-slot:append>
            <v-btn
              v-if="!event.resolved_at"
              icon="mdi-check"
              size="small"
              variant="text"
              @click="$emit('resolve', event.id)"
            ></v-btn>
            
            <v-btn
              v-else
              icon="mdi-delete"
              size="small"
              variant="text"
              @click="$emit('delete', event.id)"
            ></v-btn>
          </template>
        </v-list-item>
      </v-list>

      <div v-else class="text-center pa-8 text-disabled">
        <v-icon icon="mdi-check-circle" size="64"></v-icon>
        <div class="mt-2">Нет событий</div>
      </div>
    </v-card-text>

    <v-card-actions v-if="hasActiveEvents">
      <v-spacer></v-spacer>
      <v-btn
        color="primary"
        variant="text"
        @click="$emit('resolve-all')"
      >
        Решить все
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup>
import { computed, ref } from 'vue'
import { formatDateTime, formatDistanceToNow } from '@/utils/time'
import { useVirtualList } from '@/composables/useVirtualList'

const props = defineProps({
  events: {
    type: Array,
    default: () => [],
  },
  limit: {
    type: Number,
    default: 10,
  },
})

defineEmits(['resolve', 'resolve-all', 'delete'])

const filter = ref('all')

// Filtered events
const filteredEvents = computed(() => {
  console.log('🔍 EventLog: filteredEvents computed called')
  console.log('🔍 EventLog: props.events:', props.events)
  console.log('🔍 EventLog: filter.value:', filter.value)
  
  let filtered = [...props.events]

  if (filter.value === 'active') {
    console.log('🔍 EventLog: Filtering active events')
    filtered = filtered.filter(e => !e.resolved_at)
  } else if (filter.value === 'critical') {
    console.log('🔍 EventLog: Filtering critical events')
    const criticalLevels = ['critical', 'emergency']
    console.log('🔍 EventLog: criticalLevels:', criticalLevels)
    
    try {
      console.log('🔍 EventLog: Starting critical filter')
      filtered = filtered.filter((e, index) => {
        console.log(`🔍 EventLog: Processing event ${index}:`, e)
        
        if (!e) {
          console.warn('EventLog.vue: critical filter - event is null/undefined:', e)
          return false
        }
        if (!e.level) {
          console.warn('EventLog.vue: critical filter - event.level is null/undefined:', e)
          return false
        }
        if (typeof e.level !== 'string') {
          console.warn('EventLog.vue: critical filter - event.level is not string:', e.level, typeof e.level)
          return false
        }
        if (!criticalLevels || !Array.isArray(criticalLevels)) {
          console.error('EventLog.vue: critical filter - criticalLevels is not array:', criticalLevels)
          return false
        }
        
        console.log(`🔍 EventLog: Checking includes for level: ${e.level}`)
        // Дополнительная проверка на undefined/null перед вызовом includes
        let includesResult = false
        if (criticalLevels && Array.isArray(criticalLevels) && e.level) {
          includesResult = criticalLevels.includes(e.level)
        }
        console.log(`🔍 EventLog: includes result:`, includesResult)
        
        return includesResult && !e.resolved_at
      })
      console.log('🔍 EventLog: Critical filter completed')
    } catch (error) {
      console.error('EventLog.vue: critical filter - Error in filter:', error)
      console.error('EventLog.vue: critical filter - filtered events:', filtered)
      console.error('EventLog.vue: critical filter - criticalLevels:', criticalLevels)
    }
  }

  // Limit number of events
  const result = filtered.slice(0, props.limit)
  console.log('🔍 EventLog: filteredEvents result:', result)
  return result
})

// Has active events
const hasActiveEvents = computed(() => {
  return props.events.some(e => !e.resolved_at)
})

// Get level color
function getLevelColor(level) {
  const colors = {
    info: 'blue',
    warning: 'orange',
    critical: 'red',
    emergency: 'purple',
  }
  return colors[level] || 'grey'
}

// Get level icon
function getLevelIcon(level) {
  const icons = {
    info: 'mdi-information',
    warning: 'mdi-alert',
    critical: 'mdi-alert-circle',
    emergency: 'mdi-fire',
  }
  return icons[level] || 'mdi-help-circle'
}
</script>

<style scoped>
.event-resolved {
  opacity: 0.6;
}
</style>

