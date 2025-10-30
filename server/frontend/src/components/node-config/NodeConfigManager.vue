<template>
  <div>
    <component
      :is="configComponent"
      :node="node"
      @close="$emit('close')"
      @config-updated="handleConfigUpdate"
    />
  </div>
</template>

<script setup>
import { computed } from 'vue'
import PhNodeConfig from './PhNodeConfig.vue'
import EcNodeConfig from './EcNodeConfig.vue'
import ClimateNodeConfig from './ClimateNodeConfig.vue'
import RelayNodeConfig from './RelayNodeConfig.vue'
import WaterNodeConfig from './WaterNodeConfig.vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['close', 'config-updated'])

const configComponent = computed(() => {
  switch (props.node.node_type) {
    case 'ph':
      return PhNodeConfig
    case 'ec':
      return EcNodeConfig
    case 'climate':
      return ClimateNodeConfig
    case 'relay':
      return RelayNodeConfig
    case 'water':
      return WaterNodeConfig
    default:
      // Fallback to a generic config component if needed
      return PhNodeConfig // or create a generic one
  }
})

function handleConfigUpdate(config) {
  emit('config-updated', config)
}
</script>