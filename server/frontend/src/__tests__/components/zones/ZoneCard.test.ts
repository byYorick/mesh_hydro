import { describe, expect, it, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import ZoneCard from '@/components/zones/ZoneCard.vue'

const zonesStoreMock = vi.hoisted(() => ({
  getZoneNodes: vi.fn()
}))

const growthStoreMock = vi.hoisted(() => ({
  getCycleById: vi.fn()
}))

vi.mock('@/stores/zones', () => ({
  useZonesStore: () => zonesStoreMock
}))

vi.mock('@/stores/growth', () => ({
  useGrowthStore: () => growthStoreMock
}))

describe('ZoneCard.vue', () => {
  const zoneBase = {
    id: 1,
    name: 'Zone A',
    zone_type: 'nft',
    is_active: true,
    current_cycle_id: null,
    description: 'Test zone',
    root_node_id: 'root_1'
  } as any

  it('maps zone nodes through store getter', () => {
    zonesStoreMock.getZoneNodes.mockReturnValue([
      { node_id: 'node-1', node_type: 'ph' }
    ])
    growthStoreMock.getCycleById.mockReturnValue(null)

    const wrapper = mount(ZoneCard, {
      props: {
        zone: zoneBase,
        showActions: true
      }
    })

    expect(wrapper.vm.zoneNodes).toHaveLength(1)
    expect(wrapper.vm.zoneNodes[0]).toMatchObject({ node_id: 'node-1' })
  })

  it('returns cycle info when zone has active cycle', () => {
    zonesStoreMock.getZoneNodes.mockReturnValue([])
    growthStoreMock.getCycleById.mockReturnValue({ name: 'Cycle 1', culture: { name: 'Salad' } })

    const wrapper = mount(ZoneCard, {
      props: {
        zone: { ...zoneBase, current_cycle_id: 10 }
      }
    })

    expect(wrapper.vm.cycle).toMatchObject({ name: 'Cycle 1' })
    expect(wrapper.vm.getZoneColor).toBe('warning')
  })
})
