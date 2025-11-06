<template>
  <!-- Компонент-обертка для обнаружения узлов -->
  <!-- Уведомления отображаются через систему usePopup в UniversalPopup -->
</template>

<script setup>
import { usePopup } from '@/composables/usePopup'
import { useRouter } from 'vue-router'

const router = useRouter()
const popup = usePopup()

const nodeTypeLabel = (nodeType) => {
  const types = {
    root: 'Root Node',
    climate: 'Климат-сенсор',
    ph_ec: 'pH/EC сенсор',
    relay: 'Реле',
    water: 'Датчик воды',
    display: 'Дисплей',
    ph: 'pH сенсор',
    ec: 'EC сенсор',
    unknown: 'Неизвестный тип'
  }
  return types[nodeType] || 'Неизвестный'
}

const handleNodeDiscovered = (data) => {
  console.log('🔍 Node discovered event:', data)
  const node = data.node || data
  
  if (!node?.node_id) return
  
  // Используем новую систему usePopup
  popup.toast.add({
    level: 'info',
    message: `🔍 Новый узел обнаружен: ${node.node_id} (${nodeTypeLabel(node.node_type)})`,
    nodeId: node.node_id,
    data: {
      node_type: node.node_type,
      zone: node.zone,
      discovered_via: node.discovered_via || data.discovered_via || 'unknown'
    }
  })
  
  // Воспроизводим звук (опционально)
  playDiscoverySound()
}

const playDiscoverySound = () => {
  try {
    const audio = new Audio('/discovery-sound.mp3')
    audio.volume = 0.3
    audio.play().catch(() => {
      // Ignore if sound fails
    })
  } catch (e) {
    // Ignore sound errors
  }
}

// Экспортируем метод для ручного вызова (для совместимости)
defineExpose({
  showDiscovery: handleNodeDiscovered
})
</script>


