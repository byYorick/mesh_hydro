<template>
  <div class="root-detail-wrapper">
    <!-- Mesh Network Topology -->
    <v-card class="mb-4">
      <v-card-title class="d-flex align-center">
        <v-icon icon="mdi-network" class="mr-2"></v-icon>
        Топология Mesh-сети
        <v-spacer></v-spacer>
        <v-chip :color="meshStatusColor" size="small">
          {{ meshStatusText }}
        </v-chip>
      </v-card-title>

      <v-card-text>
        <!-- Tree Visualization -->
        <div class="mesh-topology-tree">
          <!-- Root Node -->
          <div class="mesh-node root-node">
            <v-avatar :color="node.online ? 'primary' : 'grey'" size="64">
              <v-icon icon="mdi-server-network" size="32"></v-icon>
            </v-avatar>
            <div class="node-label">
              <div class="font-weight-bold">{{ node.node_id }}</div>
              <div class="text-caption text-medium-emphasis">ROOT</div>
            </div>
            <v-chip v-if="meshNodesCount > 0" color="success" size="small" class="mt-2">
              {{ meshNodesCount }} {{ declension(meshNodesCount, ['узел', 'узла', 'узлов']) }}
            </v-chip>
          </div>

          <!-- Connected Nodes -->
          <div v-if="connectedNodes.length > 0" class="mesh-children">
            <div v-for="child in connectedNodes" :key="child.node_id" class="mesh-node child-node">
              <v-avatar :color="getNodeTypeColor(child.node_type)" size="48">
                <v-icon :icon="getNodeTypeIcon(child.node_type)" size="24"></v-icon>
              </v-avatar>
              <div class="node-label">
                <div class="font-weight-bold">{{ child.node_id }}</div>
                <div class="text-caption text-medium-emphasis">{{ getNodeTypeName(child.node_type) }}</div>
              </div>
              <v-chip 
                v-if="child.rssi" 
                :color="getRssiColor(child.rssi)" 
                size="small" 
                class="mt-2"
                :title="`RSSI: ${child.rssi} dBm`"
              >
                <v-icon :icon="getRssiIcon(child.rssi)" size="x-small" class="mr-1"></v-icon>
                {{ child.rssi }} dBm
              </v-chip>
            </div>
          </div>

          <!-- No Connected Nodes -->
          <div v-else class="text-center pa-8">
            <v-icon icon="mdi-lan-disconnect" size="64" color="grey"></v-icon>
            <div class="text-h6 mt-4 text-medium-emphasis">Нет подключенных узлов</div>
            <div class="text-caption text-disabled mt-2">
              Ожидание подключения mesh-узлов...
            </div>
          </div>
        </div>
      </v-card-text>
    </v-card>

    <!-- Mesh Network Stats -->
    <v-row>
      <v-col cols="12" md="4">
        <v-card>
          <v-card-title class="text-caption text-medium-emphasis">
            Статус Mesh
          </v-card-title>
          <v-card-text>
            <div class="text-h4 font-weight-bold mb-2">
              {{ connectedNodes.length }}
            </div>
            <div class="text-caption">Подключенных узлов</div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="4">
        <v-card>
          <v-card-title class="text-caption text-medium-emphasis">
            WiFi Подключение
          </v-card-title>
          <v-card-text>
            <div class="d-flex align-center mb-2">
              <v-icon :icon="getRssiIcon(rootRssi)" :color="getRssiColor(rootRssi)" size="32"></v-icon>
              <div class="ml-2">
                <div class="text-h6 font-weight-bold">{{ rootRssi || '-' }} dBm</div>
                <div class="text-caption">
                  <template v-if="rootRssi">
                    {{ wifiSsid || 'Подключено' }}
                  </template>
                  <template v-else>
                    Не подключено
                  </template>
                </div>
              </div>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="4">
        <v-card>
          <v-card-title class="text-caption text-medium-emphasis">
            Связь с сервером
          </v-card-title>
          <v-card-text>
            <v-chip :color="node.online ? 'success' : 'error'" size="small" class="mb-2">
              {{ node.online ? 'Connected' : 'Disconnected' }}
            </v-chip>
            <div class="text-caption">
              {{ formatLastSeen(node.last_seen_at) }}
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Connected Nodes List -->
    <v-card v-if="connectedNodes.length > 0">
      <v-card-title>
        <v-icon icon="mdi-format-list-bulleted" class="mr-2"></v-icon>
        Список подключенных узлов
      </v-card-title>
      <v-card-text>
        <v-data-table
          :headers="tableHeaders"
          :items="connectedNodes"
          :items-per-page="10"
        >
          <template v-slot:item.node_type="{ item }">
            <v-chip :color="getNodeTypeColor(item.node_type)" size="small">
              <v-icon :icon="getNodeTypeIcon(item.node_type)" start size="small"></v-icon>
              {{ getNodeTypeName(item.node_type) }}
            </v-chip>
          </template>
          <template v-slot:item.online="{ item }">
            <v-chip :color="item.online ? 'success' : 'error'" size="small">
              {{ item.online ? 'Online' : 'Offline' }}
            </v-chip>
          </template>
          <template v-slot:item.rssi="{ item }">
            <div v-if="item.rssi" class="d-flex align-center">
              <v-icon :icon="getRssiIcon(item.rssi)" :color="getRssiColor(item.rssi)" size="small" class="mr-1"></v-icon>
              {{ item.rssi }} dBm
            </div>
            <span v-else class="text-disabled">-</span>
          </template>
          <template v-slot:item.last_seen_at="{ item }">
            {{ formatLastSeen(item.last_seen_at) }}
          </template>
          <template v-slot:item.actions="{ item }">
            <v-btn
              icon
              size="small"
              variant="text"
              :to="{ name: 'NodeDetail', params: { nodeId: item.node_id } }"
            >
              <v-icon icon="mdi-open-in-new" size="small"></v-icon>
              <v-tooltip activator="parent">Открыть детали</v-tooltip>
            </v-btn>
          </template>
        </v-data-table>
      </v-card-text>
    </v-card>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { useNodesStore } from '@/stores/nodes'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const nodesStore = useNodesStore()

