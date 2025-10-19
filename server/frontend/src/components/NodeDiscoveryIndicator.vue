<template>
  <v-snackbar
    v-model="show"
    :timeout="8000"
    location="top right"
    color="success"
    variant="elevated"
  >
    <div class="d-flex align-center">
      <v-icon size="24" class="mr-3">mdi-radar</v-icon>
      <div>
        <div class="text-subtitle-1 font-weight-bold">
          🔍 Новый узел обнаружен!
        </div>
        <div class="text-body-2">
          {{ discoveredNode?.node_id }} ({{ nodeTypeLabel }})
        </div>
        <div class="text-caption">
          {{ discoveredNode?.zone }} • 
          {{ discoveredNode?.discovered_via === 'heartbeat' ? 'Heartbeat' : 'Discovery Topic' }}
        </div>
      </div>
    </div>

    <template v-slot:actions>
      <v-btn
        variant="text"
        size="small"
        @click="goToNode"
      >
        Открыть
      </v-btn>
      <v-btn
        icon="mdi-close"
        variant="text"
        size="small"
        @click="show = false"
      />
    </template>
  </v-snackbar>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'

const router = useRouter()
const show = ref(false)
const discoveredNode = ref(null)

const nodeTypeLabel = computed(() => {
  const types = {
    root: 'Root Node',
    climate: 'Климат-сенсор',
    ph_ec: 'pH/EC сенсор',
    relay: 'Реле',
    water: 'Датчик воды',
    display: 'Дисплей',
    unknown: 'Неизвестный тип'
  }
  return types[discoveredNode.value?.node_type] || 'Неизвестный'
})

const goToNode = () => {
  if (discoveredNode.value?.node_id) {
    router.push(`/nodes/${discoveredNode.value.node_id}`)
  }
  show.value = false
}

const handleNodeDiscovered = (data) => {
  console.log('🔍 Node discovered event:', data)
  discoveredNode.value = data.node
  show.value = true
  
  // Воспроизводим звук (опционально)
  playDiscoverySound()
}

const playDiscoverySound = () => {
  try {
    const audio = new Audio('/discovery-sound.mp3') // Опционально: добавить звук
    audio.volume = 0.3
    audio.play().catch(() => {
      // Ignore if sound fails
    })
  } catch (e) {
    // Ignore sound errors
  }
}

// Подключаемся к WebSocket для real-time уведомлений
onMounted(() => {
  // TODO: Подключение к Laravel Echo для real-time
  // Echo.channel('hydro-system')
  //   .listen('.node.discovered', handleNodeDiscovered)
  
  // Временно: эмулируем событие для демонстрации
  window.addEventListener('node-discovered', (event) => {
    handleNodeDiscovered(event.detail)
  })
})

// Экспортируем метод для ручного вызова
defineExpose({
  showDiscovery: handleNodeDiscovered
})
</script>

<style scoped>
.v-snackbar {
  min-width: 400px;
}
</style>


