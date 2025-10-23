<template>
  <div>
    <v-row>
      <v-col cols="12" md="6">
        <h1 class="text-h3 mb-4">Узлы системы</h1>
      </v-col>
      <v-col cols="12" md="6" class="text-right">
        <AddNodeDialog @node-created="handleNodeCreated">
          <template v-slot:activator="{ props }">
            <v-btn
              color="primary"
              size="large"
              v-bind="props"
              prepend-icon="mdi-plus-circle"
            >
              Добавить узел
            </v-btn>
          </template>
        </AddNodeDialog>
      </v-col>
    </v-row>

    <!-- Filters -->
    <v-row>
      <v-col cols="12" md="6">
        <v-text-field
          v-model="search"
          label="Поиск узлов"
          prepend-inner-icon="mdi-magnify"
          clearable
          variant="outlined"
          density="compact"
        ></v-text-field>
      </v-col>

      <v-col cols="12" md="3">
        <v-select
          v-model="filterType"
          :items="nodeTypes"
          label="Тип узла"
          clearable
          variant="outlined"
          density="compact"
        ></v-select>
      </v-col>

      <v-col cols="12" md="3">
        <v-select
          v-model="filterStatus"
          :items="['online', 'offline']"
          label="Статус"
          clearable
          variant="outlined"
          density="compact"
        ></v-select>
      </v-col>
    </v-row>

    <!-- Loading skeleton -->
    <div v-if="nodesStore.loading" class="v-row">
      <v-col
        v-for="n in 8"
        :key="`skeleton-${n}`"
        cols="12"
        sm="6"
        md="4"
        lg="3"
      >
        <SkeletonCard />
      </v-col>
    </div>

    <!-- Nodes Grid -->
    <v-row v-else>
      <v-col
        v-for="node in filteredNodes"
        :key="node.node_id"
        cols="12"
        sm="6"
        md="4"
        lg="3"
      >
        <NodeCard
          :node="node"
          @command="sendCommand(node.node_id, $event)"
        />
      </v-col>

      <v-col v-if="filteredNodes.length === 0" cols="12">
        <v-card>
          <v-card-text class="text-center pa-8">
            <v-icon icon="mdi-cloud-search" size="64" color="grey"></v-icon>
            <div class="mt-4 text-h6 text-disabled">
              {{ search ? 'Узлы не найдены' : 'Нет доступных узлов' }}
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useAppStore } from '@/stores/app'
import { useNodesStore } from '@/stores/nodes'
import { useDebounce } from '@/composables/useDebounce'
import NodeCard from '@/components/NodeCard.vue'
import AddNodeDialog from '@/components/AddNodeDialog.vue'
import SkeletonCard from '@/components/ui/SkeletonCard.vue'
import api from '@/services/api'

const appStore = useAppStore()
const nodesStore = useNodesStore()

const search = ref('')
const filterType = ref(null)
const filterStatus = ref(null)

// Debounced search для улучшения производительности
const debouncedSearch = useDebounce(search, 300)

const nodeTypes = ['ph', 'ec', 'ph_ec', 'climate', 'relay', 'water', 'display', 'root']

// Filtered nodes
const filteredNodes = computed(() => {
  let nodes = [...nodesStore.nodes]

  // Search filter с debounce
  if (debouncedSearch.value) {
    const query = debouncedSearch.value.toLowerCase()
    nodes = nodes.filter(node =>
      node.node_id.toLowerCase().includes(query) ||
      node.node_type.toLowerCase().includes(query) ||
      node.zone?.toLowerCase().includes(query)
    )
  }

  // Type filter
  if (filterType.value) {
    nodes = nodes.filter(node => node.node_type === filterType.value)
  }

  // Status filter
  if (filterStatus.value) {
    if (filterStatus.value === 'online') {
      nodes = nodes.filter(node => node.online || node.is_online)
    } else {
      nodes = nodes.filter(node => !node.online && !node.is_online)
    }
  }

  return nodes
})

onMounted(async () => {
  await nodesStore.fetchNodes()
})

// Send command
async function sendCommand(nodeId, { command, params }) {
  try {
    // Специальная обработка для команды run_pump
    if (command === 'run_pump') {
      // Определяем pump_id на основе типа насоса
      const pumpIdMap = {
        'ph_up': 0,
        'ph_down': 1,
        'ec_up': 2,
        'ec_down': 3,
        'water': 4
      }
      
      const pumpId = pumpIdMap[params.pump] || 0
      
      // Отправляем запрос на запуск насоса
      await api.post(`/nodes/${nodeId}/pump/run`, {
        pump_id: pumpId,
        duration_sec: params.duration
      })
      
      appStore.showSnackbar(`Насос ${params.pump} запущен на ${params.duration} сек`, 'success')
    } else {
      // Обычные команды через стандартный API
      await nodesStore.sendCommand(nodeId, command, params)
      appStore.showSnackbar(`Команда "${command}" отправлена`, 'success')
    }
  } catch (error) {
    console.error('Error sending command:', error)
    appStore.showSnackbar('Ошибка отправки команды', 'error')
  }
}

// Handle node creation
async function handleNodeCreated(nodeData) {
  try {
    const createdNode = await api.createNode(nodeData)
    
    // Add to store
    await nodesStore.fetchNodes()
    
    appStore.showSnackbar(
      `Узел "${nodeData.node_id}" успешно создан!`,
      'success',
      5000
    )
  } catch (error) {
    appStore.showSnackbar(
      'Ошибка создания узла: ' + (error.response?.data?.message || error.message),
      'error'
    )
  }
}
</script>

