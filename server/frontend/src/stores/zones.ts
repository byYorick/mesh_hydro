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
  current_cycle_id?: number | null
  greenhouse_id?: number | null
  greenhouse_name?: string | null
  image_url?: string
  notes?: string
  created_at?: string
  updated_at?: string
  assigned_nodes?: Record<string, any>
  node_assignments?: any[]
  current_cycle?: any
  root_node?: any
  [key: string]: any
}

interface FetchOptions {
  refresh?: boolean
}

const STORAGE_KEY = 'mesh_hydro.selected_zone'

function loadStoredMeshId(): string | null {
  try {
    return localStorage.getItem(STORAGE_KEY)
  } catch (error) {
    console.warn('zones store: failed to read selected zone', error)
    return null
  }
}

function persistMeshId(meshId: string | null) {
  try {
    if (meshId) {
      localStorage.setItem(STORAGE_KEY, meshId)
    } else {
      localStorage.removeItem(STORAGE_KEY)
    }
  } catch (error) {
    console.warn('zones store: failed to persist selected zone', error)
  }
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
      console.warn('zones.ts: failed to parse zones payload', err)
      normalized = []
    }
  } else {
    normalized = convert(payload)
  }

  if (normalized.length === 0 && !Array.isArray(payload)) {
    console.warn('zones.ts: unexpected payload shape, returning empty list', payload)
  }

  return normalized
}

function extractZoneFromResponse(payload: any): Zone | null {
  if (!payload) {
    return null
  }

  if (payload?.data) {
    if (Array.isArray(payload.data)) {
      return payload.data[0] ?? null
    }

    return payload.data as Zone
  }

  return payload as Zone
}