// Mesh network status
const meshNodesCount = computed(() => {
  return props.node.metadata?.mesh_nodes || 0
})

const connectedNodes = computed(() => {
  // Get all online nodes except root
  return nodesStore.nodes
    .filter(n => n.node_type !== 'root' && n.node_id !== props.node.node_id)
    .map(node => ({
      ...node,
      rssi: node.metadata?.rssi_to_parent || node.metadata?.wifi_rssi || null,
    }))
})

const meshStatusColor = computed(() => {
  if (!props.node.online) return 'error'
  if (connectedNodes.value.length === 0) return 'warning'
  return 'success'
})

const meshStatusText = computed(() => {
  if (!props.node.online) return 'Root Offline'
  if (connectedNodes.value.length === 0) return 'Нет узлов'
  return `${connectedNodes.value.length} узлов`
})

const rootRssi = computed(() => {
  return props.node.metadata?.wifi_rssi || props.node.metadata?.rssi_to_parent || null
})

const wifiSsid = computed(() => {
  return props.node.metadata?.wifi_ssid || null
})

// Table headers
const tableHeaders = [
  { title: 'ID узла', key: 'node_id', sortable: true },
  { title: 'Тип', key: 'node_type', sortable: true },
  { title: 'Статус', key: 'online', sortable: true },
  { title: 'RSSI', key: 'rssi', sortable: true },
  { title: 'Последний раз видели', key: 'last_seen_at', sortable: true },
  { title: 'Действия', key: 'actions', sortable: false },
]

// Helper functions
function getNodeTypeColor(type) {
  const colors = {
    'ph_ec': 'primary',
    'ph': 'purple',
    'ec': 'orange',
    'climate': 'green',
    'relay': 'blue',
    'water': 'cyan',
    'display': 'indigo',
    'root': 'deep-orange',
  }
  return colors[type] || 'grey'
}

function getNodeTypeIcon(type) {
  const icons = {
    'ph_ec': 'mdi-flask',
    'ph': 'mdi-flask-outline',
    'ec': 'mdi-flash',
    'climate': 'mdi-thermometer',
    'relay': 'mdi-electric-switch',
    'water': 'mdi-water',
    'display': 'mdi-monitor',
    'root': 'mdi-server-network',
  }
  return icons[type] || 'mdi-chip'
}

function getNodeTypeName(type) {
  const names = {
    'ph_ec': 'pH/EC',
    'ph': 'pH',
    'ec': 'EC',
    'climate': 'Климат',
    'relay': 'Реле',
    'water': 'Вода',
    'display': 'Дисплей',
    'root': 'Root',
  }
  return names[type] || type
}

function getRssiColor(rssi) {
  if (!rssi) return 'grey'
  if (rssi > -50) return 'success'
  if (rssi > -70) return 'warning'
  return 'error'
}

function getRssiIcon(rssi) {
  if (!rssi) return 'mdi-wifi-off'
  if (rssi > -50) return 'mdi-wifi-strength-4'
  if (rssi > -60) return 'mdi-wifi-strength-3'
  if (rssi > -70) return 'mdi-wifi-strength-2'
  return 'mdi-wifi-strength-1'
}

function formatLastSeen(timestamp) {
  if (!timestamp) return 'Никогда'
  const now = new Date()
  const lastSeen = new Date(timestamp)
  const diffMs = now - lastSeen
  const diffSec = Math.floor(diffMs / 1000)
  
  if (diffSec < 60) return 'Только что'
  if (diffSec < 3600) return `${Math.floor(diffSec / 60)} мин назад`
  if (diffSec < 86400) return `${Math.floor(diffSec / 3600)} ч назад`
  return `${Math.floor(diffSec / 86400)} дн назад`
}

function declension(num, forms) {
  const cases = [2, 0, 1, 1, 1, 2]
  return forms[(num % 100 > 4 && num % 100 < 20) ? 2 : cases[Math.min(num % 10, 5)]]
}
</script>

<style scoped>
.root-detail-wrapper {
  width: 100%;
}

.mesh-topology-tree {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 24px 0;
}

.mesh-node {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 16px;
  position: relative;
}

.root-node {
  margin-bottom: 24px;
}

.mesh-children {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
  justify-content: center;
  margin-top: 16px;
}

.child-node {
  flex: 0 0 auto;
}

.node-label {
  text-align: center;
  margin-top: 8px;
}

@media (max-width: 768px) {
  .mesh-children {
    flex-direction: column;
    align-items: center;
  }
  
  .child-node {
    width: 100%;
  }
}
</style>
