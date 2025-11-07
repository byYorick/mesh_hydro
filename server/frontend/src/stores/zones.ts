/**
 * ⭐ ЗОНИРОВАНИЕ: Pinia Store для управления зонами
 */
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import axios from 'axios'

export interface Zone {
  id: number
  name: string
  description?: string
  location?: string
  root_node_id: string
  mesh_network_id: string
  mqtt_topic_prefix: string
  zone_type: 'nft' | 'dwc' | 'ebb_flow' | 'drip' | 'aeroponics' | 'other'
  reservoir_volume_liters?: number
  growing_area_m2?: number
  plant_capacity?: number
  is_active: boolean
  is_available: boolean
  current_cycle_id?: number
  image_url?: string
  notes?: string
  created_at: string
  updated_at: string
  
  // Relations
  root_node?: any
  current_cycle?: any
  node_assignments?: any[]
}

function normalizeZonePayload(payload: any): Zone[] {
  const convert = (value: any): Zone[] => {
    if (Array.isArray(value)) {
      return value as Zone[]
    }

    if (value?.data && Array.isArray(value.data)) {
      return value.data as Zone[]
    }

    if (value?.zones && Array.isArray(value.zones)) {
      return value.zones as Zone[]
    }

    if (value?.items && Array.isArray(value.items)) {
      return value.items as Zone[]
    }

    return []
  }

  let normalized: Zone[] = []

  if (typeof payload === 'string') {
    try {
      const parsed = JSON.parse(payload)
      normalized = convert(parsed)
      if (normalized.length === 0 && Array.isArray(parsed)) {
        normalized = parsed as Zone[]
      }
    } catch (err) {
      console.warn('zones.ts: failed to parse zones payload string', err)
      normalized = []
    }
  } else {
    normalized = convert(payload)
  }

  if (normalized.length === 0 && !Array.isArray(payload)) {
    console.warn('zones.ts: unexpected zones payload shape, falling back to empty list', payload)
  }

  return normalized
}

export const useZonesStore = defineStore('zones', () => {
  // State
  const zones = ref<Zone[]>([])
  const selectedZone = ref<Zone | null>(null)
  const loading = ref(false)
  const error = ref<string | null>(null)

  // Computed
  const activeZones = computed(() => 
    zones.value.filter(z => z.is_active)
  )

  const availableZones = computed(() => 
    zones.value.filter(z => z.is_active && z.is_available && !z.current_cycle_id)
  )

  const busyZones = computed(() =>
    zones.value.filter(z => z.is_active && z.current_cycle_id)
  )

  const zonesByType = computed(() => {
    const byType: Record<string, Zone[]> = {}
    zones.value.forEach(zone => {
      if (!byType[zone.zone_type]) {
        byType[zone.zone_type] = []
      }
      byType[zone.zone_type].push(zone)
    })
    return byType
  })

  // Actions
  async function fetchZones() {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.get('/api/zones')
      zones.value = normalizeZonePayload(response?.data)
    } catch (err: any) {
      zones.value = []
      error.value = err.response?.data?.message || 'Failed to fetch zones'
      console.error('Error fetching zones:', err)
    } finally {
      loading.value = false
    }
  }

  async function fetchZone(id: number) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.get(`/api/zones/${id}`)
      selectedZone.value = response.data
      
      // Обновить в списке, если есть
      const index = zones.value.findIndex(z => z.id === id)
      if (index !== -1) {
        zones.value[index] = response.data
      }
      
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch zone'
      console.error('Error fetching zone:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function createZone(zoneData: Partial<Zone>) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.post('/api/zones', zoneData)
      zones.value.push(response.data)
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to create zone'
      console.error('Error creating zone:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function updateZone(id: number, zoneData: Partial<Zone>) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.put(`/api/zones/${id}`, zoneData)
      
      const index = zones.value.findIndex(z => z.id === id)
      if (index !== -1) {
        zones.value[index] = response.data
      }
      
      if (selectedZone.value?.id === id) {
        selectedZone.value = response.data
      }
      
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to update zone'
      console.error('Error updating zone:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function deleteZone(id: number) {
    loading.value = true
    error.value = null
    
    try {
      await axios.delete(`/api/zones/${id}`)
      
      const index = zones.value.findIndex(z => z.id === id)
      if (index !== -1) {
        zones.value.splice(index, 1)
      }
      
      if (selectedZone.value?.id === id) {
        selectedZone.value = null
      }
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to delete zone'
      console.error('Error deleting zone:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function fetchZoneNodes(id: number) {
    try {
      const response = await axios.get(`/api/zones/${id}/nodes`)
      return response.data.nodes
    } catch (err: any) {
      console.error('Error fetching zone nodes:', err)
      throw err
    }
  }

  async function fetchZoneRootNode(id: number) {
    try {
      const response = await axios.get(`/api/zones/${id}/root-node`)
      return response.data
    } catch (err: any) {
      console.error('Error fetching zone root node:', err)
      throw err
    }
  }

  async function sendZoneCommand(id: number, nodeId: string, command: string, params?: any) {
    try {
      const response = await axios.post(`/api/zones/${id}/command`, {
        node_id: nodeId,
        command,
        params
      })
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to send command'
      console.error('Error sending zone command:', err)
      throw err
    }
  }

  async function checkNodesAvailability(nodeIds: string[], excludeZoneId?: number) {
    try {
      const response = await axios.post('/api/zones/check-nodes-availability', {
        node_ids: nodeIds,
        zone_id: excludeZoneId
      })
      return response.data
    } catch (err: any) {
      console.error('Error checking nodes availability:', err)
      throw err
    }
  }

  function getZoneById(id: number): Zone | undefined {
    return zones.value.find(z => z.id === id)
  }

  function getZoneByRootNode(rootNodeId: string): Zone | undefined {
    return zones.value.find(z => z.root_node_id === rootNodeId)
  }

  function getZoneNodes(zoneId: number): any[] {
    const zone = zones.value.find(z => z.id === zoneId)
    if (!zone) return []
    
    // Получаем узлы из node_assignments или через root_node
    if (zone.node_assignments && zone.node_assignments.length > 0) {
      return zone.node_assignments.map(assignment => assignment.node)
    }
    
    // Если нет assignments, возвращаем только root_node
    if (zone.root_node) {
      return [zone.root_node]
    }
    
    return []
  }

  function clearError() {
    error.value = null
  }

  return {
    // State
    zones,
    selectedZone,
    loading,
    error,
    
    // Computed
    activeZones,
    availableZones,
    busyZones,
    zonesByType,
    
    // Actions
    fetchZones,
    fetchZone,
    createZone,
    updateZone,
    deleteZone,
    fetchZoneNodes,
    fetchZoneRootNode,
    sendZoneCommand,
    checkNodesAvailability,
    getZoneById,
    getZoneByRootNode,
    getZoneNodes,
    clearError,
  }
})

