import { defineStore } from 'pinia'
import { ref, computed, watch } from 'vue'
import api from '@/services/api'
import type {
  NewNode,
  ConfigureNewNodePayload,
  SetupHistoryEntry,
} from '@/types/newNode'
import { useGreenhousesStore } from '@/stores/greenhouses'

const HISTORY_LIMIT = 30

export const useSetupStore = defineStore('setup', () => {
  const greenhousesStore = useGreenhousesStore()

  // State
  const newNodes = ref<NewNode[]>([])
  const loading = ref(false)
  const error = ref<string | null>(null)
  const realtimeConnected = ref(false)
  const lastUpdatedAt = ref<string | null>(null)
  const history = ref<SetupHistoryEntry[]>([])
  const configuringMac = ref<string | null>(null)
  const targetGreenhouseId = ref<number | null>(greenhousesStore.selectedId ?? null)

  watch(
    () => greenhousesStore.selectedId,
    (value) => {
      if (targetGreenhouseId.value == null) {
        targetGreenhouseId.value = value ?? null
      }
    },
    { immediate: true },
  )

  // Getters
  const hasRootPending = computed(() => newNodes.value.some(node => node.is_root))

  const rootNode = computed(() =>
    newNodes.value.find(node => node.is_root)
  )

  const regularNodes = computed(() =>
    newNodes.value.filter(node => !node.is_root)
  )

  const onlineCount = computed(() =>
    newNodes.value.filter(node => node.is_online).length
  )

  const totalCount = computed(() => newNodes.value.length)

  const isLoading = computed(() => loading.value)

  // Helpers
  const upsertNode = (node: NewNode) => {
    const index = newNodes.value.findIndex(item => item.mac_address === node.mac_address)
    if (index >= 0) {
      newNodes.value[index] = {
        ...newNodes.value[index],
        ...node,
      }
    } else {
      newNodes.value.unshift(node)
    }
  }

  const appendHistory = (entry: SetupHistoryEntry) => {
    history.value.unshift(entry)
    if (history.value.length > HISTORY_LIMIT) {
      history.value.splice(HISTORY_LIMIT)
    }
  }

  const removeNode = (macAddress: string) => {
    newNodes.value = newNodes.value.filter(node => node.mac_address !== macAddress)
  }

  const markNodeStatus = (macAddress: string, status: NewNode['status']) => {
    const node = newNodes.value.find(item => item.mac_address === macAddress)
    if (node) {
      node.status = status
    }
  }

  // Actions
  const fetchNewNodes = async () => {
    loading.value = true
    error.value = null
    try {
      const response = await api.getNewNodes()
      newNodes.value = Array.isArray(response) ? response : []
      lastUpdatedAt.value = new Date().toISOString()
    } catch (err: any) {
      console.error('Failed to fetch new nodes', err)
      error.value = err?.response?.data?.message || err?.message || 'Ошибка загрузки новых узлов'
      newNodes.value = []
    } finally {
      loading.value = false
    }
  }

  const loadNode = async (macAddress: string) => {
    try {
      const node = await api.getNewNode(macAddress)
      if (node) {
        upsertNode(node)
      }
    } catch (err: any) {
      console.warn(`Не удалось загрузить узел ${macAddress}`, err)
    }
  }

  const configureNode = async (macAddress: string, payload: ConfigureNewNodePayload) => {
    configuringMac.value = macAddress
    try {
      const payloadWithGreenhouse: ConfigureNewNodePayload & { greenhouse_id?: number | null } = {
        ...payload,
      }

      if (payloadWithGreenhouse.greenhouse_id == null && targetGreenhouseId.value != null) {
        payloadWithGreenhouse.greenhouse_id = targetGreenhouseId.value
      }

      const result = await api.configureNewNode(macAddress, payloadWithGreenhouse)
      markNodeStatus(macAddress, 'configuring')
      appendHistory({
        timestamp: new Date().toISOString(),
        mac_address: macAddress,
        node_id: (payload as any).node_id,
        zone: (payload as any).zone,
        action: 'configured',
        message: 'Конфигурация отправлена на устройство',
      })
      if (payloadWithGreenhouse.greenhouse_id) {
        greenhousesStore.fetchGreenhouse(payloadWithGreenhouse.greenhouse_id, { refresh: true }).catch(
          (err: any) => {
            console.warn('Не удалось обновить данные теплицы после конфигурации узла', err)
          },
        )
      }
      return result
    } catch (err: any) {
      console.error('Ошибка отправки конфигурации', err)
      throw err
    } finally {
      configuringMac.value = null
    }
  }

  const deleteNewNode = async (macAddress: string) => {
    try {
      await api.deleteNewNode(macAddress)
      removeNode(macAddress)
      appendHistory({
        timestamp: new Date().toISOString(),
        mac_address: macAddress,
        action: 'removed',
        message: 'Узел удалён из списка новых',
      })
    } catch (err: any) {
      console.error('Не удалось удалить новый узел', err)
      throw err
    }
  }

  // Realtime handlers
  const handleDiscovered = async (payload: any) => {
    realtimeConnected.value = true
    const macAddress = payload?.mac_address
    if (!macAddress) {
      return
    }

    // Try to use payload data first
    const eventNode: NewNode = {
      mac_address: macAddress,
      node_type: payload?.node_type || 'unknown',
      is_root: Boolean(payload?.is_root),
      pin: payload?.pin || '',
      temp_mesh_id: payload?.temp_mesh_id,
      status: payload?.status || 'discovered',
      metadata: payload?.metadata || {},
      discovered_at: payload?.discovered_at || new Date().toISOString(),
      last_heartbeat_at: payload?.last_heartbeat_at || null,
      is_online: payload?.is_online ?? true,
    }

    upsertNode(eventNode)
    appendHistory({
      timestamp: new Date().toISOString(),
      mac_address: macAddress,
      node_type: eventNode.node_type,
      action: 'discovered',
      message: 'Обнаружено новое устройство',
    })

    // Fetch full node details asynchronously
    await loadNode(macAddress)
  }

  const handleUpdated = (payload: any) => {
    realtimeConnected.value = true
    const macAddress = payload?.mac_address
    if (!macAddress) {
      return
    }

    const node = newNodes.value.find(item => item.mac_address === macAddress)
    if (!node) {
      return
    }

    node.status = payload?.status || node.status
    node.last_heartbeat_at = payload?.last_heartbeat_at || node.last_heartbeat_at
    node.is_online = payload?.is_online ?? node.is_online
    node.metadata = {
      ...(node.metadata || {}),
      ...(payload?.metadata || {}),
    }
  }

  const handleConfigured = (payload: any) => {
    realtimeConnected.value = true
    const macAddress = payload?.mac_address
    if (!macAddress) {
      return
    }

    const node = newNodes.value.find(item => item.mac_address === macAddress)
    removeNode(macAddress)
    appendHistory({
      timestamp: new Date().toISOString(),
      mac_address: macAddress,
      node_id: payload?.node_id || node?.metadata?.requested_node_id,
      node_type: node?.node_type,
      action: 'configured',
      message: `Узел ${payload?.node_id || node?.node_type || ''} успешно настроен`,
    })
    const greenhouseId =
      payload?.greenhouse_id ||
      node?.metadata?.greenhouse_id ||
      node?.metadata?.requested_greenhouse_id ||
      null
    if (greenhouseId) {
      greenhousesStore.fetchGreenhouse(greenhouseId, { refresh: true }).catch((err: any) => {
        console.warn('Не удалось обновить данные теплицы после события configured', err)
      })
    }
  }

  const setRealtimeConnection = (status: boolean) => {
    realtimeConnected.value = status
  }

  return {
    // state
    newNodes,
    loading,
    error,
    realtimeConnected,
    lastUpdatedAt,
    history,
    configuringMac,
    targetGreenhouseId,

    // getters
    hasRootPending,
    rootNode,
    regularNodes,
    onlineCount,
    totalCount,
    isLoading,

    // actions
    fetchNewNodes,
    loadNode,
    configureNode,
    deleteNewNode,
    handleDiscovered,
    handleUpdated,
    handleConfigured,
    setRealtimeConnection,
    setTargetGreenhouseId: (id: number | null) => {
      targetGreenhouseId.value = id
    },
  }
})


