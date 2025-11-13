import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import greenhousesApi from '@/services/greenhouses-api'
import { useZonesStore } from '@/stores/zones'
import { useNodesStore } from '@/stores/nodes'
import type {
  AttachNodePayload,
  AttachZonePayload,
  ClimateProfile,
  CreateGreenhousePayload,
  GreenhouseAutomationRule,
  GreenhouseDetail,
  GreenhouseNodeLink,
  GreenhouseSummary,
  GreenhouseZoneLink,
  UpdateGreenhousePayload,
} from '@/types/greenhouse'

interface FetchOptions {
  refresh?: boolean
  params?: Record<string, any>
}

const STORAGE_KEY = 'mesh_hydro.selected_greenhouse'

function loadStoredId(): number | null {
  try {
    const raw = localStorage.getItem(STORAGE_KEY)
    if (!raw) {
      return null
    }
    const parsed = Number(raw)
    return Number.isFinite(parsed) ? parsed : null
  } catch (error) {
    console.warn('greenhouses store: failed to load selected greenhouse id', error)
    return null
  }
}

function persistSelectedId(value: number | null) {
  try {
    if (value === null || value === undefined) {
      localStorage.removeItem(STORAGE_KEY)
      return
    }
    localStorage.setItem(STORAGE_KEY, String(value))
  } catch (error) {
    console.warn('greenhouses store: failed to persist selected greenhouse id', error)
  }
}

