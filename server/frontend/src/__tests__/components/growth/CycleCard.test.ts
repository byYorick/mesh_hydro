/**
 * ⭐ GROWTH PLANNER: Тесты для CycleCard компонента
 */
import { describe, it, expect, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import CycleCard from '@/components/growth/CycleCard.vue'
import type { GrowthCycle } from '@/stores/growth'

describe('CycleCard.vue', () => {
  let pinia: ReturnType<typeof createPinia>

  beforeEach(() => {
    pinia = createPinia()
  })

  const mockCycle: GrowthCycle = {
    id: 1,
    zone_id: 1,
    preset_id: 1,
    culture_id: 1,
    current_stage_id: 2,
    started_at: new Date(Date.now() - 15 * 24 * 60 * 60 * 1000).toISOString(), // 15 дней назад
    expected_harvest_at: new Date(Date.now() + 20 * 24 * 60 * 60 * 1000).toISOString(), // через 20 дней
    status: 'active',
    plant_count: 20,
    progress: 42.8,
    current_day: 15,
    remaining_days: 20,
    status_color: 'success',
    status_icon: 'mdi-play-circle',
    zone: {
      id: 1,
      name: 'Зона 1 - NFT',
    } as any,
    culture: {
      id: 1,
      name: 'Салат листовой',
      icon: 'mdi-leaf',
      category_color: 'success',
    } as any,
    current_stage: {
      id: 2,
      name: 'Вегетативный рост',
      icon: 'mdi-leaf',
      color: 'success',
    } as any,
  }

  it('renders cycle information correctly', () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: mockCycle,
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('Салат листовой')
    expect(wrapper.text()).toContain('Зона 1 - NFT')
    expect(wrapper.text()).toContain('15')
    expect(wrapper.text()).toContain('20')
    expect(wrapper.text()).toContain('20')
  })

  it('displays progress bar for active cycle', () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: { ...mockCycle, status: 'active', progress: 50 },
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    const progressBar = wrapper.findComponent({ name: 'VProgressLinear' })
    expect(progressBar.exists()).toBe(true)
    expect(progressBar.props('modelValue')).toBe(50)
  })

  it('displays current stage information', () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: mockCycle,
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('Текущая стадия')
    expect(wrapper.text()).toContain('Вегетативный рост')
  })

  it('displays harvest weight when available', () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: {
          ...mockCycle,
          status: 'harvested',
          harvest_weight_kg: 5.5,
        },
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('5.50')
    expect(wrapper.text()).toContain('кг')
  })

  it('shows correct status chip', () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: { ...mockCycle, status: 'active' },
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('Активен')
  })

  it('emits select event when clicked', async () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: mockCycle,
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    await wrapper.trigger('click')

    expect(wrapper.emitted('select')).toBeTruthy()
    expect(wrapper.emitted('select')?.[0]?.[0]).toEqual(mockCycle)
  })

  it('emits harvest event when harvest button clicked', async () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: { ...mockCycle, status: 'active' },
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    // Находим кнопку сбора урожая
    const buttons = wrapper.findAll('button')
    const harvestBtn = buttons.find(btn => btn.text().includes('Собрать урожай'))
    
    if (harvestBtn) {
      await harvestBtn.trigger('click', { stopPropagation: () => {} })
      expect(wrapper.emitted('harvest')).toBeTruthy()
    }
  })

  it('emits cancel event when cancel button clicked', async () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: { ...mockCycle, status: 'active' },
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    // Находим кнопку отмены
    const buttons = wrapper.findAll('button')
    const cancelBtn = buttons.find(btn => btn.text().includes('Отменить'))
    
    if (cancelBtn) {
      await cancelBtn.trigger('click', { stopPropagation: () => {} })
      expect(wrapper.emitted('cancel')).toBeTruthy()
    }
  })

  it('applies active class for active cycle', () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: { ...mockCycle, status: 'active' },
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.classes()).toContain('cycle-card--active')
  })

  it('hides actions when showActions is false', () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: mockCycle,
        showActions: false,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.find('v-card-actions').exists()).toBe(false)
  })

  it('formats dates correctly', () => {
    const wrapper = mount(CycleCard, {
      props: {
        cycle: mockCycle,
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    // Проверяем что даты отображаются
    expect(wrapper.text()).toContain('Начало:')
    expect(wrapper.text()).toContain('Ожидаемый сбор:')
  })
})

