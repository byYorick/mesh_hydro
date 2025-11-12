/**
 * ⭐ GROWTH PLANNER: Pinia Store для управления циклами роста
 */
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import axios from 'axios'

export interface GrowthCulture {
  id: number
  name: string
  slug: string
  category: 'leafy_greens' | 'berries' | 'vegetables' | 'herbs'
  description?: string
  typical_cycle_days?: number
  optimal_temp_min?: number
  optimal_temp_max?: number
  optimal_humidity_min?: number
  optimal_humidity_max?: number
  image_url?: string
  is_system: boolean
  is_active: boolean
  icon?: string
  category_color?: string
}

export interface GrowthStage {
  id: number
  preset_id: number
  name: string
  order: number
  duration_days: number
  target_params: Record<string, any>
  description?: string
  care_instructions?: string
  auto_actions?: any[]
  icon?: string
  color?: string
}

export interface GrowthPreset {
  id: number
  culture_id: number
  name: string
  slug: string
  description?: string
  preset_type: 'system' | 'custom'
  total_days: number
  difficulty: 'easy' | 'medium' | 'hard'
  recommended_system: 'nft' | 'dwc' | 'drip' | 'ebb_flow' | 'any'
  usage_count: number
  avg_rating?: number
  is_public: boolean
  is_active: boolean
  
  // Relations
  culture?: GrowthCulture
  stages?: GrowthStage[]
  difficulty_icon?: string
  difficulty_color?: string
}

export interface GrowthCycle {
  id: number
  zone_id: number
  preset_id: number
  culture_id: number
  greenhouse_id?: number | null
  current_stage_id?: number
  started_at: string
  expected_harvest_at?: string
  actual_harvest_at?: string
  ended_at?: string
  status: 'planning' | 'active' | 'paused' | 'harvested' | 'failed' | 'cancelled'
  plant_count?: number
  harvest_weight_kg?: number
  notes?: string
  rating?: number
  
  // Relations
  zone?: any
  preset?: GrowthPreset
  culture?: GrowthCulture
  current_stage?: GrowthStage
  stage_history?: any[]
  notifications?: any[]
  
  // Computed
  progress?: number
  current_day?: number
  remaining_days?: number
  status_color?: string
  status_icon?: string
}