export const useZonesStore = defineStore('zones', () => {
  const zones = ref<Zone[]>([])
const selectedZone = ref<string | null>(loadStoredMeshId() ?? null)
  const loading = ref(false)
  const error = ref<string | null>(null)

  const zoneNodesCache = ref<Record<number, any[]>>({})
  const zoneStatsCache = ref<Record<number, any>>({})

  const hasZones = computed(() => zones.value.length > 0)

  const zoneOptions = computed(() =>
    zones.value.map((zone) => ({
      value: zone.mesh_network_id || `zone_${zone.id}`,
      label: zone.name || zone.mesh_network_id || `Zone ${zone.id}`,
      nodes_total: Array.isArray(zone.node_assignments) ? zone.node_assignments.length : 0,
      nodes_online: undefined,
    })),
  )

  const selectedZoneSummary = computed(() => {
    if (!selectedZone.value) {
      return null
    }

    return zones.value.find((z) => z.mesh_network_id === selectedZone.value) ?? null
  })

  const activeZones = computed(() => zones.value.filter((z) => z.is_active))

  const availableZones = computed(() =>
    zones.value.filter((z) => z.is_active && z.is_available && !z.current_cycle_id),
  )

  const busyZones = computed(() =>
    zones.value.filter((z) => z.is_active && !!z.current_cycle_id),
  )

  const zonesByType = computed(() => {
    const byType: Record<string, Zone[]> = {}
    zones.value.forEach((zone) => {
      const type = zone.zone_type || 'other'
      if (!byType[type]) {
        byType[type] = []
      }
      byType[type].push(zone)
    })
    return byType
  })

  const zonesByGreenhouse = computed(() => {
    const byGreenhouse: Record<string, Zone[]> = {}
    zones.value.forEach((zone) => {
      const key = zone.greenhouse_id != null ? String(zone.greenhouse_id) : 'unassigned'
      if (!byGreenhouse[key]) {
        byGreenhouse[key] = []
      }
      byGreenhouse[key].push(zone)
    })
    return byGreenhouse
  })

  function reconcileSelection() {
    if (!zones.value.length) {
      selectZone(null)
      return
    }

    if (selectedZone.value) {
      const exists = zones.value.some((z) => z.mesh_network_id === selectedZone.value)
      if (exists) {
        return
      }
    }

    const firstZone = zones.value[0]
    if (firstZone?.mesh_network_id) {
      selectZone(firstZone.mesh_network_id)
    } else {
      selectZone(null)
    }
  }

  function selectZone(meshId: string | null) {
    if (selectedZone.value === meshId) {
      return
    }
    selectedZone.value = meshId
    persistMeshId(meshId)
  }

  function clearSelection() {
    selectZone(null)
  }

  function setZones(newZones: Zone[]) {
    zones.value = newZones
    reconcileSelection()
  }

  function setZoneNodes(zoneId: number, nodes: any[]) {
    zoneNodesCache.value = {
      ...zoneNodesCache.value,
      [zoneId]: nodes,
    }
  }

  function getZoneCacheKey(zoneIdentifier: number | string): number | null {
    if (typeof zoneIdentifier === 'number') {
      return zoneIdentifier
    }

    const found = zones.value.find((z) => z.mesh_network_id === zoneIdentifier)
    return found?.id ?? null
  }

  async function fetchZones(): Promise<Zone[]> {
    loading.value = true
    error.value = null

    try {
      const response = await axios.get('/api/zones')
      zones.value = normalizeZonePayload(response?.data ?? response)
      reconcileSelection()
      return zones.value
    } catch (err: any) {
      zones.value = []
      error.value = err?.response?.data?.message || err?.message || 'Failed to fetch zones'
      console.error('zones store: fetchZones failed', err)
      return []
    } finally {
      loading.value = false
    }
  }

  async function fetchZone(id: number): Promise<Zone | null> {
    try {
      const response = await axios.get(`/api/zones/${id}`)
      const zone = extractZoneFromResponse(response?.data ?? response)
      if (zone) {
        const index = zones.value.findIndex((z) => z.id === zone.id)
        if (index !== -1) {
          zones.value[index] = zone
        } else {
          zones.value.push(zone)
        }
        reconcileSelection()
      }
      return zone
    } catch (err) {
      console.error('zones store: fetchZone failed', err)
      throw err
    }
  }

  async function fetchZoneNodes(zoneIdentifier: number | string, options: FetchOptions = {}) {
    const cacheKey = getZoneCacheKey(zoneIdentifier)
    if (!options.refresh && cacheKey && zoneNodesCache.value[cacheKey]) {
      return zoneNodesCache.value[cacheKey]
    }

    try {
      const response = await axios.get(`/api/zones/${zoneIdentifier}/nodes`)
      const nodes = response?.data?.data?.child_nodes
        ?? response?.data?.nodes
        ?? response?.nodes
        ?? []

      if (cacheKey) {
        setZoneNodes(cacheKey, nodes)
      }

      return nodes
    } catch (err) {
      console.error('zones store: fetchZoneNodes failed', err)
      throw err
    }
  }

  async function fetchZoneRootNode(id: number) {
    try {
      const response = await axios.get(`/api/zones/${id}/root-node`)
      return response?.data?.data ?? response?.data ?? null
    } catch (err) {
      console.error('zones store: fetchZoneRootNode failed', err)
      throw err
    }
  }

  async function fetchZoneStats(id: number, options: FetchOptions = {}) {
    if (!options.refresh && zoneStatsCache.value[id]) {
      return zoneStatsCache.value[id]
    }

    try {
      const response = await axios.get(`/api/zones/${id}/statistics`)
      const stats = response?.data?.data ?? response?.data ?? null
      if (stats) {
        zoneStatsCache.value = { ...zoneStatsCache.value, [id]: stats }
      }
      return stats
    } catch (err) {
      console.error('zones store: fetchZoneStats failed', err)
      throw err
    }
  }

  async function createZone(zoneData: Partial<Zone>): Promise<Zone> {
    try {
      const response = await axios.post('/api/zones', zoneData)
      const zone = extractZoneFromResponse(response?.data ?? response)
      if (!zone) {
        throw new Error('Zone creation response malformed')
      }

      zones.value.push(zone)
      if (!selectedZone.value && zone.mesh_network_id) {
        selectZone(zone.mesh_network_id)
      }

      return zone
    } catch (err) {
      console.error('zones store: createZone failed', err)
      throw err
    }
  }

  async function updateZone(id: number, zoneData: Partial<Zone>): Promise<Zone> {
    try {
      const response = await axios.put(`/api/zones/${id}`, zoneData)
      const updated = extractZoneFromResponse(response?.data ?? response)
      if (!updated) {
        throw new Error('Zone update response malformed')
      }

      const index = zones.value.findIndex((z) => z.id === id)
      if (index !== -1) {
        zones.value[index] = updated
      } else {
        zones.value.push(updated)
      }

      reconcileSelection()
      return updated
    } catch (err) {
      console.error('zones store: updateZone failed', err)
      throw err
    }
  }

  async function deleteZone(id: number): Promise<void> {
    try {
      await axios.delete(`/api/zones/${id}`)
      const removed = zones.value.find((z) => z.id === id)

      zones.value = zones.value.filter((z) => z.id !== id)
      delete zoneNodesCache.value[id]
      delete zoneStatsCache.value[id]

      if (removed && removed.mesh_network_id === selectedZone.value) {
        reconcileSelection()
      }
    } catch (err) {
      console.error('zones store: deleteZone failed', err)
      throw err
    }
  }

  async function sendZoneCommand(id: number, nodeId: string, command: string, params?: any) {
    try {
      const response = await axios.post(`/api/zones/${id}/command`, {
        node_id: nodeId,
        command,
        params,
      })
      return response?.data ?? response
    } catch (err) {
      console.error('zones store: sendZoneCommand failed', err)
      throw err
    }
  }

  async function checkNodesAvailability(nodeIds: string[], excludeZoneId?: number) {
    try {
      const response = await axios.post('/api/zones/check-nodes-availability', {
        node_ids: nodeIds,
        zone_id: excludeZoneId,
      })
      return response?.data?.data ?? response?.data ?? {}
    } catch (err) {
      console.error('zones store: checkNodesAvailability failed', err)
      throw err
    }
  }

  function getZoneById(id: number): Zone | undefined {
    return zones.value.find((z) => z.id === id)
  }

  function getZoneByRootNode(rootNodeId: string): Zone | undefined {
    return zones.value.find((z) => z.root_node_id === rootNodeId)
  }

  function getZoneNodes(zoneId: number): any[] {
    return zoneNodesCache.value[zoneId] ?? []
  }

  function updateZoneGreenhouse(zoneId: number, greenhouseId: number | null, greenhouseName?: string | null) {
    const index = zones.value.findIndex((z) => z.id === zoneId)
    if (index === -1) {
      return
    }

    zones.value[index] = {
      ...zones.value[index],
      greenhouse_id: greenhouseId ?? null,
      greenhouse_name: greenhouseName ?? null,
    }
  }

  function getZonesByGreenhouse(greenhouseId: number | null) {
    if (greenhouseId == null) {
      return zones.value.filter((zone) => zone.greenhouse_id == null)
    }
    return zones.value.filter((zone) => zone.greenhouse_id === greenhouseId)
  }

  function clearError() {
    error.value = null
  }

  function resetStore() {
    zones.value = []
    selectedZone.value = null
    loading.value = false
    error.value = null
    zoneNodesCache.value = {}
    zoneStatsCache.value = {}
    persistMeshId(null)
  }

  return {
    zones,
    selectedZone,
    loading,
    error,
    hasZones,
    zoneOptions,
    selectedZoneSummary,
    activeZones,
    availableZones,
    busyZones,
    zonesByType,
    zonesByGreenhouse,
    fetchZones,
    fetchZone,
    fetchZoneNodes,
    fetchZoneRootNode,
    fetchZoneStats,
    createZone,
    updateZone,
    deleteZone,
    sendZoneCommand,
    checkNodesAvailability,
    selectZone,
    clearSelection,
    setZones,
    getZoneById,
    getZoneByRootNode,
    getZoneNodes,
    getZonesByGreenhouse,
    clearError,
    resetStore,
    updateZoneGreenhouse,
  }
})

