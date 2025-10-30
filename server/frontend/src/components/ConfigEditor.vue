<template>
  <v-dialog v-model="dialog" max-width="800" scrollable>
    <template v-slot:activator="{ props }">
      <slot name="activator" :props="props"></slot>
    </template>

    <v-card>
      <v-card-title class="bg-secondary">
        <v-icon icon="mdi-cog" class="mr-2"></v-icon>
        Конфигурация: {{ node?.node_id }}
      </v-card-title>

      <v-card-text class="pt-4">
        <NodeConfigManager 
          :node="node" 
          @config-updated="handleConfigUpdate"
          @close="dialog = false"
        />
      </v-card-text>
    </v-card>
  </v-dialog>
</template>

<script setup>
import { ref, watch } from 'vue'
import NodeConfigManager from './node-config/NodeConfigManager.vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['config-updated'])

const dialog = ref(false)

function handleConfigUpdate(config) {
  emit('config-updated', config)
  dialog.value = false
}
</script>