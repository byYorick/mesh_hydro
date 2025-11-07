/**
 * ⭐ ЗОНИРОВАНИЕ: Тесты для zones store
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useZonesStore, type Zone } from '@/stores/zones'
import axios from 'axios'

vi.mock('axios')
const mockedAxios = axios as any

describe('Zones Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
  })

  describe('State', () => {
    it('should have initial state', () => {
      const store = useZonesStore()
      
      expect(store.zones).toEqual([])
      expect(store.selectedZone).toBeNull()
      expect(store.loading).toBe(false)
      expect(store.error).toBeNull()
    })
  })

  describe('Computed', () => {
    it('should filter active zones', () => {
      const store = useZonesStore()
      
      store.zones = [
        { id: 1, name: 'Zone 1', is_active: true } as Zone,
        { id: 2, name: 'Zone 2', is_active: false } as Zone,
        { id: 3, name: 'Zone 3', is_active: true } as Zone,
      ]
      
      expect(store.activeZones).toHaveLength(2)
      expect(store.activeZones[0].name).toBe('Zone 1')
      expect(store.activeZones[1].name).toBe('Zone 3')
    })

    it('should filter available zones', () => {
      const store = useZonesStore()
      
      store.zones = [
        { id: 1, name: 'Zone 1', is_active: true, is_available: true, current_cycle_id: null } as Zone,
        { id: 2, name: 'Zone 2', is_active: true, is_available: false, current_cycle_id: null } as Zone,
        { id: 3, name: 'Zone 3', is_active: true, is_available: true, current_cycle_id: 1 } as Zone,
      ]
      
      expect(store.availableZones).toHaveLength(1)
      expect(store.availableZones[0].name).toBe('Zone 1')
    })

    it('should filter busy zones', () => {
      const store = useZonesStore()
      
      store.zones = [
        { id: 1, name: 'Zone 1', is_active: true, current_cycle_id: null } as Zone,
        { id: 2, name: 'Zone 2', is_active: true, current_cycle_id: 1 } as Zone,
        { id: 3, name: 'Zone 3', is_active: true, current_cycle_id: 2 } as Zone,
      ]
      
      expect(store.busyZones).toHaveLength(2)
    })

    it('should group zones by type', () => {
      const store = useZonesStore()
      
      store.zones = [
        { id: 1, name: 'Zone 1', zone_type: 'nft' } as Zone,
        { id: 2, name: 'Zone 2', zone_type: 'dwc' } as Zone,
        { id: 3, name: 'Zone 3', zone_type: 'nft' } as Zone,
      ]
      
      expect(store.zonesByType.nft).toHaveLength(2)
      expect(store.zonesByType.dwc).toHaveLength(1)
    })
  })

  describe('Actions', () => {
    it('should fetch zones', async () => {
      const store = useZonesStore()
      const mockZones = [
        { id: 1, name: 'Zone 1', is_active: true },
        { id: 2, name: 'Zone 2', is_active: true },
      ]
      
      mockedAxios.get.mockResolvedValue({ data: mockZones })
      
      await store.fetchZones()
      
      expect(mockedAxios.get).toHaveBeenCalledWith('/api/zones')
      expect(store.zones).toEqual(mockZones)
      expect(store.loading).toBe(false)
    })

    it('should handle fetch zones error', async () => {
      const store = useZonesStore()
      const errorMessage = 'Network error'
      
      mockedAxios.get.mockRejectedValue({ response: { data: { message: errorMessage } } })
      
      await store.fetchZones()
      
      expect(store.error).toBe(errorMessage)
      expect(store.loading).toBe(false)
    })

    it('should create zone', async () => {
      const store = useZonesStore()
      const newZone = { name: 'New Zone', root_node_id: 'root_001', mesh_network_id: 'MESH_001', mqtt_topic_prefix: 'hydro/zone1/', zone_type: 'nft' }
      const createdZone = { id: 1, ...newZone }
      
      mockedAxios.post.mockResolvedValue({ data: createdZone })
      
      const result = await store.createZone(newZone)
      
      expect(mockedAxios.post).toHaveBeenCalledWith('/api/zones', newZone)
      expect(store.zones).toContainEqual(createdZone)
      expect(result).toEqual(createdZone)
    })

    it('should update zone', async () => {
      const store = useZonesStore()
      const existingZone = { id: 1, name: 'Zone 1', is_active: true } as Zone
      store.zones = [existingZone]
      
      const updatedZone = { id: 1, name: 'Updated Zone', is_active: true }
      mockedAxios.put.mockResolvedValue({ data: updatedZone })
      
      await store.updateZone(1, { name: 'Updated Zone' })
      
      expect(mockedAxios.put).toHaveBeenCalledWith('/api/zones/1', { name: 'Updated Zone' })
      expect(store.zones[0].name).toBe('Updated Zone')
    })

    it('should delete zone', async () => {
      const store = useZonesStore()
      const zone = { id: 1, name: 'Zone 1' } as Zone
      store.zones = [zone]
      
      mockedAxios.delete.mockResolvedValue({})
      
      await store.deleteZone(1)
      
      expect(mockedAxios.delete).toHaveBeenCalledWith('/api/zones/1')
      expect(store.zones).not.toContainEqual(zone)
    })

    it('should get zone by id', () => {
      const store = useZonesStore()
      const zone1 = { id: 1, name: 'Zone 1' } as Zone
      const zone2 = { id: 2, name: 'Zone 2' } as Zone
      store.zones = [zone1, zone2]
      
      expect(store.getZoneById(1)).toEqual(zone1)
      expect(store.getZoneById(3)).toBeUndefined()
    })

    it('should get zone by root node id', () => {
      const store = useZonesStore()
      const zone1 = { id: 1, name: 'Zone 1', root_node_id: 'root_001' } as Zone
      const zone2 = { id: 2, name: 'Zone 2', root_node_id: 'root_002' } as Zone
      store.zones = [zone1, zone2]
      
      expect(store.getZoneByRootNode('root_001')).toEqual(zone1)
      expect(store.getZoneByRootNode('root_003')).toBeUndefined()
    })

    it('should clear error', () => {
      const store = useZonesStore()
      store.error = 'Some error'
      
      store.clearError()
      
      expect(store.error).toBeNull()
    })
  })
})

