<template>
  <v-container fluid class="setup-page">
    <v-row>
      <v-col cols="12">
        <v-card variant="outlined" class="pa-4">
          <div class="d-flex flex-column flex-sm-row align-sm-center justify-space-between ga-4">
            <div>
              <div class="text-h5 font-weight-medium d-flex align-center ga-2">
                <v-icon color="primary" icon="mdi-access-point-plus"></v-icon>
                Настройка узлов
              </div>
              <div class="text-caption text-medium-emphasis mt-1">
                Последнее обновление: {{ lastUpdatedLabel }}
              </div>
            </div>

            <div class="d-flex align-center ga-2 flex-wrap">
              <v-select
                v-model="selectedGreenhouseId"
                :items="greenhouseOptions"
                item-title="label"
                item-value="value"
                label="Целевая теплица"
                variant="outlined"
                density="compact"
                style="min-width: 220px; max-width: 280px"
                clearable
                :loading="greenhousesStore.loading"
              ></v-select>

              <v-chip
                :color="setupStore.realtimeConnected ? 'success' : 'warning'"
                variant="tonal"
                prepend-icon="mdi-wifi"
              >
                {{ setupStore.realtimeConnected ? 'WebSocket: online' : 'WebSocket: offline' }}
              </v-chip>

              <v-chip color="info" variant="tonal" prepend-icon="mdi-radar">
                Новых: {{ setupStore.totalCount }}
              </v-chip>

              <v-chip color="success" variant="tonal" prepend-icon="mdi-heart-pulse">
                Онлайн: {{ setupStore.onlineCount }}
              </v-chip>

              <v-btn
                color="primary"
                variant="tonal"
                prepend-icon="mdi-refresh"
                :loading="setupStore.loading"
                @click="refreshNewNodes"
              >
                Обновить
              </v-btn>
            </div>
          </div>
        </v-card>
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12" lg="8">
        <v-card variant="outlined" class="pa-4">
          <div class="d-flex align-center justify-space-between mb-4">
            <div class="text-h6">🆕 Новые узлы</div>
            <v-chip v-if="setupStore.totalCount > 0" color="primary" variant="flat" size="small">
              {{ setupStore.totalCount }}
            </v-chip>
          </div>

          <template v-if="setupStore.totalCount === 0 && !setupStore.loading">
            <v-alert type="info" variant="tonal" border="start" class="mb-0">
              На данный момент новых узлов не обнаружено.
              Подключите устройство к питанию — оно появится здесь автоматически.
            </v-alert>
          </template>

          <template v-else>
            <v-row dense>
              <v-col cols="12" v-if="rootPendingNode">
                <NewNodeCard
                  :node="rootPendingNode"
                  :loading="isConfiguring(rootPendingNode)"
                  :disabled="setupStore.loading"
                  @configure="openWizard"
                  @remove="removeNode"
                />
              </v-col>

              <v-col
                v-for="node in regularPendingNodes"
                :key="node.mac_address"
                cols="12"
                md="6"
              >
                <NewNodeCard
                  :node="node"
                  :loading="isConfiguring(node)"
                  :disabled="setupStore.loading || removingMac === node.mac_address"
                  @configure="openWizard"
                  @remove="removeNode"
                />
              </v-col>
            </v-row>
          </template>
        </v-card>
      </v-col>

      <v-col cols="12" lg="4">
        <v-card variant="outlined" class="pa-4">
          <div class="text-h6 mb-3 d-flex align-center ga-2">
            <v-icon icon="mdi-history"></v-icon>
            История
          </div>

          <v-list v-if="historyItems.length" density="compact">
            <v-list-item
              v-for="entry in historyItems"
              :key="entry.timestamp + entry.mac_address + entry.action"
            >
              <template #prepend>
                <v-avatar
                  size="28"
                  :color="historyColor(entry.action)"
                >
                  <v-icon size="18">{{ historyIcon(entry.action) }}</v-icon>
                </v-avatar>
              </template>
              <v-list-item-title class="text-body-2">
                {{ historyMessage(entry) }}
              </v-list-item-title>
              <v-list-item-subtitle class="text-caption text-medium-emphasis">
                {{ formatDistance(entry.timestamp) }}
              </v-list-item-subtitle>
            </v-list-item>
          </v-list>

          <v-alert
            v-else
            type="info"
            variant="tonal"
            border="start"
            class="mb-0"
          >
            История появится после обнаружения или настройки устройств.
          </v-alert>
        </v-card>

        <v-card variant="outlined" class="pa-4 mt-4">
          <div class="text-h6 mb-3 d-flex align-center ga-2">
            <v-icon icon="mdi-check-circle"></v-icon>
            Недавно настроенные
          </div>

          <v-list v-if="configuredNodes.length" density="compact">
            <v-list-item
              v-for="node in configuredNodes"
              :key="node.node_id"
              :to="{ name: 'NodeDetail', params: { nodeId: node.node_id } }"
            >
              <template #prepend>
                <v-icon color="success" icon="mdi-checkbox-marked-circle"></v-icon>
              </template>
              <v-list-item-title>{{ node.node_id }}</v-list-item-title>
              <v-list-item-subtitle class="text-caption text-medium-emphasis">
                {{ node.node_type }} · {{ node.zone || 'Зона не указана' }}
              </v-list-item-subtitle>
            </v-list-item>
          </v-list>

          <v-alert
            v-else
            type="info"
            variant="tonal"
            border="start"
            class="mb-0"
          >
            После настройки устройства появятся здесь для быстрого перехода.
          </v-alert>
        </v-card>
      </v-col>
    </v-row>

    <SetupWizardDialog
      v-model="wizardOpen"
      :node="selectedNode"
      :zones="zones"
      :loading="isConfiguring(selectedNode || undefined)"
      @submit="configureNode"
      @cancel="closeWizard"
    />
  </v-container>
