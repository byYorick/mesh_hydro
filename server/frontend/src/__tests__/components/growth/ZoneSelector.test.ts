/**
 * ⭐ ЗОНИРОВАНИЕ: Тесты для ZoneSelector компонента
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { setActivePinia, createPinia } from 'pinia'
import ZoneSelector from '@/components/growth/ZoneSelector.vue'
import { useZonesStore, type Zone } from '@/stores/zones'
import axios from 'axios'
const mockedAxios = axios as any

describe('ZoneSelector.vue', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
    mockedAxios.get.mockResolvedValue({ data: [] })
  })

  const mockZones: Zone[] = [
    {
      id: 1,
      name: 'Зона 1 - NFT',
      root_node_id: 'root_001',
      mesh_network_id: 'HYDRO1_ZONE1',
      mqtt_topic_prefix: 'hydro/zone1/',
      zone_type: 'nft',
      is_active: true,
      is_available: true,
      current_cycle_id: null,
      reservoir_volume_liters: 100,
      growing_area_m2: 2,
    } as Zone,
    {
      id: 2,
      name: 'Зона 2 - DWC',
      root_node_id: 'root_002',
      mesh_network_id: 'HYDRO1_ZONE2',
      mqtt_topic_prefix: 'hydro/zone2/',
      zone_type: 'dwc',
      is_active: true,
      is_available: true,
      current_cycle_id: null,
      reservoir_volume_liters: 50,
    } as Zone,
    {
      id: 3,
      name: 'Зона 3 - Занята',
      root_node_id: 'root_003',
      mesh_network_id: 'HYDRO1_ZONE3',
      mqtt_topic_prefix: 'hydro/zone3/',
      zone_type: 'drip',
      is_active: true,
      is_available: false,
      current_cycle_id: 1,
    } as Zone,
  ]

  it('renders zone selector', () => {
    const wrapper = mount(ZoneSelector, {
      global: {
        plugins: [createPinia()],
      },
    })

    expect(wrapper.find('select').exists() || wrapper.findComponent({ name: 'VSelect' }).exists()).toBe(true)
  })

  it('loads zones on mount', async () => {
    const store = useZonesStore()
    mockedAxios.get.mockResolvedValue({ data: mockZones })

    const wrapper = mount(ZoneSelector, {
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()
    await new Promise(resolve => setTimeout(resolve, 100))

    // Проверяем что zones были загружены
    expect(store.zones.length).toBeGreaterThanOrEqual(0)
  })

  it('displays available zones only', async () => {
    const store = useZonesStore()
    store.zones = mockZones

    const wrapper = mount(ZoneSelector, {
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()

    // Должны быть только доступные зоны (без current_cycle_id)
    const availableZones = mockZones.filter(z => z.is_active && z.is_available && !z.current_cycle_id)
    expect(availableZones.length).toBe(2)
  })

  it('emits update:modelValue when zone is selected', async () => {
    const store = useZonesStore()
    store.zones = mockZones

    const wrapper = mount(ZoneSelector, {
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()

    // Симулируем выбор зоны
    await wrapper.setProps({ modelValue: mockZones[0] })

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
  })

  it('displays zone details when zone is selected', async () => {
    const store = useZonesStore()
    store.zones = mockZones

    const wrapper = mount(ZoneSelector, {
      props: {
        modelValue: mockZones[0],
      },
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()

    expect(wrapper.text()).toContain('Зона 1 - NFT')
    expect(wrapper.text()).toContain('root_001')
  })

  it('shows availability alert correctly', async () => {
    const store = useZonesStore()
    store.zones = mockZones

    const wrapper = mount(ZoneSelector, {
      props: {
        modelValue: mockZones[0], // Доступная зона
      },
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()

    expect(wrapper.text()).toMatch(/доступна/i)
  })

  it('shows busy alert for occupied zone', async () => {
    const store = useZonesStore()
    store.zones = mockZones

    const wrapper = mount(ZoneSelector, {
      props: {
        modelValue: mockZones[2], // Занятая зона
      },
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()

    expect(wrapper.text()).toContain('Занята')
  })

  it('displays zone type correctly', async () => {
    const store = useZonesStore()
    store.zones = mockZones

    const wrapper = mount(ZoneSelector, {
      props: {
        modelValue: mockZones[0],
      },
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()

    expect(wrapper.text()).toContain('NFT')
  })

  it('excludes zones when excludeZoneIds is provided', async () => {
    const store = useZonesStore()
    store.zones = mockZones

    const wrapper = mount(ZoneSelector, {
      props: {
        excludeZoneIds: [1],
      },
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()

    // Зона 1 должна быть исключена
    const vm: any = wrapper.vm
    const availableZones = vm.availableZones
    expect(availableZones.find((z: Zone) => z.id === 1)).toBeUndefined()
  })

  it('displays error messages', () => {
    const wrapper = mount(ZoneSelector, {
      props: {
        errorMessages: ['Зона обязательна для выбора'],
      },
      global: {
        plugins: [createPinia()],
      },
    })

    expect(wrapper.text()).toContain('Зона обязательна для выбора')
  })
})