export const useGrowthStore = defineStore('growth', () => {
  // State - Cultures
  const cultures = ref<GrowthCulture[]>([])
  
  // State - Presets
  const presets = ref<GrowthPreset[]>([])
  const selectedPreset = ref<GrowthPreset | null>(null)
  
  // State - Cycles
  const cycles = ref<GrowthCycle[]>([])
  const selectedCycle = ref<GrowthCycle | null>(null)
  
  // State - Common
  const loading = ref(false)
  const error = ref<string | null>(null)

  // Computed - Cultures
  const activeCultures = computed(() =>
    cultures.value.filter(c => c.is_active)
  )

  const culturesByCategory = computed(() => {
    const byCategory: Record<string, GrowthCulture[]> = {}
    cultures.value.forEach(culture => {
      if (!byCategory[culture.category]) {
        byCategory[culture.category] = []
      }
      byCategory[culture.category].push(culture)
    })
    return byCategory
  })

  // Computed - Presets
  const systemPresets = computed(() =>
    presets.value.filter(p => p.preset_type === 'system' && p.is_active)
  )

  const customPresets = computed(() =>
    presets.value.filter(p => p.preset_type === 'custom' && p.is_active)
  )

  const presetsByCulture = computed(() => {
    const byCulture: Record<number, GrowthPreset[]> = {}
    presets.value.forEach(preset => {
      if (!byCulture[preset.culture_id]) {
        byCulture[preset.culture_id] = []
      }
      byCulture[preset.culture_id].push(preset)
    })
    return byCulture
  })

  // Computed - Cycles
  const activeCycles = computed(() =>
    cycles.value.filter(c => c.status === 'active')
  )

  const completedCycles = computed(() =>
    cycles.value.filter(c => c.status === 'harvested' || c.status === 'ended')
  )

  const cyclesByZone = computed(() => {
    const byZone: Record<number, GrowthCycle[]> = {}
    cycles.value.forEach(cycle => {
      if (!byZone[cycle.zone_id]) {
        byZone[cycle.zone_id] = []
      }
      byZone[cycle.zone_id].push(cycle)
    })
    return byZone
  })

  const cyclesByGreenhouse = computed(() => {
    const byGreenhouse: Record<string, GrowthCycle[]> = {}
    cycles.value.forEach(cycle => {
      const key = cycle.greenhouse_id != null ? String(cycle.greenhouse_id) : 'unassigned'
      if (!byGreenhouse[key]) {
        byGreenhouse[key] = []
      }
      byGreenhouse[key].push(cycle)
    })
    return byGreenhouse
  })

  // Actions - Cultures
  async function fetchCultures() {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.get('/api/growth/cultures')
      cultures.value = response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch cultures'
      console.error('Error fetching cultures:', err)
    } finally {
      loading.value = false
    }
  }

  async function createCulture(cultureData: Partial<GrowthCulture>) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.post('/api/growth/cultures', cultureData)
      cultures.value.push(response.data)
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to create culture'
      console.error('Error creating culture:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  // Actions - Presets
  async function fetchPresets(filters?: { culture_id?: number; type?: string; system?: string }) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.get('/api/growth/presets', { params: filters })
      presets.value = response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch presets'
      console.error('Error fetching presets:', err)
    } finally {
      loading.value = false
    }
  }

  async function fetchPreset(id: number) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.get(`/api/growth/presets/${id}`)
      selectedPreset.value = response.data
      
      // Обновить в списке, если есть
      const index = presets.value.findIndex(p => p.id === id)
      if (index !== -1) {
        presets.value[index] = response.data
      }
      
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch preset'
      console.error('Error fetching preset:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function clonePreset(id: number, name: string) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.post(`/api/growth/presets/${id}/clone`, { name })
      presets.value.push(response.data)
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to clone preset'
      console.error('Error cloning preset:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  // Actions - Cycles
  async function fetchCycles(filters?: { zone_id?: number; status?: string }) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.get('/api/growth/cycles', { params: filters })
      cycles.value = response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch cycles'
      console.error('Error fetching cycles:', err)
    } finally {
      loading.value = false
    }
  }

  async function fetchCycle(id: number) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.get(`/api/growth/cycles/${id}`)
      selectedCycle.value = response.data
      
      // Обновить в списке, если есть
      const index = cycles.value.findIndex(c => c.id === id)
      if (index !== -1) {
        cycles.value[index] = response.data
      }
      
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch cycle'
      console.error('Error fetching cycle:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function createCycle(cycleData: {
    zone_id: number
    preset_id: number
    plant_count?: number
    notes?: string
    greenhouse_id?: number | null
  }) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.post('/api/growth/cycles', cycleData)
      cycles.value.push(response.data)
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to create cycle'
      console.error('Error creating cycle:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function updateCycle(id: number, cycleData: Partial<GrowthCycle>) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.put(`/api/growth/cycles/${id}`, cycleData)
      
      const index = cycles.value.findIndex(c => c.id === id)
      if (index !== -1) {
        cycles.value[index] = response.data
      }
      
      if (selectedCycle.value?.id === id) {
        selectedCycle.value = response.data
      }
      
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to update cycle'
      console.error('Error updating cycle:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function harvestCycle(id: number, harvestData: { harvest_weight_kg: number; notes?: string; rating?: number }) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.post(`/api/growth/cycles/${id}/harvest`, harvestData)
      
      const index = cycles.value.findIndex(c => c.id === id)
      if (index !== -1) {
        cycles.value[index] = response.data
      }
      
      if (selectedCycle.value?.id === id) {
        selectedCycle.value = response.data
      }
      
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to harvest cycle'
      console.error('Error harvesting cycle:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function cancelCycle(id: number, reason: string) {
    loading.value = true
    error.value = null
    
    try {
      const response = await axios.post(`/api/growth/cycles/${id}/cancel`, { reason })
      
      const index = cycles.value.findIndex(c => c.id === id)
      if (index !== -1) {
        cycles.value[index] = response.data
      }
      
      if (selectedCycle.value?.id === id) {
        selectedCycle.value = response.data
      }
      
      return response.data
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to cancel cycle'
      console.error('Error cancelling cycle:', err)
      throw err
    } finally {
      loading.value = false
    }
  }

  async function fetchCycleStats(id: number) {
    try {
      const response = await axios.get(`/api/growth/cycles/${id}/stats`)
      return response.data
    } catch (err: any) {
      console.error('Error fetching cycle stats:', err)
      throw err
    }
  }

  function getCultureById(id: number): GrowthCulture | undefined {
    return cultures.value.find(c => c.id === id)
  }

  function getPresetById(id: number): GrowthPreset | undefined {
    return presets.value.find(p => p.id === id)
  }

  function getCycleById(id: number): GrowthCycle | undefined {
    return cycles.value.find(c => c.id === id)
  }

  function getCycleByZone(zoneId: number): GrowthCycle | undefined {
    return cycles.value.find(c => c.zone_id === zoneId && c.status === 'active')
  }

  function clearError() {
    error.value = null
  }

  return {
    // State
    cultures,
    presets,
    cycles,
    selectedPreset,
    selectedCycle,
    loading,
    error,
    
    // Computed
    activeCultures,
    culturesByCategory,
    systemPresets,
    customPresets,
    presetsByCulture,
    activeCycles,
    completedCycles,
    cyclesByZone,
    cyclesByGreenhouse,
    
    // Actions
    fetchCultures,
    createCulture,
    fetchPresets,
    fetchPreset,
    clonePreset,
    fetchCycles,
    fetchCycle,
    createCycle,
    updateCycle,
    harvestCycle,
    cancelCycle,
    fetchCycleStats,
    getCultureById,
    getPresetById,
    getCycleById,
    getCycleByZone,
    clearError,
  }
})

