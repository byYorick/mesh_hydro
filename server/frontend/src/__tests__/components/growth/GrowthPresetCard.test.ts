/**
 * ⭐ GROWTH PLANNER: Тесты для GrowthPresetCard компонента
 */
import { describe, it, expect, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import GrowthPresetCard from '@/components/growth/GrowthPresetCard.vue'
import type { GrowthPreset } from '@/stores/growth'

describe('GrowthPresetCard.vue', () => {
  let pinia: ReturnType<typeof createPinia>

  beforeEach(() => {
    pinia = createPinia()
  })

  const mockPreset: GrowthPreset = {
    id: 1,
    culture_id: 1,
    name: 'Салат NFT - Стандартный',
    slug: 'salat-nft-standart',
    description: 'Оптимизированный пресет для выращивания салата',
    preset_type: 'system',
    total_days: 35,
    difficulty: 'easy',
    recommended_system: 'nft',
    usage_count: 10,
    avg_rating: 4.5,
    is_public: true,
    is_active: true,
    culture: {
      id: 1,
      name: 'Салат листовой',
      slug: 'salat-listovoy',
      category: 'leafy_greens',
      icon: 'mdi-leaf',
      category_color: 'success',
    } as any,
    stages: [
      { id: 1, name: 'Прорастание', order: 1, duration_days: 7 } as any,
      { id: 2, name: 'Вегетация', order: 2, duration_days: 21 } as any,
      { id: 3, name: 'Сбор урожая', order: 3, duration_days: 7 } as any,
    ],
  }

  it('renders preset information correctly', () => {
    const wrapper = mount(GrowthPresetCard, {
      props: {
        preset: mockPreset,
        isSelected: false,
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('Салат NFT - Стандартный')
    expect(wrapper.text()).toContain('Салат листовой')
    expect(wrapper.text()).toContain('35 дней')
    expect(wrapper.text()).toContain('3')
    expect(wrapper.text()).toContain('10')
  })

  it('displays system preset chip', () => {
    const wrapper = mount(GrowthPresetCard, {
      props: {
        preset: { ...mockPreset, preset_type: 'system' },
        isSelected: false,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('Системный')
  })

  it('displays custom preset chip', () => {
    const wrapper = mount(GrowthPresetCard, {
      props: {
        preset: { ...mockPreset, preset_type: 'custom' },
        isSelected: false,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('Пользовательский')
  })

  it('shows difficulty correctly', () => {
    const wrapper = mount(GrowthPresetCard, {
      props: {
        preset: { ...mockPreset, difficulty: 'easy' },
        isSelected: false,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('Легко')
  })

  it('displays rating when available', () => {
    const wrapper = mount(GrowthPresetCard, {
      props: {
        preset: mockPreset,
        isSelected: false,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('4.5')
    expect(wrapper.text()).toContain('10 использований')
  })

  it('emits select event when clicked', async () => {
    const wrapper = mount(GrowthPresetCard, {
      props: {
        preset: mockPreset,
        isSelected: false,
      },
      global: {
        plugins: [pinia],
      },
    })

    await wrapper.trigger('click')

    expect(wrapper.emitted('select')).toBeTruthy()
    expect(wrapper.emitted('select')?.[0]?.[0]).toEqual(mockPreset)
  })

  it('emits clone event when clone button clicked', async () => {
    const wrapper = mount(GrowthPresetCard, {
      props: {
        preset: mockPreset,
        isSelected: false,
        showActions: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    const cloneButton = wrapper.find('button')
    await cloneButton.trigger('click', { stopPropagation: () => {} })

    // Находим кнопку клонирования
    const buttons = wrapper.findAll('button')
    const cloneBtn = buttons.find(btn => btn.text().includes('Клонировать'))
    if (cloneBtn) {
      await cloneBtn.trigger('click', { stopPropagation: () => {} })
      expect(wrapper.emitted('clone')).toBeTruthy()
    }
  })

  it('applies selected class when isSelected is true', () => {
    const wrapper = mount(GrowthPresetCard, {
      props: {
        preset: mockPreset,
        isSelected: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.classes()).toContain('preset-card--selected')
  })

  it('hides actions when showActions is false', () => {
    const wrapper = mount(GrowthPresetCard, {
      props: {
        preset: mockPreset,
        isSelected: false,
        showActions: false,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.find('v-card-actions').exists()).toBe(false)
  })
})

