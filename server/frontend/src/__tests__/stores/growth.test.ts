/**
 * ⭐ GROWTH PLANNER: Тесты для growth store
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useGrowthStore, type GrowthCulture, type GrowthPreset, type GrowthCycle } from '@/stores/growth'
import axios from 'axios'

vi.mock('axios')
const mockedAxios = axios as any

describe('Growth Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
  })

  describe('State', () => {
    it('should have initial state', () => {
      const store = useGrowthStore()
      
      expect(store.cultures).toEqual([])
      expect(store.presets).toEqual([])
      expect(store.cycles).toEqual([])
      expect(store.selectedPreset).toBeNull()
      expect(store.selectedCycle).toBeNull()
      expect(store.loading).toBe(false)
      expect(store.error).toBeNull()
    })
  })

  describe('Computed - Cultures', () => {
    it('should filter active cultures', () => {
      const store = useGrowthStore()
      
      store.cultures = [
        { id: 1, name: 'Culture 1', is_active: true } as GrowthCulture,
        { id: 2, name: 'Culture 2', is_active: false } as GrowthCulture,
        { id: 3, name: 'Culture 3', is_active: true } as GrowthCulture,
      ]
      
      expect(store.activeCultures).toHaveLength(2)
    })

    it('should group cultures by category', () => {
      const store = useGrowthStore()
      
      store.cultures = [
        { id: 1, name: 'Lettuce', category: 'leafy_greens' } as GrowthCulture,
        { id: 2, name: 'Strawberry', category: 'berries' } as GrowthCulture,
        { id: 3, name: 'Lettuce 2', category: 'leafy_greens' } as GrowthCulture,
      ]
      
      expect(store.culturesByCategory.leafy_greens).toHaveLength(2)
      expect(store.culturesByCategory.berries).toHaveLength(1)
    })
  })

  describe('Computed - Presets', () => {
    it('should filter system presets', () => {
      const store = useGrowthStore()
      
      store.presets = [
        { id: 1, name: 'Preset 1', preset_type: 'system', is_active: true } as GrowthPreset,
        { id: 2, name: 'Preset 2', preset_type: 'custom', is_active: true } as GrowthPreset,
        { id: 3, name: 'Preset 3', preset_type: 'system', is_active: true } as GrowthPreset,
      ]
      
      expect(store.systemPresets).toHaveLength(2)
    })

    it('should filter custom presets', () => {
      const store = useGrowthStore()
      
      store.presets = [
        { id: 1, name: 'Preset 1', preset_type: 'system', is_active: true } as GrowthPreset,
        { id: 2, name: 'Preset 2', preset_type: 'custom', is_active: true } as GrowthPreset,
      ]
      
      expect(store.customPresets).toHaveLength(1)
    })

    it('should group presets by culture', () => {
      const store = useGrowthStore()
      
      store.presets = [
        { id: 1, name: 'Preset 1', culture_id: 1 } as GrowthPreset,
        { id: 2, name: 'Preset 2', culture_id: 2 } as GrowthPreset,
        { id: 3, name: 'Preset 3', culture_id: 1 } as GrowthPreset,
      ]
      
      expect(store.presetsByCulture[1]).toHaveLength(2)
      expect(store.presetsByCulture[2]).toHaveLength(1)
    })
  })

  describe('Computed - Cycles', () => {
    it('should filter active cycles', () => {
      const store = useGrowthStore()
      
      store.cycles = [
        { id: 1, status: 'active' } as GrowthCycle,
        { id: 2, status: 'harvested' } as GrowthCycle,
        { id: 3, status: 'active' } as GrowthCycle,
      ]
      
      expect(store.activeCycles).toHaveLength(2)
    })

    it('should filter completed cycles', () => {
      const store = useGrowthStore()
      
      store.cycles = [
        { id: 1, status: 'active' } as GrowthCycle,
        { id: 2, status: 'harvested' } as GrowthCycle,
        { id: 3, status: 'ended' } as GrowthCycle,
      ]
      
      expect(store.completedCycles).toHaveLength(2)
    })

    it('should group cycles by zone', () => {
      const store = useGrowthStore()
      
      store.cycles = [
        { id: 1, zone_id: 1, status: 'active' } as GrowthCycle,
        { id: 2, zone_id: 2, status: 'active' } as GrowthCycle,
        { id: 3, zone_id: 1, status: 'harvested' } as GrowthCycle,
      ]
      
      expect(store.cyclesByZone[1]).toHaveLength(2)
      expect(store.cyclesByZone[2]).toHaveLength(1)
    })
  })

  describe('Actions - Cultures', () => {
    it('should fetch cultures', async () => {
      const store = useGrowthStore()
      const mockCultures = [
        { id: 1, name: 'Lettuce', category: 'leafy_greens' },
        { id: 2, name: 'Strawberry', category: 'berries' },
      ]
      
      mockedAxios.get.mockResolvedValue({ data: mockCultures })
      
      await store.fetchCultures()
      
      expect(mockedAxios.get).toHaveBeenCalledWith('/api/growth/cultures')
      expect(store.cultures).toEqual(mockCultures)
    })

    it('should create culture', async () => {
      const store = useGrowthStore()
      const newCulture = { name: 'New Culture', slug: 'new-culture', category: 'vegetables' }
      const createdCulture = { id: 1, ...newCulture }
      
      mockedAxios.post.mockResolvedValue({ data: createdCulture })
      
      const result = await store.createCulture(newCulture)
      
      expect(mockedAxios.post).toHaveBeenCalledWith('/api/growth/cultures', newCulture)
      expect(store.cultures).toContainEqual(createdCulture)
      expect(result).toEqual(createdCulture)
    })
  })

  describe('Actions - Presets', () => {
    it('should fetch presets with filters', async () => {
      const store = useGrowthStore()
      const mockPresets = [
        { id: 1, name: 'Preset 1', culture_id: 1 },
        { id: 2, name: 'Preset 2', culture_id: 2 },
      ]
      
      mockedAxios.get.mockResolvedValue({ data: mockPresets })
      
      await store.fetchPresets({ culture_id: 1, type: 'system' })
      
      expect(mockedAxios.get).toHaveBeenCalledWith('/api/growth/presets', {
        params: { culture_id: 1, type: 'system' }
      })
      expect(store.presets).toEqual(mockPresets)
    })

    it('should clone preset', async () => {
      const store = useGrowthStore()
      const originalPreset = { id: 1, name: 'Original Preset' } as GrowthPreset
      const clonedPreset = { id: 2, name: 'Cloned Preset' } as GrowthPreset
      
      mockedAxios.post.mockResolvedValue({ data: clonedPreset })
      
      const result = await store.clonePreset(1, 'Cloned Preset')
      
      expect(mockedAxios.post).toHaveBeenCalledWith('/api/growth/presets/1/clone', { name: 'Cloned Preset' })
      expect(store.presets).toContainEqual(clonedPreset)
      expect(result).toEqual(clonedPreset)
    })
  })

  describe('Actions - Cycles', () => {
    it('should create cycle', async () => {
      const store = useGrowthStore()
      const cycleData = { zone_id: 1, preset_id: 1, plant_count: 10 }
      const createdCycle = { id: 1, ...cycleData, status: 'active' } as GrowthCycle
      
      mockedAxios.post.mockResolvedValue({ data: createdCycle })
      
      const result = await store.createCycle(cycleData)
      
      expect(mockedAxios.post).toHaveBeenCalledWith('/api/growth/cycles', cycleData)
      expect(store.cycles).toContainEqual(createdCycle)
      expect(result).toEqual(createdCycle)
    })

    it('should harvest cycle', async () => {
      const store = useGrowthStore()
      const cycle = { id: 1, status: 'active' } as GrowthCycle
      store.cycles = [cycle]
      
      const harvestedCycle = { id: 1, status: 'harvested', harvest_weight_kg: 5.5 } as GrowthCycle
      mockedAxios.post.mockResolvedValue({ data: harvestedCycle })
      
      await store.harvestCycle(1, { harvest_weight_kg: 5.5, rating: 5 })
      
      expect(mockedAxios.post).toHaveBeenCalledWith('/api/growth/cycles/1/harvest', {
        harvest_weight_kg: 5.5,
        rating: 5
      })
      expect(store.cycles[0].status).toBe('harvested')
    })

    it('should cancel cycle', async () => {
      const store = useGrowthStore()
      const cycle = { id: 1, status: 'active' } as GrowthCycle
      store.cycles = [cycle]
      
      const cancelledCycle = { id: 1, status: 'cancelled' } as GrowthCycle
      mockedAxios.post.mockResolvedValue({ data: cancelledCycle })
      
      await store.cancelCycle(1, 'User cancelled')
      
      expect(mockedAxios.post).toHaveBeenCalledWith('/api/growth/cycles/1/cancel', { reason: 'User cancelled' })
      expect(store.cycles[0].status).toBe('cancelled')
    })

    it('should get cycle by zone', () => {
      const store = useGrowthStore()
      const cycle1 = { id: 1, zone_id: 1, status: 'active' } as GrowthCycle
      const cycle2 = { id: 2, zone_id: 2, status: 'active' } as GrowthCycle
      const cycle3 = { id: 3, zone_id: 1, status: 'harvested' } as GrowthCycle
      
      store.cycles = [cycle1, cycle2, cycle3]
      
      expect(store.getCycleByZone(1)).toEqual(cycle1)
      expect(store.getCycleByZone(2)).toEqual(cycle2)
      expect(store.getCycleByZone(3)).toBeUndefined()
    })
  })
})

