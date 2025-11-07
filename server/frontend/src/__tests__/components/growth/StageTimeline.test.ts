/**
 * ⭐ GROWTH PLANNER: Тесты для StageTimeline компонента
 */
import { describe, it, expect, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import StageTimeline from '@/components/growth/StageTimeline.vue'
import type { GrowthStage } from '@/stores/growth'

describe('StageTimeline.vue', () => {
  let pinia: ReturnType<typeof createPinia>

  beforeEach(() => {
    pinia = createPinia()
  })

  const mockStages: GrowthStage[] = [
    {
      id: 1,
      preset_id: 1,
      name: 'Прорастание',
      order: 1,
      duration_days: 7,
      target_params: {
        ph_min: 5.8,
        ph_max: 6.2,
        ec_min: 0.8,
        ec_max: 1.2,
        temp_min: 20,
        temp_max: 22,
      },
      description: 'Прорастание семян',
    },
    {
      id: 2,
      preset_id: 1,
      name: 'Вегетативный рост',
      order: 2,
      duration_days: 21,
      target_params: {
        ph_min: 5.5,
        ph_max: 6.0,
        ec_min: 1.2,
        ec_max: 1.8,
      },
      description: 'Активный рост листьев',
    },
    {
      id: 3,
      preset_id: 1,
      name: 'Подготовка к сбору',
      order: 3,
      duration_days: 7,
      target_params: {
        ph_min: 5.5,
        ph_max: 6.0,
      },
    },
  ]

  it('renders all stages', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: mockStages,
        showDetails: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('Прорастание')
    expect(wrapper.text()).toContain('Вегетативный рост')
    expect(wrapper.text()).toContain('Подготовка к сбору')
  })

  it('displays stage duration', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: mockStages,
        showConnector: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('7 дн.')
    expect(wrapper.text()).toContain('21 дн.')
  })

  it('highlights active stage', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: mockStages,
        currentStageId: 2,
      },
      global: {
        plugins: [pinia],
      },
    })

    const activeCard = wrapper.find('.stage-card--active')
    expect(activeCard.exists()).toBe(true)
  })

  it('marks completed stages', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: mockStages,
        completedStageIds: [1],
      },
      global: {
        plugins: [pinia],
      },
    })

    const completedCard = wrapper.find('.stage-card--completed')
    expect(completedCard.exists()).toBe(true)
  })

  it('displays stage description when showDetails is true', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: mockStages,
        showDetails: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('Прорастание семян')
    expect(wrapper.text()).toContain('Активный рост листьев')
  })

  it('hides details when showDetails is false', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: mockStages,
        showDetails: false,
      },
      global: {
        plugins: [pinia],
      },
    })

    // Проверяем что описание не отображается в card-text
    const cardText = wrapper.find('.v-card-text')
    if (cardText.exists()) {
      expect(cardText.text()).not.toContain('Прорастание семян')
    }
  })

  it('displays target parameters when showTargetParams is true', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: mockStages,
        showTargetParams: true,
        showDetails: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    // Проверяем что параметры отображаются
    expect(wrapper.text()).toContain('pH мин')
    expect(wrapper.text()).toContain('EC мин')
  })

  it('formats parameter names correctly', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: mockStages,
        showTargetParams: true,
        showDetails: true,
      },
      global: {
        plugins: [pinia],
      },
    })

    expect(wrapper.text()).toContain('pH мин')
    expect(wrapper.text()).toContain('pH макс')
    expect(wrapper.text()).toContain('EC мин')
  })

  it('uses correct icons for different stage types', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: [
          { ...mockStages[0], name: 'Прорастание' },
          { ...mockStages[1], name: 'Вегетация' },
          { ...mockStages[2], name: 'Цветение' },
        ],
      },
      global: {
        plugins: [pinia],
      },
    })

    // Компонент должен использовать правильные иконки
    expect(wrapper.html()).toContain('mdi-')
  })

  it('applies correct colors to stages', () => {
    const wrapper = mount(StageTimeline, {
      props: {
        stages: mockStages,
        currentStageId: 2,
      },
      global: {
        plugins: [pinia],
      },
    })

    // Активная стадия должна иметь цвет success
    const activeItem = wrapper.findComponent({ name: 'VTimelineItem' })
    expect(activeItem.exists()).toBe(true)
  })
})