export const useGreenhousesStore = defineStore('greenhouses', () => {
  const zonesStore = useZonesStore()
  const nodesStore = useNodesStore()

  const items = ref<GreenhouseSummary[]>([])
  const detailCache = ref<Record<number, GreenhouseDetail>>({})
  const zonesCache = ref<Record<number, GreenhouseZoneLink[]>>({})
  const nodesCache = ref<Record<number, GreenhouseNodeLink[]>>({})
  const automationCache = ref<Record<number, GreenhouseAutomationRule[]>>({})

  const loading = ref(false)
  const error = ref<string | null>(null)
  const selectedId = ref<number | null>(loadStoredId())

  const hasGreenhouses = computed(() => items.value.length > 0)

  const selectedGreenhouse = computed<GreenhouseDetail | GreenhouseSummary | null>(() => {
    if (!selectedId.value) {
      return null
    }
    return (
      detailCache.value[selectedId.value] ||
      items.value.find((item) => item.id === selectedId.value) ||
      null
    )
  })

  const greenhouseOptions = computed(() =>
    items.value.map((item) => ({
      value: item.id,
      label: item.name,
      description: item.description,
      zones: item.zone_count,
      nodes: item.node_count,
    })),
  )

  const knownClimateProfiles = computed<ClimateProfile[]>(() => {
    const collected = new Map<string, ClimateProfile>()

    const collect = (profiles?: ClimateProfile[] | null) => {
      if (!profiles?.length) {
        return
      }

      profiles.forEach((profile) => {
        if (!profile?.name) {
          return
        }

        collected.set(profile.name, { ...profile })
      })
    }

    items.value.forEach((item) => collect(item.climate_profiles))
    Object.values(detailCache.value).forEach((detail) => {
      collect(detail.climate_profiles)
      collect(detail.settings?.climate_profiles ?? null)
    })

    return Array.from(collected.values())
  })

  function setGreenhouses(newItems: GreenhouseSummary[]) {
    items.value = newItems
    reconcileSelection()
  }

  function reconcileSelection() {
    if (!items.value.length) {
      selectGreenhouse(null)
      return
    }

    if (selectedId.value) {
      const exists = items.value.some((item) => item.id === selectedId.value)
      if (exists) {
        return
      }
    }

    selectGreenhouse(items.value[0]?.id ?? null)
  }

  function selectGreenhouse(id: number | null) {
    if (selectedId.value === id) {
      return
    }
    selectedId.value = id
    persistSelectedId(id)
  }

  function upsertSummary(greenhouse: GreenhouseSummary | GreenhouseDetail) {
    const index = items.value.findIndex((item) => item.id === greenhouse.id)
    const summary: GreenhouseSummary = {
      id: greenhouse.id,
      name: greenhouse.name,
      code: greenhouse.code,
      description: greenhouse.description,
      status: greenhouse.status,
      root_node_id: greenhouse.root_node_id,
      root_node_mac: greenhouse.root_node_mac,
      zone_count: greenhouse.zone_count,
      node_count: greenhouse.node_count,
      active_cycle_count: greenhouse.active_cycle_count,
      alert_count: greenhouse.alert_count,
      image_url: greenhouse.image_url,
      created_at: greenhouse.created_at,
      updated_at: greenhouse.updated_at,
      climate_profiles:
        greenhouse.climate_profiles ??
        ('settings' in greenhouse ? greenhouse.settings?.climate_profiles ?? null : null),
    }

    if (index === -1) {
      items.value.unshift(summary)
    } else {
      items.value[index] = summary
    }
  }

  async function fetchGreenhouses(options: FetchOptions = {}) {
    if (loading.value) {
      await Promise.resolve()
    }

    if (!options.refresh && items.value.length) {
      return items.value
    }

    loading.value = true
    error.value = null
    try {
      const { greenhouses } = await greenhousesApi.list(options.params ?? {})
      setGreenhouses(greenhouses)
      return greenhouses
    } catch (err: any) {
      console.error('greenhouses store: failed to fetch list', err)
      error.value =
        err?.response?.data?.message || err?.message || 'Не удалось загрузить список теплиц'
      setGreenhouses([])
      return []
    } finally {
      loading.value = false
    }
  }

  async function fetchGreenhouse(id: number, options: FetchOptions = {}) {
    if (!options.refresh && detailCache.value[id]) {
      return detailCache.value[id]
    }

    loading.value = true
    error.value = null
    try {
      const { greenhouse } = await greenhousesApi.get(id, options.params ?? {})
      if (greenhouse) {
        detailCache.value[id] = greenhouse
        upsertSummary(greenhouse)
      }
      return greenhouse ?? null
    } catch (err: any) {
      console.error(`greenhouses store: failed to fetch greenhouse ${id}`, err)
      error.value =
        err?.response?.data?.message || err?.message || 'Не удалось загрузить данные теплицы'
      return null
    } finally {
      loading.value = false
    }
  }

  async function createGreenhouse(payload: CreateGreenhousePayload) {
    loading.value = true
    error.value = null
    try {
      const greenhouse = await greenhousesApi.create(payload)
      if (greenhouse) {
        detailCache.value[greenhouse.id] = greenhouse
        upsertSummary(greenhouse)
        selectGreenhouse(greenhouse.id)
      }
      return greenhouse
    } catch (err: any) {
      console.error('greenhouses store: failed to create greenhouse', err)
      error.value = err?.response?.data?.message || err?.message || 'Не удалось создать теплицу'
      throw err
    } finally {
      loading.value = false
    }
  }

  async function updateGreenhouse(id: number, payload: UpdateGreenhousePayload) {
    loading.value = true
    error.value = null
    try {
      const greenhouse = await greenhousesApi.update(id, payload)
      if (greenhouse) {
        detailCache.value[id] = greenhouse
        upsertSummary(greenhouse)
      }
      return greenhouse
    } catch (err: any) {
      console.error(`greenhouses store: failed to update greenhouse ${id}`, err)
      error.value = err?.response?.data?.message || err?.message || 'Не удалось обновить теплицу'
      throw err
    } finally {
      loading.value = false
    }
  }

  async function deleteGreenhouse(id: number) {
    loading.value = true
    error.value = null
    try {
      await greenhousesApi.remove(id)
      items.value = items.value.filter((item) => item.id !== id)
      delete detailCache.value[id]
      delete zonesCache.value[id]
      delete nodesCache.value[id]
      delete automationCache.value[id]

      zonesStore.getZonesByGreenhouse(id).forEach((zone) => {
        zonesStore.updateZoneGreenhouse(zone.id, null, null)
      })

      nodesStore.nodes
        .filter((node) => node.greenhouse_id === id)
        .forEach((node) => nodesStore.setNodeGreenhouse(node.node_id, null, null))

      if (selectedId.value === id) {
        selectGreenhouse(null)
      }
    } catch (err: any) {
      console.error(`greenhouses store: failed to delete greenhouse ${id}`, err)
      error.value = err?.response?.data?.message || err?.message || 'Не удалось удалить теплицу'
      throw err
    } finally {
      loading.value = false
    }
  }

  async function fetchZones(id: number, options: FetchOptions = {}) {
    if (!options.refresh && zonesCache.value[id]) {
      return zonesCache.value[id]
    }

    try {
      const zones = await greenhousesApi.listZones(id)
      zonesCache.value[id] = zones
      const greenhouseName =
        detailCache.value[id]?.name || items.value.find((item) => item.id === id)?.name || null
      zones.forEach((zone) => {
        zonesStore.updateZoneGreenhouse(zone.zone_id, id, greenhouseName)
      })
      return zones
    } catch (err: any) {
      console.error(`greenhouses store: failed to fetch zones for greenhouse ${id}`, err)
      throw err
    }
  }

  async function attachZone(id: number, payload: AttachZonePayload) {
    try {
      const zoneLink = await greenhousesApi.attachZone(id, payload)
      if (!zoneLink) {
        return null
      }

      const current = zonesCache.value[id] ?? []
      const index = current.findIndex((item) => item.zone_id === zoneLink.zone_id)
      if (index === -1) {
        zonesCache.value[id] = [...current, zoneLink]
      } else {
        zonesCache.value[id] = [
          ...current.slice(0, index),
          zoneLink,
          ...current.slice(index + 1),
        ]
      }

      const summary = detailCache.value[id]
      if (summary) {
        summary.zone_count = (summary.zone_count ?? 0) + (index === -1 ? 1 : 0)
        detailCache.value[id] = { ...summary }
      }

      const baseSummary = items.value.find((item) => item.id === id)
      if (baseSummary && index === -1) {
        baseSummary.zone_count += 1
      }

      const greenhouseName =
        detailCache.value[id]?.name || items.value.find((item) => item.id === id)?.name || null
      zonesStore.updateZoneGreenhouse(zoneLink.zone_id, id, greenhouseName)

      return zoneLink
    } catch (err: any) {
      console.error(`greenhouses store: failed to attach zone to greenhouse ${id}`, err)
      throw err
    }
  }

  async function detachZone(id: number, zoneId: number) {
    try {
      await greenhousesApi.detachZone(id, zoneId)
      const current = zonesCache.value[id] ?? []
      zonesCache.value[id] = current.filter((item) => item.zone_id !== zoneId)

      const summary = detailCache.value[id]
      if (summary) {
        summary.zone_count = Math.max(0, (summary.zone_count ?? 1) - 1)
        detailCache.value[id] = { ...summary }
      }

      const baseSummary = items.value.find((item) => item.id === id)
      if (baseSummary) {
        baseSummary.zone_count = Math.max(0, baseSummary.zone_count - 1)
      }

      zonesStore.updateZoneGreenhouse(zoneId, null, null)
    } catch (err: any) {
      console.error(`greenhouses store: failed to detach zone ${zoneId} from greenhouse ${id}`, err)
      throw err
    }
  }

  async function fetchNodes(id: number, options: FetchOptions = {}) {
    if (!options.refresh && nodesCache.value[id]) {
      return nodesCache.value[id]
    }

    try {
      const nodes = await greenhousesApi.listNodes(id)
      nodesCache.value[id] = nodes
      const greenhouseName =
        detailCache.value[id]?.name || items.value.find((item) => item.id === id)?.name || null
      nodes.forEach((node) => {
        nodesStore.setNodeGreenhouse(node.node_id, id, greenhouseName)
      })
      return nodes
    } catch (err: any) {
      console.error(`greenhouses store: failed to fetch nodes for greenhouse ${id}`, err)
      throw err
    }
  }

  async function attachNode(id: number, payload: AttachNodePayload) {
    try {
      const nodeLink = await greenhousesApi.attachNode(id, payload)
      if (!nodeLink) {
        return null
      }

      const current = nodesCache.value[id] ?? []
      const index = current.findIndex((item) => item.node_id === nodeLink.node_id)
      if (index === -1) {
        nodesCache.value[id] = [...current, nodeLink]
      } else {
        nodesCache.value[id] = [
          ...current.slice(0, index),
          nodeLink,
          ...current.slice(index + 1),
        ]
      }

      const summary = detailCache.value[id]
      if (summary) {
        summary.node_count = (summary.node_count ?? 0) + (index === -1 ? 1 : 0)
        detailCache.value[id] = { ...summary }
      }

      const baseSummary = items.value.find((item) => item.id === id)
      if (baseSummary && index === -1) {
        baseSummary.node_count += 1
      }

      const greenhouseName =
        detailCache.value[id]?.name || items.value.find((item) => item.id === id)?.name || null
      nodesStore.setNodeGreenhouse(nodeLink.node_id, id, greenhouseName)

      return nodeLink
    } catch (err: any) {
      console.error(`greenhouses store: failed to attach node to greenhouse ${id}`, err)
      throw err
    }
  }

  async function detachNode(id: number, nodeId: string) {
    try {
      await greenhousesApi.detachNode(id, nodeId)
      const current = nodesCache.value[id] ?? []
      nodesCache.value[id] = current.filter((item) => item.node_id !== nodeId)

      const summary = detailCache.value[id]
      if (summary) {
        summary.node_count = Math.max(0, (summary.node_count ?? 1) - 1)
        detailCache.value[id] = { ...summary }
      }

      const baseSummary = items.value.find((item) => item.id === id)
      if (baseSummary) {
        baseSummary.node_count = Math.max(0, baseSummary.node_count - 1)
      }

      nodesStore.setNodeGreenhouse(nodeId, null, null)
    } catch (err: any) {
      console.error(`greenhouses store: failed to detach node ${nodeId} from greenhouse ${id}`, err)
      throw err
    }
  }

  async function fetchAutomationRules(id: number, options: FetchOptions = {}) {
    if (!options.refresh && automationCache.value[id]) {
      return automationCache.value[id]
    }

    try {
      const rules = await greenhousesApi.listAutomationRules(id)
      automationCache.value[id] = rules
      return rules
    } catch (err: any) {
      console.error(`greenhouses store: failed to fetch automation rules for greenhouse ${id}`, err)
      throw err
    }
  }

  async function createAutomationRule(id: number, payload: Partial<GreenhouseAutomationRule>) {
    try {
      const rule = await greenhousesApi.createAutomationRule(id, payload)
      if (!rule) {
        return null
      }

      const current = automationCache.value[id] ?? []
      automationCache.value[id] = [rule, ...current]
      return rule
    } catch (err: any) {
      console.error(`greenhouses store: failed to create automation rule for greenhouse ${id}`, err)
      throw err
    }
  }

  async function updateAutomationRule(
    id: number,
    ruleId: number,
    payload: Partial<GreenhouseAutomationRule>,
  ) {
    try {
      const rule = await greenhousesApi.updateAutomationRule(id, ruleId, payload)
      if (!rule) {
        return null
      }

      const current = automationCache.value[id] ?? []
      const index = current.findIndex((item) => item.id === ruleId)
      if (index === -1) {
        automationCache.value[id] = [rule, ...current]
      } else {
        automationCache.value[id] = [
          ...current.slice(0, index),
          rule,
          ...current.slice(index + 1),
        ]
      }

      return rule
    } catch (err: any) {
      console.error(`greenhouses store: failed to update automation rule ${ruleId}`, err)
      throw err
    }
  }

  async function deleteAutomationRule(id: number, ruleId: number) {
    try {
      await greenhousesApi.deleteAutomationRule(id, ruleId)
      const current = automationCache.value[id] ?? []
      automationCache.value[id] = current.filter((item) => item.id !== ruleId)
    } catch (err: any) {
      console.error(`greenhouses store: failed to delete automation rule ${ruleId}`, err)
      throw err
    }
  }

  function getZonesFromCache(id: number) {
    return zonesCache.value[id] ?? []
  }

  function getNodesFromCache(id: number) {
    return nodesCache.value[id] ?? []
  }

  function getAutomationFromCache(id: number) {
    return automationCache.value[id] ?? []
  }

  function clearError() {
    error.value = null
  }

  function resetStore() {
    items.value = []
    detailCache.value = {}
    zonesCache.value = {}
    nodesCache.value = {}
    automationCache.value = {}
    loading.value = false
    error.value = null
    selectGreenhouse(null)
  }

  return {
    items,
    detailCache,
    zonesCache,
    nodesCache,
    automationCache,
    loading,
    error,
    selectedId,
    hasGreenhouses,
    selectedGreenhouse,
    greenhouseOptions,
    knownClimateProfiles,
    fetchGreenhouses,
    fetchGreenhouse,
    createGreenhouse,
    updateGreenhouse,
    deleteGreenhouse,
    selectGreenhouse,
    fetchZones,
    attachZone,
    detachZone,
    fetchNodes,
    attachNode,
    detachNode,
    fetchAutomationRules,
    createAutomationRule,
    updateAutomationRule,
    deleteAutomationRule,
    getZonesFromCache,
    getNodesFromCache,
    getAutomationFromCache,
    clearError,
    resetStore,
  }
})