</template>

<script setup lang="ts">
import { computed, ref, onMounted } from 'vue'
import NewNodeCard from '@/components/setup/NewNodeCard.vue'
import SetupWizardDialog from '@/components/setup/SetupWizardDialog.vue'
import { useSetupStore } from '@/stores/setup'
import { useNodesStore } from '@/stores/nodes'
import { useZonesStore } from '@/stores/zones'
import { useGreenhousesStore } from '@/stores/greenhouses'
import type { NewNode } from '@/types/newNode'
import { usePopup } from '@/composables/usePopup'
import { formatDistanceToNow } from '@/utils/time'

const setupStore = useSetupStore()
const nodesStore = useNodesStore()
const zonesStore = useZonesStore()
const greenhousesStore = useGreenhousesStore()
const popup = usePopup()

const selectedNode = ref<NewNode | null>(null)
const wizardOpen = ref(false)
const removingMac = ref<string | null>(null)

const zones = computed(() => zonesStore.zones)
const regularPendingNodes = computed(() => setupStore.regularNodes)
const rootPendingNode = computed(() => setupStore.rootNode)
const historyItems = computed(() => setupStore.history)
const configuredNodes = computed(() => nodesStore.nodes.slice(0, 6))
const greenhouseOptions = computed(() => greenhousesStore.greenhouseOptions)

const selectedGreenhouseId = computed<number | null>({
  get: () => setupStore.targetGreenhouseId ?? null,
  set: (value) => {
    setupStore.setTargetGreenhouseId(value ?? null)
    greenhousesStore.selectGreenhouse(value ?? null)
  },
})

const lastUpdatedLabel = computed(() => {
  if (!setupStore.lastUpdatedAt) return 'нет данных'
  return formatDistanceToNow(setupStore.lastUpdatedAt)
})

const formatDistance = (date: string) => formatDistanceToNow(date)

const isConfiguring = (node?: NewNode) => {
  if (!node) return false
  return setupStore.configuringMac === node.mac_address
}

const historyIcon = (action: string) => {
  switch (action) {
    case 'configured':
      return 'mdi-check'
    case 'removed':
      return 'mdi-delete'
    case 'failed':
      return 'mdi-alert'
    default:
      return 'mdi-radar'
  }
}

const historyColor = (action: string) => {
  switch (action) {
    case 'configured':
      return 'success'
    case 'removed':
      return 'error'
    case 'failed':
      return 'warning'
    default:
      return 'info'
  }
}

const historyMessage = (entry: any) => {
  switch (entry.action) {
    case 'configured':
      return `Узел ${entry.node_id || entry.mac_address} настроен`
    case 'removed':
      return `Узел ${entry.mac_address} удалён из списка`
    case 'failed':
      return `Ошибка настройки ${entry.node_id || entry.mac_address}`
    default:
      return `Обнаружен узел ${entry.mac_address}`
  }
}

onMounted(async () => {
  try {
    if (!nodesStore.nodes.length) {
      await nodesStore.fetchNodes()
    }
  } catch (error) {
    console.warn('Не удалось загрузить список узлов', error)
  }

  try {
    if (!setupStore.newNodes.length) {
      await setupStore.fetchNewNodes()
    }
  } catch (error) {
    console.warn('Не удалось загрузить новые узлы', error)
  }

  if (!zonesStore.zones.length) {
    zonesStore.fetchZones().catch((error: any) => {
      console.warn('Не удалось загрузить зоны', error)
    })
  }

  if (!greenhousesStore.items.length) {
    greenhousesStore.fetchGreenhouses().catch((error: any) => {
      console.warn('Не удалось загрузить теплицы', error)
    })
  }
})

function openWizard(node: NewNode) {
  selectedNode.value = node
  wizardOpen.value = true
}

function closeWizard() {
  wizardOpen.value = false
  selectedNode.value = null
}

async function configureNode(payload: Record<string, any>) {
  if (!selectedNode.value) return

  try {
    await setupStore.configureNode(selectedNode.value.mac_address, payload)
    popup.toast.success('Конфигурация отправлена на устройство')
    wizardOpen.value = false
    selectedNode.value = null
  } catch (error: any) {
    console.error('Ошибка настройки узла', error)
    const message = error?.response?.data?.message || 'Не удалось отправить конфигурацию'
    popup.toast.error(message)
  }
}

async function removeNode(node: NewNode) {
  if (setupStore.configuringMac === node.mac_address) return

  const confirm = window.confirm(`Удалить устройство ${node.mac_address} из списка новых узлов?`)
  if (!confirm) return

  removingMac.value = node.mac_address
  try {
    await setupStore.deleteNewNode(node.mac_address)
    popup.toast.success('Узел удалён из списка новых устройств')
  } catch (error: any) {
    console.error('Не удалось удалить узел', error)
    const message = error?.response?.data?.message || 'Ошибка удаления узла'
    popup.toast.error(message)
  } finally {
    removingMac.value = null
  }
}

async function refreshNewNodes() {
  try {
    await setupStore.fetchNewNodes()
    popup.toast.info('Список новых узлов обновлён')
  } catch (error: any) {
    console.error('Ошибка обновления новых узлов', error)
    const message = error?.response?.data?.message || 'Не удалось обновить список'
    popup.toast.error(message)
  }
}
</script>

<style scoped>
.setup-page {
  min-height: 100%;
}

.ga-2 {
  gap: 8px;
}

.ga-4 {
  gap: 16px;
}
</style>


